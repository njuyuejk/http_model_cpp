#include "app/ApplicationManager.h"
#include "routeManager/HttpServer.h"
#include "routeManager/RouteManager.h"
#include "routeManager/RouteInitializer.h"
#include "exception/GlobalExceptionHandler.h"
#include <iostream>

int main() {

    Logger::info("Starting 58AI Program... \n"
                 "===============================================\n"
                 "         ███████╗ █████╗  █████╗ ██╗\n"
                 "         ██╔════╝██╔══██╗██╔══██╗██║\n"
                 "         ███████╗╚█████╔╝███████║██║\n"
                 "         ╚════██║██╔══██╗██╔══██║██║\n"
                 "         ███████║╚█████╔╝██║  ██║██║\n"
                 "         ╚══════╝ ╚════╝ ╚═╝  ╚═╝╚═╝\n"
                 "===============================================\n");

    try {
        // 初始化应用程序管理器
        auto& appManager = ApplicationManager::getInstance();
        if (!appManager.initialize("./modelConfig.json")) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }

        // 初始化所有路由组
        ExceptionHandler::execute("init route", [&]() {
            RouteInitializer::initializeRoutes();
        });

        // 创建HTTP服务器
        HttpServer server(appManager.getHTTPServerConfig());

        // 配置所有路由
        ExceptionHandler::execute("configure route", [&]() {
            RouteManager::getInstance().configureRoutes(server);
        });

        const auto& grpcConfig = AppConfig::getGRPCServerConfig();
        std::string grpcAddress = grpcConfig.host + ":" + std::to_string(grpcConfig.port);
        if (!appManager.initializeGrpcServer(grpcAddress)) {
            Logger::get_instance().warning("无法在 " + grpcAddress + " 上启动gRPC服务器，将继续运行但不包含gRPC功能");
            // 即使gRPC服务器启动失败，我们也会继续执行
        }

        // 启动服务器
        if (!server.start()) {
            Logger::get_instance().error("Failed to start server");
            appManager.shutdown();
            return -1;
        }

        // 程序会在这里阻塞，直到服务器停止

        // 在程序结束时会自动调用析构函数停止服务器和关闭应用程序管理器
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error occurred during program execution: " << e.what() << std::endl;
        Logger::get_instance().fatal(std::string("Fatal program error: ") + e.what());
        return -1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred during program execution" << std::endl;
        Logger::get_instance().fatal("Unknown fatal program error");
        return -1;
    }
}