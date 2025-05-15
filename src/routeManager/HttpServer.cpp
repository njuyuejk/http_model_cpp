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
            Logger::get_instance().info("注册路由: GET " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "POST") {
            server.Post(route.pattern, route.handler);
            Logger::get_instance().info("注册路由: POST " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "PUT") {
            server.Put(route.pattern, route.handler);
            Logger::get_instance().info("注册路由: PUT " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        } else if (route.method == "DELETE") {
            server.Delete(route.pattern, route.handler);
            Logger::get_instance().info("注册路由: DELETE " + route.pattern +
                                        (route.description.empty() ? "" : " - " + route.description));
        }
    }
}

bool HttpServer::start() {
    if (running) {
        Logger::get_instance().warning("服务器已经在运行");
        return true;
    }

    // 在启动前注册所有路由
    registerRoutes();

    Logger::get_instance().info("正在启动服务器 " + config.host + ":" + std::to_string(config.port));

    // 设置超时（如果配置有指定）
    if (config.connectionTimeout > 0) {
        server.set_connection_timeout(config.connectionTimeout);
    }

    if (config.readTimeout > 0) {
        server.set_read_timeout(config.readTimeout);
    }

    // 启动服务器（非阻塞方式）
    if (!server.listen(config.host.c_str(), config.port)) {
        Logger::get_instance().error("启动服务器失败");
        return false;
    }

    running = true;
    return true;
}

void HttpServer::stop() {
    if (!running) {
        return;
    }

    Logger::get_instance().info("正在停止服务器");
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