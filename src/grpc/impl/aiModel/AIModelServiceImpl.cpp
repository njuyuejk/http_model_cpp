//
// Created by YJK on 2025/5/20.
//

#include "grpc/impl/aiModel/AIModelServiceImpl.h"
#include "app/ApplicationManager.h"
#include "common/Logger.h"
#include "common/base64.h"
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono>

AIModelServiceImpl::AIModelServiceImpl(ApplicationManager& appManager)
        : appManager_(appManager) {}

grpc::Status AIModelServiceImpl::ProcessImage(
        grpc::ServerContext* context,
        const grpc_service::ImageRequest* request,
        grpc_service::ImageResponse* response) {

    // 获取请求ID用于日志跟踪
    auto requestId = std::this_thread::get_id();
    auto start_time = std::chrono::high_resolution_clock::now();

    // 开始gRPC请求监控
    appManager_.startGrpcRequest();

    try {
        Logger::info("Received gRPC ProcessImage request, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        // 验证请求参数
        std::string base64_image = request->image_base64();
        int model_type = request->model_type();

        if (base64_image.empty()) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_message("Empty image data");
            return grpc::Status::OK;
        }

        if (model_type <= 0) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_message("Invalid model type");
            return grpc::Status::OK;
        }

        // 解码base64图像
        std::vector<unsigned char> decoded_data;
        try {
            std::string decoded_str = base64_decode(base64_image);
            decoded_data = std::vector<unsigned char>(decoded_str.begin(), decoded_str.end());
        } catch (const std::exception& e) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_message("Base64 decode failed: " + std::string(e.what()));
            return grpc::Status::OK;
        }

        cv::Mat ori_img = cv::imdecode(decoded_data, cv::IMREAD_COLOR);
        if (ori_img.empty()) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_message("Image decoding failed");
            return grpc::Status::OK;
        }

        Logger::info("Processing gRPC image request - model_type: " + std::to_string(model_type) +
                     ", image_size: " + std::to_string(ori_img.cols) + "x" + std::to_string(ori_img.rows) +
                     ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));

        // 获取超时配置
        int timeout = appManager_.getConcurrencyConfig().modelAcquireTimeoutMs;

        // 使用模型池进行推理
        std::vector<std::vector<std::any>> results_vector;
        std::vector<std::string> plate_results_vector;

        bool success = appManager_.executeModelInference(model_type,
                                                         ori_img,
                                                         results_vector,
                                                         plate_results_vector,
                                                         timeout);

        if (!success) {
            appManager_.failGrpcRequest();

            // 获取模型池状态用于错误诊断
            auto poolStatus = appManager_.getModelPoolStatus(model_type);
            std::string errorDetail = "Model inference failed for type " + std::to_string(model_type);

            if (poolStatus.totalModels == 0) {
                errorDetail += " - No model instances available";
            } else if (!poolStatus.isEnabled) {
                errorDetail += " - Model pool is disabled";
            } else if (poolStatus.availableModels == 0) {
                errorDetail += " - All model instances are busy (timeout after " +
                               std::to_string(timeout) + "ms)";
            }

            response->set_success(false);
            response->set_message(errorDetail);
            return grpc::Status::OK;
        }

        // 计算处理时间
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // 填充响应
        response->set_success(true);
        response->set_message("Processing successful (time: " + std::to_string(duration.count()) + "ms)");

        // 添加检测结果 - 线程安全的结果转换
        for (const auto& inner_vec : results_vector) {
            auto* detection = response->add_detection_results();
            for (const auto& value : inner_vec) {
                try {
                    if (value.type() == typeid(float)) {
                        detection->add_values(std::any_cast<float>(value));
                    } else if (value.type() == typeid(double)) {
                        detection->add_values(static_cast<float>(std::any_cast<double>(value)));
                    } else if (value.type() == typeid(int)) {
                        detection->add_values(static_cast<float>(std::any_cast<int>(value)));
                    }
                } catch (const std::bad_any_cast& e) {
                    Logger::warning("Failed to cast result value: " + std::string(e.what()) +
                                    ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
                }
            }
        }

        // 添加车牌结果
        for (const auto& plate : plate_results_vector) {
            response->add_plate_results(plate);
        }

        Logger::info("gRPC ProcessImage completed successfully - model_type: " +
                     std::to_string(model_type) + ", time: " + std::to_string(duration.count()) + "ms" +
                     ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));

        // 完成gRPC请求监控
        appManager_.completeGrpcRequest();
        return grpc::Status::OK;

    } catch (const std::exception& e) {
        appManager_.failGrpcRequest();
        Logger::error("gRPC ProcessImage error: " + std::string(e.what()) +
                      ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
        response->set_success(false);
        response->set_message("Internal error: " + std::string(e.what()));
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status AIModelServiceImpl::ControlModel(
        grpc::ServerContext* context,
        const grpc_service::ModelControlRequest* request,
        grpc_service::ModelControlResponse* response) {

    auto requestId = std::this_thread::get_id();

    // 开始gRPC请求监控
    appManager_.startGrpcRequest();

    try {
        Logger::info("Received gRPC ControlModel request, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        std::string model_name = request->model_name();
        int model_type = request->model_type();
        bool enable = request->enabled();

        // 验证参数
        if (model_name.empty()) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_model_name("");
            response->set_enabled(false);
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty model name");
        }

        if (model_type <= 0) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_model_name(model_name);
            response->set_enabled(false);
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid model type");
        }

        // 线程安全的模型池状态设置
        bool success = appManager_.setModelEnabled(model_type, enable);

        if (!success) {
            appManager_.failGrpcRequest();
            response->set_success(false);
            response->set_model_name(model_name);
            response->set_enabled(false);
            Logger::warning("Model pool not found: model_type=" + std::to_string(model_type) +
                            ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model pool not found");
        }

        // 获取当前状态（线程安全）
        bool current_status = appManager_.isModelEnabled(model_type);
        auto poolStatus = appManager_.getModelPoolStatus(model_type);

        response->set_success(true);
        response->set_model_name(model_name);
        response->set_enabled(current_status);

        Logger::info("Model pool control successful: model_type=" +
                     std::to_string(model_type) + ", enabled=" +
                     (current_status ? "true" : "false") +
                     ", pool_size=" + std::to_string(poolStatus.totalModels) +
                     ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));

        // 完成gRPC请求监控
        appManager_.completeGrpcRequest();
        return grpc::Status::OK;

    } catch (const std::exception& e) {
        appManager_.failGrpcRequest();
        Logger::error("gRPC ControlModel error: " + std::string(e.what()) +
                      ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
        response->set_success(false);
        response->set_enabled(false);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}
