//
// Created by YJK on 2025/5/15.
//
#include "routeManager/HttpServer.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

HttpServer::HttpServer(const HTTPServerConfig& serverConfig)
        : config(serverConfig), running(false) {
    // 初始化服务器
}

HttpServer::~HttpServer() {
    if (running) {
        stop();
    }
}

HttpServer& HttpServer::addGet(const std::string& pattern,
                               httplib::Server::Handler handler,
                               const std::string& description) {
    routes.emplace_back("GET", pattern, description, handler);
    return *this;
}

HttpServer& HttpServer::addPost(const std::string& pattern,
                                httplib::Server::Handler handler,
                                const std::string& description) {
    routes.emplace_back("POST", pattern, description, handler);
    return *this;
}

HttpServer& HttpServer::addPut(const std::string& pattern,
                               httplib::Server::Handler handler,
                               const std::string& description) {
    routes.emplace_back("PUT", pattern, description, handler);
    return *this;
}

HttpServer& HttpServer::addDelete(const std::string& pattern,
                                  httplib::Server::Handler handler,
                                  const std::string& description) {
    routes.emplace_back("DELETE", pattern, description, handler);
    return *this;
}

HttpServer& HttpServer::setErrorHandler(httplib::Server::Handler handler) {
    server.set_error_handler(handler);
    return *this;
}

HttpServer& HttpServer::setExceptionHandler(httplib::Server::ExceptionHandler handler) {
    server.set_exception_handler(handler);
    return *this;
}

void HttpServer::registerRoutes() {
    for (const auto& route : routes) {
        if (route.method == "GET") {
            server.Get(route.pattern, route.handler);
            LOGGER_INFO("Registering route: GET " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "POST") {
            server.Post(route.pattern, route.handler);
            LOGGER_INFO("Registering route: POST " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "PUT") {
            server.Put(route.pattern, route.handler);
            LOGGER_INFO("Registering route: PUT " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "DELETE") {
            server.Delete(route.pattern, route.handler);
            LOGGER_INFO("Registering route: DELETE " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        }
    }
}

bool HttpServer::start() {
    if (running) {
        LOGGER_WARNING("Server is already running");
        return true;
    }

    // 在启动前注册所有路由
    registerRoutes();

    LOGGER_INFO("Starting server " + config.host + ":" + std::to_string(config.port));

    // 设置超时（如果配置有指定）
    if (config.connectionTimeout > 0) {
        server.set_keep_alive_timeout(config.connectionTimeout);
    }

    if (config.readTimeout > 0) {
        server.set_read_timeout(config.readTimeout);
    }

    // 在新线程中启动服务器
    serverStarted = false;
    serverThread = std::make_unique<std::thread>([this]() {
        // 标记服务器已启动
        running = true;
        serverStarted = true;

        LOGGER_INFO("HTTP server thread started, listening on " +
                                    config.host + ":" + std::to_string(config.port));

        // 这里会阻塞直到服务器停止
        bool success = server.listen(config.host.c_str(), config.port);

        if (!success) {
            LOGGER_ERROR("HTTP server listen returned false");
        }

        running = false;
        LOGGER_INFO("HTTP server thread ended");
    });

    // 等待服务器启动
    int waitCount = 0;
    while (!serverStarted && waitCount < 50) { // 最多等待5秒
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitCount++;
    }

    if (serverStarted) {
        LOGGER_INFO("HTTP server successfully started");
        return true;
    } else {
        LOGGER_ERROR("HTTP server failed to start within timeout");
        if (serverThread && serverThread->joinable()) {
            server.stop();
            serverThread->join();
        }
        return false;
    }

//    // 启动服务器（非阻塞方式）
//    if (!server.listen(config.host.c_str(), config.port)) {
//        Logger::get_instance().error("Failed to start server");
//        return false;
//    }
//
//    running = true;
//    return true;
}

void HttpServer::stop() {
    if (!running) {
        return;
    }

    LOGGER_INFO("Stopping server");
    server.stop();

    if (serverThread && serverThread->joinable()) {
        serverThread->join();
    }

    running = false;
}

bool HttpServer::isRunning() const {
    return running;
}

const HTTPServerConfig& HttpServer::getConfig() const {
    return config;
}

const std::vector<HttpServer::RouteInfo>& HttpServer::getRoutes() const {
    return routes;
}

void HttpServer::wait() {
    if (serverThread && serverThread->joinable()) {
        serverThread->join();
    }
}
