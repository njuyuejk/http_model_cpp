//
// Created by YJK on 2025/5/15.
//

#ifndef API_ROUTES_H
#define API_ROUTES_H

#include "routeManager/RouteManager.h"
#include "handlers/api_handler.h"

/**
 * @brief API路由组
 * 包含API相关接口
 */
class ApiRoutes : public BaseRouteGroup {
public:
    ApiRoutes() : BaseRouteGroup("api", "/api", "API接口") {}

    void registerRoutes(HttpServer& server) override {
        // API数据接口
//        server.addPost("/api/data", Handlers::handle_api_data, "提交API数据")
//                .addGet("/api/data", Handlers::handle_api_data, "获取API数据");

        // 这里可以添加更多API接口，例如：
        // server.addGet("/api/status", Handlers::handle_api_status, "获取API状态");
        // server.addPost("/api/upload", Handlers::handle_api_upload, "上传文件");
        // server.addGet("/api/metrics", Handlers::handle_api_metrics, "获取指标数据");
    }
};

#endif // API_ROUTES_H