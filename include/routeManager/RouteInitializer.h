//
// Created by YJK on 2025/5/15.
//

#ifndef ROUTE_INITIALIZER_H
#define ROUTE_INITIALIZER_H

#include "RouteManager.h"
#include "routeManager/BasicRoutes.h"
#include "routeManager/UserRoutes.h"
#include "routeManager/ApiRoutes.h"

/**
 * @brief 路由初始化器
 * 负责注册所有的路由组
 */
class RouteInitializer {
public:
    /**
     * @brief 初始化所有路由组
     */
    static void initializeRoutes() {
        auto& routeManager = RouteManager::getInstance();

        // 注册基础路由组
        routeManager.addGroup(std::make_shared<BasicRoutes>());

        // 注册用户路由组
        routeManager.addGroup(std::make_shared<UserRoutes>());

        // 注册API路由组
        routeManager.addGroup(std::make_shared<ApiRoutes>());

        // 可以在这里添加更多路由组
        // 例如:
        // routeManager.addGroup(std::make_shared<AdminRoutes>());
        // routeManager.addGroup(std::make_shared<ReportRoutes>());
        // routeManager.addGroup(std::make_shared<FileRoutes>());
    }
};

#endif // ROUTE_INITIALIZER_H