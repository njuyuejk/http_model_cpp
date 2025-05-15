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
            Logger::get_instance().info("Registering route: GET " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "POST") {
            server.Post(route.pattern, route.handler);
            Logger::get_instance().info("Registering route: POST " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "PUT") {
            server.Put(route.pattern, route.handler);
            Logger::get_instance().info("Registering route: PUT " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "DELETE") {
            server.Delete(route.pattern, route.handler);
            Logger::get_instance().info("Registering route: DELETE " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        }
    }
}

bool HttpServer::start() {
    if (running) {
        Logger::get_instance().warning("Server is already running");
        return true;
    }

    // 在启动前注册所有路由
    registerRoutes();

    Logger::get_instance().info("Starting server " + config.host + ":" + std::to_string(config.port));

    // 设置超时（如果配置有指定）
    if (config.connectionTimeout > 0) {
        server.set_keep_alive_timeout(config.connectionTimeout);
    }

    if (config.readTimeout > 0) {
        server.set_read_timeout(config.readTimeout);
    }

    // 启动服务器（非阻塞方式）
    if (!server.listen(config.host.c_str(), config.port)) {
        Logger::get_instance().error("Failed to start server");
        return false;
    }

    running = true;
    return true;
}

void HttpServer::stop() {
    if (!running) {
        return;
    }

    Logger::get_instance().info("Stopping server");
    server.stop();
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