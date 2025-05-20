//
// Created by YJK on 2025/5/20.
//

#include "grpc/AIModelServiceImpl.h"
#include "app/ApplicationManager.h"
#include "common/Logger.h"
#include "common/base64.h"
#include "opencv2/opencv.hpp"

AIModelServiceImpl::AIModelServiceImpl(ApplicationManager& appManager)
        : appManager_(appManager) {}

grpc::Status AIModelServiceImpl::ProcessImage(
        grpc::ServerContext* context,
        const grpc_service::ImageRequest* request,
        grpc_service::ImageResponse* response) {

    try {
        Logger::info("收到gRPC ProcessImage请求");

        // 解码base64图像
        std::string base64_image = request->image_base64();
        int model_type = request->model_type();

        std::vector<unsigned char> decoded_data =
                std::vector<unsigned char>(base64_decode(base64_image).begin(),
                                           base64_decode(base64_image).end());

        cv::Mat ori_img = cv::imdecode(decoded_data, cv::IMREAD_COLOR);
        if (ori_img.empty()) {
            response->set_success(false);
            response->set_message("图像解码失败");
            return grpc::Status::OK;
        }

        // 使用AI模型处理
        std::vector<std::vector<std::any>> results_vector;
        std::vector<std::string> plate_results_vector;
        bool model_found = false;

        for (auto& model : appManager_.singleModelPools_) {
            // 选择正确的模型
            if (model->modelType != model_type) {
                continue;
            }

            model_found = true;

            // 检查模型是否启用
            if (!model->isEnabled) {
                response->set_success(false);
                response->set_message("模型未启用");
                return grpc::Status::OK;
            }

            model->singleRKModel->ori_img = ori_img;
            if (!model->singleRKModel->interf()) {
                response->set_success(false);
                response->set_message("模型推理失败");
                return grpc::Status::OK;
            }

            results_vector = model->singleRKModel->results_vector;

            if (model_type == 1) {
                plate_results_vector = model->singleRKModel->plateResults;
            }
        }

        if (!model_found) {
            response->set_success(false);
            response->set_message("未找到模型类型");
            return grpc::Status::OK;
        }

        // 填充响应
        response->set_success(true);
        response->set_message("处理成功");

        // 添加检测结果
        for (const auto& inner_vec : results_vector) {
            auto* detection = response->add_detection_results();
            for (const auto& value : inner_vec) {
                if (value.type() == typeid(float)) {
                    detection->add_values(std::any_cast<float>(value));
                } else if (value.type() == typeid(double)) {
                    detection->add_values(static_cast<float>(std::any_cast<double>(value)));
                } else if (value.type() == typeid(int)) {
                    detection->add_values(static_cast<float>(std::any_cast<int>(value)));
                }
                // 根据需要处理其他类型
            }
        }

        // 添加车牌结果
        for (const auto& plate : plate_results_vector) {
            response->add_plate_results(plate);
        }

        Logger::info("gRPC ProcessImage处理成功完成");
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        Logger::error("gRPC ProcessImage错误: " + std::string(e.what()));
        response->set_success(false);
        response->set_message("错误: " + std::string(e.what()));
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status AIModelServiceImpl::ControlModel(
        grpc::ServerContext* context,
        const grpc_service::ModelControlRequest* request,
        grpc_service::ModelControlResponse* response) {

    try {
        Logger::info("收到gRPC ControlModel请求");

        std::string model_name = request->model_name();
        int model_type = request->model_type();
        bool enable = request->enabled();

        bool found = false;

        // 更新模型状态
        for (auto& model : appManager_.singleModelPools_) {
            if (model->modelType == model_type) {
                model->isEnabled = enable;
                found = true;

                response->set_success(true);
                response->set_model_name(model->modelName);
                response->set_enabled(model->isEnabled);

                Logger::info("模型控制成功: model_type=" +
                             std::to_string(model_type) + ", enabled=" +
                             (enable ? "true" : "false"));
            }
        }

        if (!found) {
            response->set_success(false);
            response->set_model_name(model_name);
            response->set_enabled(false);
            Logger::warning("未找到模型: model_type=" + std::to_string(model_type));
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "未找到模型");
        }

        return grpc::Status::OK;
    } catch (const std::exception& e) {
        Logger::error("gRPC ControlModel错误: " + std::string(e.what()));
        response->set_success(false);
        response->set_enabled(false);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}