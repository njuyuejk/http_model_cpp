#include "httplib.h"
#include "nlohmann/json.hpp"

// 引入处理器
#include "handlers/root_handler.h"
#include "handlers/user_handler.h"
#include "handlers/api_handler.h"

#include <iostream>
#include <string>

using json = nlohmann::json;

int main() {
    // 获取服务器配置
//    ServerConfig config = Config::get_server_config();

    std::vector<std::string> tests;
    tests.clear();
    tests.push_back("aaaaaa");
    std::cout << "vector size is: " << tests.size() << std::endl;
    tests.clear();
    std::cout << "vector111111 size is: " << tests.size() << std::endl;

    // 创建 HTTP 服务器对象
    httplib::Server svr;

    // 记录服务器启动信息
    Logger::get_instance().log("Starting server on 0.0.0.0:" + std::to_string(8888));

    // 设置路由处理器
    svr.Get("/", Handlers::handle_root);
    svr.Get(R"(/user/(\w+))", Handlers::handle_user);
    svr.Post("/api/data", Handlers::handle_api_data);
    svr.Get("/api/data", Handlers::handle_api_data);


    // 自定义错误处理
    svr.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        json error_json = {
                {"error", "Not Found"},
                {"path", req.path}
        };
        res.set_content(error_json.dump(), "application/json");
        Logger::get_instance().log("404 Not Found: " + req.path, WARNING);
    });

    // 启动服务器
    if (!svr.listen("0.0.0.0", 8888)) {
        Logger::get_instance().log("Failed to start server", ERRORS);
        return -1;
    }

    return 0;
}
