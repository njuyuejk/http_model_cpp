//
// Created by YJK on 2025/5/15.
//

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "httplib.h"
#include "common/StreamConfig.h"
#include "common/Logger.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief HTTP服务器类
 * 负责管理HTTP服务器的路由设置、启动和关闭
 */
class HttpServer {
private:
    // HTTP服务器实例
    httplib::Server server;

    // 服务器配置
    HTTPServerConfig config;

    // 记录服务器是否正在运行
    bool running;

    // 路由信息结构体（用于更灵活的路由管理）
    struct RouteInfo {
        std::string method;      // HTTP方法：GET, POST等
        std::string pattern;     // URL模式
        std::string description; // 路由描述
        httplib::Server::Handler handler; // 处理函数

        RouteInfo(const std::string& m, const std::string& p,
                  const std::string& d, httplib::Server::Handler h)
                : method(m), pattern(p), description(d), handler(h) {}
    };

    // 存储所有路由信息
    std::vector<RouteInfo> routes;

public:
    /**
     * @brief 构造函数
     * @param serverConfig HTTP服务器配置
     */
    explicit HttpServer(const HTTPServerConfig& serverConfig);

    /**
     * @brief 析构函数
     */
    ~HttpServer();

    /**
     * @brief 添加GET路由
     * @param pattern URL模式
     * @param handler 处理函数
     * @param description 路由描述
     * @return 当前server实例的引用，用于链式调用
     */
    HttpServer& addGet(const std::string& pattern,
                       httplib::Server::Handler handler,
                       const std::string& description = "");

    /**
     * @brief 添加POST路由
     * @param pattern URL模式
     * @param handler 处理函数
     * @param description 路由描述
     * @return 当前server实例的引用，用于链式调用
     */
    HttpServer& addPost(const std::string& pattern,
                        httplib::Server::Handler handler,
                        const std::string& description = "");

    /**
     * @brief 添加PUT路由
     * @param pattern URL模式
     * @param handler 处理函数
     * @param description 路由描述
     * @return 当前server实例的引用，用于链式调用
     */
    HttpServer& addPut(const std::string& pattern,
                       httplib::Server::Handler handler,
                       const std::string& description = "");

    /**
     * @brief 添加DELETE路由
     * @param pattern URL模式
     * @param handler 处理函数
     * @param description 路由描述
     * @return 当前server实例的引用，用于链式调用
     */
    HttpServer& addDelete(const std::string& pattern,
                          httplib::Server::Handler handler,
                          const std::string& description = "");

    /**
     * @brief 设置错误处理器
     * @param handler 错误处理函数
     * @return 当前server实例的引用，用于链式调用
     */
    HttpServer& setErrorHandler(httplib::Server::Handler handler);

    /**
     * @brief 设置异常处理器
     * @param handler 异常处理函数
     * @return 当前server实例的引用，用于链式调用
     */
    HttpServer& setExceptionHandler(httplib::Server::ExceptionHandler handler);

    /**
     * @brief 注册所有路由
     * 将注册所有添加的路由到服务器
     */
    void registerRoutes();

    /**
     * @brief 启动服务器
     * @return 启动是否成功
     */
    bool start();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 服务器是否在运行
     * @return 运行状态
     */
    bool isRunning() const;

    /**
     * @brief 获取服务器配置
     * @return 服务器配置
     */
    const HTTPServerConfig& getConfig() const;

    /**
     * @brief 获取所有路由信息
     * @return 路由信息列表
     */
    const std::vector<RouteInfo>& getRoutes() const;
};

#endif // HTTP_SERVER_H
