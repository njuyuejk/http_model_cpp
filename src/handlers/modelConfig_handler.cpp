//
// Created by YJK on 2025/1/2.
//

/*
 * 没增加并发的原始逻辑
 * */
//// src/handlers/user_handler.cpp - 已更新
//#include "handlers/modelConfig_handler.h"
//#include "nlohmann/json.hpp"
//#include "exception/GlobalExceptionHandler.h"
//#include "app/ApplicationManager.h"
//
//using json = nlohmann::json;
//
//void Handlers::handle_model_config(const httplib::Request& req, httplib::Response& res) {
//    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
//        if (req.matches.size() < 2) {
//            throw APIException("Invalid username parameter", 400);
//        }
//
//        std::string modelName = req.matches[1];
//
//        // 验证模型名称
//        if (modelName.empty() || modelName.length() > 50) {
//            throw APIException("Invalid username length", 400);
//        }
//
//        if (!req.has_param("modelType")) {
//            throw APIException("Missing required query parameter: modelType", 400);
//        }
//        if (!req.has_param("isEnabled")) {
//            throw APIException("Missing required query parameter: isEabled", 400);
//        }
//
//        std::string model_type_str = req.get_param_value("modelType");
//        std::string is_enabled_str = req.get_param_value("isEnabled");
//
//        int model_type;
//        try {
//            model_type = std::stoi(model_type_str);
//        } catch (const std::exception&) {
//            throw APIException("Invalid modelType parameter: must be an integer", 400);
//        }
//
//        bool isEnabled;
//        if (is_enabled_str == "true" || is_enabled_str == "1") {
//            isEnabled = true;
//        } else if (is_enabled_str == "false" || is_enabled_str == "0") {
//            isEnabled = false;
//        } else {
//            throw APIException("Invalid isEabled parameter: must be 'true', 'false', '1', or '0'", 400);
//        }
//
//        // 这里可以添加更多验证逻辑
//
//        try {
//            // 使用ApplicationManager设置模型状态
//            auto& appManager = ApplicationManager::getInstance();
//            bool success = appManager.setModelEnabled(model_type, isEnabled);
//
//            if (!success) {
//                throw APIException("Model not found for the specified type", 404);
//            }
//
//            json response_json = {
//                    {"status", "success"},
//                    {"model_name", modelName},
//                    {"model_response", isEnabled}
//            };
//
//            res.set_content(response_json.dump(), "application/json");
//        } catch (const std::exception& e) {
//            throw APIException(std::string("User information processing error: ") + e.what(), 500);
//        }
//    });
//}

#include "handlers/modelConfig_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"
#include "app/ApplicationManager.h"

using json = nlohmann::json;

void Handlers::handle_model_config(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        auto& appManager = ApplicationManager::getInstance();

        // 开始请求监控
        appManager.startHttpRequest();

        try {
            if (req.matches.size() < 2) {
                appManager.failHttpRequest();
                throw APIException("Invalid model name parameter", 400);
            }

            std::string modelName = req.matches[1];

            // 验证模型名称
            if (modelName.empty() || modelName.length() > 50) {
                appManager.failHttpRequest();
                throw APIException("Invalid model name length", 400);
            }

            // 处理GET请求 - 获取模型状态
            if (req.method == "GET") {
                if (!req.has_param("modelType")) {
                    appManager.failHttpRequest();
                    throw APIException("Missing required query parameter: modelType", 400);
                }

                std::string model_type_str = req.get_param_value("modelType");
                int model_type;
                try {
                    model_type = std::stoi(model_type_str);
                } catch (const std::exception&) {
                    appManager.failHttpRequest();
                    throw APIException("Invalid modelType parameter: must be an integer", 400);
                }

                // 获取模型池状态
                auto poolStatus = appManager.getModelPoolStatus(model_type);

                json response_json = {
                        {"status", "success"},
                        {"model_name", modelName},
                        {"model_type", model_type},
                        {"enabled", poolStatus.isEnabled},
                        {"pool_info", {
                                           {"total_models", poolStatus.totalModels},
                                           {"available_models", poolStatus.availableModels},
                                           {"busy_models", poolStatus.busyModels},
                                           {"model_path", poolStatus.modelPath},
                                           {"threshold", poolStatus.threshold}
                                   }}
                };

                res.set_content(response_json.dump(), "application/json");
                appManager.completeHttpRequest();
                return;
            }

            // 处理POST请求 - 设置模型状态
            if (!req.has_param("modelType")) {
                appManager.failHttpRequest();
                throw APIException("Missing required query parameter: modelType", 400);
            }
            if (!req.has_param("isEnabled")) {
                appManager.failHttpRequest();
                throw APIException("Missing required query parameter: isEnabled", 400);
            }

            std::string model_type_str = req.get_param_value("modelType");
            std::string is_enabled_str = req.get_param_value("isEnabled");

            int model_type;
            try {
                model_type = std::stoi(model_type_str);
            } catch (const std::exception&) {
                appManager.failHttpRequest();
                throw APIException("Invalid modelType parameter: must be an integer", 400);
            }

            bool isEnabled;
            if (is_enabled_str == "true" || is_enabled_str == "1") {
                isEnabled = true;
            } else if (is_enabled_str == "false" || is_enabled_str == "0") {
                isEnabled = false;
            } else {
                appManager.failHttpRequest();
                throw APIException("Invalid isEnabled parameter: must be 'true', 'false', '1', or '0'", 400);
            }

            // 设置模型池状态
            bool success = appManager.setModelEnabled(model_type, isEnabled);

            if (!success) {
                appManager.failHttpRequest();
                throw APIException("Model pool not found for the specified type", 404);
            }

            // 获取更新后的状态
            auto poolStatus = appManager.getModelPoolStatus(model_type);

            json response_json = {
                    {"status", "success"},
                    {"model_name", modelName},
                    {"model_type", model_type},
                    {"enabled", poolStatus.isEnabled},
                    {"message", "Model status updated successfully"},
                    {"pool_info", {
                                       {"total_models", poolStatus.totalModels},
                                       {"available_models", poolStatus.availableModels},
                                       {"busy_models", poolStatus.busyModels}
                               }}
            };

            res.set_content(response_json.dump(), "application/json");
            appManager.completeHttpRequest();

        } catch (...) {
            appManager.failHttpRequest();
            throw; // 重新抛出异常让ExceptionHandler处理
        }
    });
}