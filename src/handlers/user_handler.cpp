//
// Created by YJK on 2025/1/2.
//

#include "handlers/user_handler.h"
#include "nlohmann/json.hpp"


using json = nlohmann::json;

void Handlers::handle_user(const httplib::Request& req, httplib::Response& res) {
    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content("Bad Request", "text/plain");
        return;
    }
    std::string username = req.matches[1];

    // 示例：使用模型处理（假设用户名需要通过模型处理）
    json response_json = {
            {"status", "success"},
            {"user", username},
            {"model_response", ""}
    };
    res.set_content(response_json.dump(), "application/json");
}
