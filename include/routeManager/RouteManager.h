//
// Created by YJK on 2025/5/15.
//

#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include "HttpServer.h"
#include "common/Logger.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

/**
 * @brief API分组接口，用于组织API路由
 */
class RouteGroup {
public:
    virtual ~RouteGroup() = default;

    /**
     * @brief 注册该组的所有路由
     * @param server HTTP服务器实例
     */
    virtual void registerRoutes(HttpServer& server) = 0;

    /**
     * @brief 获取API组的名称
     * @return 组名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取API组的基础路径
     * @return 基础路径
     */
    virtual std::string getBasePath() const = 0;

    /**
     * @brief 获取API组的描述
     * @return 组描述
     */
    virtual std::string getDescription() const = 0;
};

/**
 * @brief 路由管理器类
 * 负责配置所有HTTP路由，支持模块化和可扩展的路由注册
 */
class RouteManager {
private:
    // 存储所有路由组
    std::vector<std::shared_ptr<RouteGroup>> routeGroups;

    // 存储路由组的索引（按名称）
    std::unordered_map<std::string, std::shared_ptr<RouteGroup>> groupIndex;

    // 单例实例
    static RouteManager* instance;
    static std::mutex instanceMutex;

    // 私有构造函数
    RouteManager() = default;

public:
    // 禁止拷贝和移动
    RouteManager(const RouteManager&) = delete;
    RouteManager& operator=(const RouteManager&) = delete;
    RouteManager(RouteManager&&) = delete;
    RouteManager& operator=(RouteManager&&) = delete;

    // 析构函数
    ~RouteManager() = default;

    /**
     * @brief 获取单例实例
     * @return RouteManager单例引用
     */
    static RouteManager& getInstance();

    /**
     * @brief 添加路由组
     * @param group 路由组指针
     * @return 是否成功添加
     */
    bool addGroup(std::shared_ptr<RouteGroup> group);

    /**
     * @brief 通过名称获取路由组
     * @param name 组名称
     * @return 路由组指针，如果不存在则返回nullptr
     */
    std::shared_ptr<RouteGroup> getGroup(const std::string& name) const;

    /**
     * @brief 配置所有路由
     * @param server HTTP服务器实例
     */
    void configureRoutes(HttpServer& server);
};

/**
 * @brief 基础API路由组实现
 * 可以被继承以创建特定功能的API路由组
 */
class BaseRouteGroup : public RouteGroup {
protected:
    std::string name;
    std::string basePath;
    std::string description;

public:
    /**
     * @brief 构造函数
     * @param groupName 组名称
     * @param groupBasePath 组基础路径
     * @param groupDescription 组描述
     */
    BaseRouteGroup(const std::string& groupName,
                   const std::string& groupBasePath,
                   const std::string& groupDescription)
            : name(groupName), basePath(groupBasePath), description(groupDescription) {}

    ~BaseRouteGroup() override = default;

    /**
     * @brief 获取组名称
     * @return 组名称
     */
    std::string getName() const override {
        return name;
    }

    /**
     * @brief 获取组基础路径
     * @return 基础路径
     */
    std::string getBasePath() const override {
        return basePath;
    }

    /**
     * @brief 获取组描述
     * @return 组描述
     */
    std::string getDescription() const override {
        return description;
    }
};

#endif // ROUTE_MANAGER_H