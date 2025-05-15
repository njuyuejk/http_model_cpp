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
        Logger::get_instance().warning("尝试添加空的路由组");
        return false;
    }

    const auto& name = group->getName();
    if (groupIndex.find(name) != groupIndex.end()) {
        Logger::get_instance().warning("路由组已存在: " + name);
        return false;
    }

    routeGroups.push_back(group);
    groupIndex[name] = group;

    Logger::get_instance().info("添加路由组: " + name + ", 基础路径: " + group->getBasePath());
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
    Logger::get_instance().info("正在配置所有路由组...");

    for (const auto& group : routeGroups) {
        Logger::get_instance().info("正在配置路由组: " + group->getName());
        group->registerRoutes(server);
    }

    Logger::get_instance().info("所有路由配置完成");
}