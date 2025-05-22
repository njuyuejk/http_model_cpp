//
// Created by YJK on 2025/1/2.
//

/*
 * 没增加并发的原始逻辑
 * */
//// src/handlers/api_handler.cpp - 已更新
//#include "handlers/api_handler.h"
//#include "nlohmann/json.hpp"
//#include "exception/GlobalExceptionHandler.h"
//#include "common/base64.h"
//#include "opencv2/opencv.hpp"
//#include "app/ApplicationManager.h"
//
//using json = nlohmann::json;
//
//void Handlers::handle_api_model_process(const httplib::Request& req, httplib::Response& res) {
//    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
//        // 检查内容类型
//        if (!req.has_header("Content-Type") || req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
//            throw APIException("Request must include 'application/json' Content-Type", 415);
//        }
//
//        // 解析 JSON 数据
//        json received_json;
//        try {
//            received_json = json::parse(req.body);
//        } catch (const json::exception& e) {
//            throw JSONParseException(std::string("Invalid JSON format: ") + e.what());
//        }
//
//        // 验证必要字段
//        if (!received_json.contains("img")) {
//            throw APIException("Request must include 'img' field", 400);
//        }
//
//        if (!received_json.contains("modelType")) {
//            throw APIException("Request must include 'modelType' field", 400);
//        }
//
//        std::string message = received_json["img"];
//        int modelType = received_json["modelType"];
//        std::vector<std::string> plateResults_vector;
//
//        std::vector<unsigned char> decoded_data = std::vector<unsigned char>(base64_decode(message).begin(), base64_decode(message).end());
//
//        cv::Mat ori_img = cv::imdecode(decoded_data, cv::IMREAD_COLOR);
//        if (ori_img.empty()) {
//            throw APIException("image decode failed", 400);
//        }
//
//        // 获取ApplicationManager实例
//        auto& appManager = ApplicationManager::getInstance();
//
//        // 检查模型是否启用
//        if (!appManager.isModelEnabled(modelType)) {
//            throw APIException("Model is not enabled", 400);
//        }
//
//        // 获取模型的共享引用
//        auto modelRef = appManager.getSharedModelReference(modelType);
//        if (!modelRef) {
//            throw APIException("Model not found for the specified type", 404);
//        }
//
//        std::vector<std::vector<std::any>> results_vector;
//
//        modelRef->ori_img = ori_img;
//        if (!modelRef->interf()) {
//            throw ModelException("model inference field, please check input", "Unknown model");
//        }
//
//        cv::Mat dstMat = modelRef->ori_img;
//        results_vector = modelRef->results_vector;
//
//        if (modelType == 1) {
//            plateResults_vector = modelRef->plateResults;
//        }
//
////        cv::imwrite("./test.jpg", ori_img);
//
//        Logger::info("output ori_img width: "+ std::to_string(ori_img.cols));
//
//        // 手动将嵌套向量转换为 JSON
//        json json_data = json::array();
//        for (const auto& inner_vec : results_vector) {
//            json inner_json = json::array();
//            for (const auto& item : inner_vec) {
//                inner_json.push_back(any_to_json(item));
//            }
//            json_data.push_back(inner_json);
//        }
//
//        // 构建响应 JSON
//        json response_json = {
//                {"status", "success"},
//                {"message", ori_img.cols},
//                {"processed_message", ori_img.rows},
//                {"detect_results", json_data},
//                {"plate_results", plateResults_vector},
//                {"detect_type", modelType},
//                {"received", true}
//        };
//
//        // 发送响应
//        res.set_content(response_json.dump(), "application/json");
//
//        results_vector.clear();
//        std::vector<std::vector<std::any>>().swap(results_vector);
//    });
//}

#include "handlers/api_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"
#include "common/base64.h"
#include "opencv2/opencv.hpp"
#include "app/ApplicationManager.h"
#include <chrono>

using json = nlohmann::json;

void Handlers::handle_api_model_process(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        auto& appManager = ApplicationManager::getInstance();

        // 开始请求监控
        appManager.startHttpRequest();

        try {
            // 获取请求开始时间
            auto start_time = std::chrono::high_resolution_clock::now();

            // 检查内容类型
            if (!req.has_header("Content-Type") ||
                req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
                appManager.failHttpRequest();
                throw APIException("Request must include 'application/json' Content-Type", 415);
            }

            // 解析 JSON 数据
            json received_json;
            try {
                received_json = json::parse(req.body);
            } catch (const json::exception& e) {
                appManager.failHttpRequest();
                throw JSONParseException(std::string("Invalid JSON format: ") + e.what());
            }

            // 验证必要字段
            if (!received_json.contains("img")) {
                appManager.failHttpRequest();
                throw APIException("Request must include 'img' field", 400);
            }

            if (!received_json.contains("modelType")) {
                appManager.failHttpRequest();
                throw APIException("Request must include 'modelType' field", 400);
            }

            std::string message = received_json["img"];
            int modelType = received_json["modelType"];

            // 获取超时配置
            int timeout = appManager.getConcurrencyConfig().modelAcquireTimeoutMs;
            if (received_json.contains("timeout") && received_json["timeout"].is_number_integer()) {
                timeout = received_json["timeout"];
            }

            // 验证模型类型
            if (modelType <= 0) {
                appManager.failHttpRequest();
                throw APIException("Invalid model type", 400);
            }

            // 解码Base64图像
            std::vector<unsigned char> decoded_data;
            try {
                std::string decoded_str = base64_decode(message);
                decoded_data = std::vector<unsigned char>(decoded_str.begin(), decoded_str.end());
            } catch (const std::exception& e) {
                appManager.failHttpRequest();
                throw APIException("Base64 decode failed: " + std::string(e.what()), 400);
            }

            cv::Mat ori_img = cv::imdecode(decoded_data, cv::IMREAD_COLOR);
            if (ori_img.empty()) {
                appManager.failHttpRequest();
                throw APIException("Image decode failed", 400);
            }

            // 记录图像处理开始
            Logger::info("Processing image request - model_type: " + std::to_string(modelType) +
                         ", image_size: " + std::to_string(ori_img.cols) + "x" + std::to_string(ori_img.rows));

            // 使用模型池进行推理
            std::vector<std::vector<std::any>> results_vector;
            std::vector<std::string> plateResults_vector;

            bool success = appManager.executeModelInference(modelType,
                                                            ori_img,
                                                            results_vector,
                                                            plateResults_vector,
                                                            timeout);

            if (!success) {
                appManager.failHttpRequest();
                // 获取模型池状态用于错误诊断
                auto poolStatus = appManager.getModelPoolStatus(modelType);
                std::string errorDetail = "Model inference failed for type " + std::to_string(modelType);
                if (poolStatus.totalModels == 0) {
                    errorDetail += " - No model instances available";
                } else if (!poolStatus.isEnabled) {
                    errorDetail += " - Model pool is disabled";
                } else if (poolStatus.availableModels == 0) {
                    errorDetail += " - All model instances are busy";
                }
                throw APIException(errorDetail, 503);
            }

            // 计算处理时间
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            // 转换结果为JSON
            json json_data = json::array();
            for (const auto& inner_vec : results_vector) {
                json inner_json = json::array();
                for (const auto& item : inner_vec) {
                    inner_json.push_back(any_to_json(item));
                }
                json_data.push_back(inner_json);
            }

            // 构建响应 JSON
            json response_json = {
                    {"status", "success"},
                    {"message", "Processing completed successfully"},
                    {"image_width", ori_img.cols},
                    {"image_height", ori_img.rows},
                    {"detect_results", json_data},
                    {"plate_results", plateResults_vector},
                    {"detect_type", modelType},
                    {"processing_time_ms", duration.count()},
                    {"received", true}
            };

            // 添加并发状态信息（如果启用监控）
            if (appManager.getConcurrencyConfig().enableConcurrencyMonitoring) {
                auto httpStats = appManager.getHttpConcurrencyStats();
                auto poolStatus = appManager.getModelPoolStatus(modelType);

                response_json["concurrency_info"] = {
                        {"active_http_requests", httpStats.active},
                        {"total_http_requests", httpStats.total},
                        {"model_pool_status", {
                                                         {"total_models", poolStatus.totalModels},
                                                         {"available_models", poolStatus.availableModels},
                                                         {"busy_models", poolStatus.busyModels}
                                                 }}
                };
            }

            // 发送响应
            res.set_content(response_json.dump(), "application/json");

            // 记录成功处理
            Logger::info("Image processing completed successfully - model_type: " +
                         std::to_string(modelType) + ", time: " + std::to_string(duration.count()) + "ms");

            // 完成请求监控
            appManager.completeHttpRequest();

        } catch (...) {
            appManager.failHttpRequest();
            throw; // 重新抛出异常让ExceptionHandler处理
        }
    });
}