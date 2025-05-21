//
// Created by YJK on 2025/5/15.
//

#ifndef BASIC_ROUTES_H
#define BASIC_ROUTES_H

#include "routeManager/RouteManager.h"
#include "handlers/root_handler.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

/**
 * @brief 基础路由组
 * 包含主页和错误处理等基本路由
 */
class BasicRoutes : public BaseRouteGroup {
public:
    BasicRoutes() : BaseRouteGroup("basic", "/", "基础路由组") {}

    void registerRoutes(HttpServer& server) override {
        // 注册根路由
        server.addGet("/", Handlers::handle_root, "首页");

        // 配置错误处理
        server.setErrorHandler([](const httplib::Request& req, httplib::Response& res) {
//            json error_json = {
//                    {"error", "Not Found"},
//                    {"path", req.path}
//            };
            APIException notFoundError("未找到路径: " + req.path, 404);
            ExceptionHandler::setErrorResponse(res, notFoundError, &req);
//            res.set_content(error_json.dump(), "application/json");
            Logger::get_instance().log("404 Not Found: " + req.path, WARNING);
        });

        // 配置异常处理
        server.setExceptionHandler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
            try {
                if (ep) {
                    std::rethrow_exception(ep);
                }
            } catch (const std::exception& e) {
                json error_json = {
                        {"error", "Internal Server Error"},
                        {"message", e.what()},
                        {"path", req.path}
                };
                res.status = 500;
                res.set_content(error_json.dump(), "application/json");
                Logger::get_instance().error("Server exception: " + std::string(e.what()) + " path: " + req.path);
            }
        });
    }
};

#endif // BASIC_ROUTES_H