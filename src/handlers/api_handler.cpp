//
// Created by YJK on 2025/1/2.
//

// src/handlers/api_handler.cpp - 已更新
#include "handlers/api_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"

using json = nlohmann::json;

void Handlers::handle_api_data(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        // 检查内容类型
        if (!req.has_header("Content-Type") || req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
            throw APIException("请求必须包含 'application/json' Content-Type", 415);
        }

        // 解析 JSON 数据
        json received_json;
        try {
            received_json = json::parse(req.body);
        } catch (const json::exception& e) {
            throw JSONParseException(std::string("无效的JSON格式: ") + e.what());
        }

        // 验证必要字段
        if (!received_json.contains("message")) {
            throw APIException("请求必须包含'message'字段", 400);
        }

        std::string message = received_json["message"];

        // 处理数据
        // 示例：这里可以添加模型相关的处理逻辑

        // 构建响应 JSON
        json response_json = {
                {"状态", "成功"},
                {"消息", message},
                {"处理后消息", message},
                {"已接收", true}
        };

        // 发送响应
        res.set_content(response_json.dump(), "application/json");
    });
}