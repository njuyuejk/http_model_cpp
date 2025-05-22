//
// Created by YJK on 2025/5/22.
//

#ifndef STATUS_ROUTES_H
#define STATUS_ROUTES_H

#include "routeManager/RouteManager.h"
#include "handlers/status_handler.h"

/**
 * @brief 状态监控路由组
 * 包含系统状态和监控相关接口
 */
class StatusRoutes : public BaseRouteGroup {
public:
    StatusRoutes() : BaseRouteGroup("status", "/api/status", "系统状态监控接口") {}

    void registerRoutes(HttpServer& server) override {
        // 系统状态接口
        server.addGet("/api/status/system", Handlers::handle_system_status, "获取系统状态")
                .addGet("/api/status/models", Handlers::handle_model_pools_status, "获取模型池状态")
                .addGet("/api/status/concurrency", Handlers::handle_concurrency_stats, "获取并发统计");
    }
};

#endif // STATUS_ROUTES_H
