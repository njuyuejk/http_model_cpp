//
// Created by YJK on 2025/1/2.
//

// src/handlers/user_handler.cpp - 已更新
#include "handlers/user_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"

using json = nlohmann::json;

void Handlers::handle_user(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            throw APIException("Invalid username parameter", 400);
        }

        std::string username = req.matches[1];

        // 验证用户名
        if (username.empty() || username.length() > 50) {
            throw APIException("Invalid username length", 400);
        }

        // 这里可以添加更多验证逻辑

        // 示例：使用模型处理（假设用户名需要通过模型处理）
        try {
            // 模型处理逻辑可以放在这里
            // 如果模型处理抛出异常，会被外部的ExceptionHandler捕获

            json response_json = {
                    {"status", "success"},
                    {"user", username},
                    {"model_response", ""}
            };

            res.set_content(response_json.dump(), "application/json");
        } catch (const std::exception& e) {
            throw APIException(std::string("User information processing error: ") + e.what(), 500);
        }
    });
}