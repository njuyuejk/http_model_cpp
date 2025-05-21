//
// Created by YJK on 2025/5/15.
//

#ifndef ROUTE_INITIALIZER_H
#define ROUTE_INITIALIZER_H

#include "routeManager/RouteManager.h"
#include "routeManager/base/BasicRoutes.h"
#include "routeManager/route/ModelConfigRoutes.h"
#include "routeManager/route/ApiRoutes.h"
#include "routeManager/route/ModelRoutes.h"

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

        // 注册API路由组
        routeManager.addGroup(std::make_shared<ApiRoutes>());

        // 注册模型路由
        routeManager.addGroup(std::make_shared<ModelRoutes>());

        // 注册模型配置路由组
        routeManager.addGroup(std::make_shared<ModelConfigRoutes>());

        // 可以在这里添加更多路由组
        // 例如:
        // routeManager.addGroup(std::make_shared<AdminRoutes>());
        // routeManager.addGroup(std::make_shared<ReportRoutes>());
        // routeManager.addGroup(std::make_shared<FileRoutes>());
    }
};

#endif // ROUTE_INITIALIZER_H