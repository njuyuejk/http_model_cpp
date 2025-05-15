//
// Created by YJK on 2025/5/15.
//
#include "routeManager/RouteManager.h"

// 初始化静态成员
RouteManager* RouteManager::instance = nullptr;
std::mutex RouteManager::instanceMutex;

RouteManager& RouteManager::getInstance() {
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance == nullptr) {
            instance = new RouteManager();
        }
    }
    return *instance;
}

bool RouteManager::addGroup(std::shared_ptr<RouteGroup> group) {
    if (!group) {
        Logger::get_instance().warning("Attempt to add empty route group");
        return false;
    }

    const auto& name = group->getName();
    if (groupIndex.find(name) != groupIndex.end()) {
        Logger::get_instance().warning("Route group already exists: " + name);
        return false;
    }

    routeGroups.push_back(group);
    groupIndex[name] = group;

    Logger::get_instance().info("Added route group: " + name + ", base path: " + group->getBasePath());
    return true;
}

std::shared_ptr<RouteGroup> RouteManager::getGroup(const std::string& name) const {
    auto it = groupIndex.find(name);
    if (it != groupIndex.end()) {
        return it->second;
    }
    return nullptr;
}

void RouteManager::configureRoutes(HttpServer& server) {
    Logger::get_instance().info("Configuring all route groups...");

    for (const auto& group : routeGroups) {
        ExceptionHandler::execute("Configuring route group: " + group->getName(), [&]() {
            Logger::get_instance().info("Configuring route group: " + group->getName());
            group->registerRoutes(server);
        });
    }

    // 添加全局异常处理器
    server.setExceptionHandler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        try {
            if (ep) {
                std::rethrow_exception(ep);
            }
        } catch (const std::exception& e) {
            ExceptionHandler::setErrorResponse(res, e, &req);
        }
    });

    Logger::get_instance().info("All route configurations completed");
}