//
// Created by YJK on 2025/1/2.
//

#include "handlers/api_handler.h"
#include "nlohmann/json.hpp"


using json = nlohmann::json;

void Handlers::handle_api_data(const httplib::Request& req, httplib::Response& res) {
    try {
        // 解析 JSON 数据
        json received_json = json::parse(req.body);

        // 示例：使用模型处理数据
        std::string message = received_json.value("message", "");

        // 构建响应 JSON
        json response_json;
        response_json["message"] = message;
        response_json["processed_message"] = message;
        response_json["received"] = true;

        // 发送响应
        res.set_content(response_json.dump(), "application/json");
    } catch (json::parse_error& e) {
        res.status = 400;
        res.set_content("Invalid JSON", "text/plain");
    } catch (std::exception& e) {
        res.status = 500;
        res.set_content("Internal Server Error", "text/plain");
    }
}