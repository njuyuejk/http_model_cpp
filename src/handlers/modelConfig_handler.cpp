//
// Created by YJK on 2025/1/2.
//

// src/handlers/user_handler.cpp - 已更新
#include "handlers/modelConfig_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"
#include "app/ApplicationManager.h"

using json = nlohmann::json;

void Handlers::handle_model_config(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            throw APIException("Invalid username parameter", 400);
        }

        std::string modelName = req.matches[1];

        // 验证用户名
        if (modelName.empty() || modelName.length() > 50) {
            throw APIException("Invalid username length", 400);
        }

        if (!req.has_param("modelType")) {
            throw APIException("Missing required query parameter: modelType", 400);
        }
        if (!req.has_param("isEnabled")) {
            throw APIException("Missing required query parameter: isEabled", 400);
        }

        std::string model_type_str = req.get_param_value("modelType");
        std::string is_enabled_str = req.get_param_value("isEnabled");

        int model_type;
        try {
            model_type = std::stoi(model_type_str);
        } catch (const std::exception&) {
            throw APIException("Invalid modelType parameter: must be an integer", 400);
        }

        bool isEnabled;
        if (is_enabled_str == "true" || is_enabled_str == "1") {
            isEnabled = true;
        } else if (is_enabled_str == "false" || is_enabled_str == "0") {
            isEnabled = false;
        } else {
            throw APIException("Invalid isEabled parameter: must be 'true', 'false', '1', or '0'", 400);
        }

        // 这里可以添加更多验证逻辑

        try {
            // 模型处理逻辑可以放在这里
            // 如果模型处理抛出异常，会被外部的ExceptionHandler捕获
            for (auto &model: ApplicationManager::getInstance().singleModelPools_) {
                if (model->modelType == model_type) {
                    model->isEnabled = isEnabled;
                }
            }

            json response_json = {
                    {"status", "success"},
                    {"model_name", modelName},
                    {"model_response", isEnabled}
            };

            res.set_content(response_json.dump(), "application/json");
        } catch (const std::exception& e) {
            throw APIException(std::string("User information processing error: ") + e.what(), 500);
        }
    });
}