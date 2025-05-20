#include "app/ApplicationManager.h"
#include "routeManager/HttpServer.h"
#include "routeManager/RouteManager.h"
#include "routeManager/RouteInitializer.h"
#include "exception/GlobalExceptionHandler.h"
#include "grpc/GrpcServer.h"
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

        // 初始化gRPC服务器
        std::unique_ptr<GrpcServer> grpcServer;
        ExceptionHandler::execute("初始化gRPC服务器", [&]() {
            std::string grpcAddress = appManager.getGrpcServerAddress();
            Logger::info("正在初始化gRPC服务器，地址: " + grpcAddress);
            grpcServer = std::make_unique<GrpcServer>(grpcAddress, appManager);
            if (!grpcServer->start()) {
                Logger::warning("在 " + grpcAddress + " 上启动gRPC服务器失败，将继续运行但不包含gRPC功能");
            } else {
                Logger::info("gRPC服务器在 " + grpcAddress + " 上成功启动");
            }
        });

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