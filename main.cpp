#include "app/ApplicationManager.h"
#include "routeManager/HttpServer.h"
#include "routeManager/RouteManager.h"
#include "routeManager/RouteInitializer.h"
#include <iostream>

int main() {
    // 初始化应用程序管理器
    auto& appManager = ApplicationManager::getInstance();
    if (!appManager.initialize("modelConfig.json")) {
        std::cerr << "初始化应用程序失败" << std::endl;
        return -1;
    }

    // 初始化所有路由组
    RouteInitializer::initializeRoutes();

    // 创建HTTP服务器
    HttpServer server(appManager.getHTTPServerConfig());

    // 配置所有路由
    RouteManager::getInstance().configureRoutes(server);

    // 启动服务器
    if (!server.start()) {
        Logger::get_instance().error("服务器启动失败");
        appManager.shutdown();
        return -1;
    }

    // 程序会在这里阻塞，直到服务器停止

    // 在程序结束时会自动调用析构函数停止服务器和关闭应用程序管理器
    return 0;
}
