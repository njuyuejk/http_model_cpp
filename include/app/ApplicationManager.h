//
// Created by YJK on 2025/5/15.
//

#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include "common/Logger.h"
#include "common/StreamConfig.h"
#include "AIService/rknnPool.h"
#include <string>
#include <memory>
#include <mutex>

// 前向声明
class GrpcServer;
class HttpServer;

struct SingleModelEntry{
    std::unique_ptr<rknn_lite> singleRKModel; // 单个模型
    std::string modelName; // 模型名称
    int modelType; // 模型类型
    bool isEnabled; //模型可用
};

/**
 * @brief 应用程序管理器单例类
 * 负责集中管理应用程序的初始化、配置和生命周期
 */
class ApplicationManager {
private:
    // 单例实例
    static ApplicationManager* instance;
    static std::mutex instanceMutex;

    // 私有构造函数，防止直接实例化
    ApplicationManager();

    // 初始化状态标志
    bool initialized;

    // 配置文件路径
    std::string configFilePath;

    // gRPC服务器
    std::unique_ptr<GrpcServer> grpcServer;

    // HTTP服务器
    std::unique_ptr<HttpServer> httpServer;

    // 初始化gRPC服务器
    bool initializeGrpcServer();

    // 初始化HTTP路由
    bool initializeRoutes();

    // 配置并启动HTTP服务器
    bool startHttpServer();

public:
    // AI处理部分
    std::vector<std::unique_ptr<SingleModelEntry>> singleModelPools_;

public:
    // 禁止拷贝和移动
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    ApplicationManager(ApplicationManager&&) = delete;
    ApplicationManager& operator=(ApplicationManager&&) = delete;

    // 析构函数
    ~ApplicationManager();

    // 获取单例实例
    static ApplicationManager& getInstance();

    /**
     * @brief 初始化应用程序
     * @param configPath 配置文件路径
     * @return 初始化是否成功
     */
    bool initialize(const std::string& configPath = "modelConfig.json");

    /**
     * @brief 关闭应用程序
     */
    void shutdown();

    /**
     * @brief 获取HTTP服务器配置
     * @return HTTP服务器配置引用
     */
    const HTTPServerConfig& getHTTPServerConfig() const;

    /**
     * @brief 初始化模型
     * @return 初始化是否成功
     */
    bool initializeModels();

    // 获取格式化为host:port的gRPC服务器地址
    std::string getGrpcServerAddress() const;

    // 获取HTTP服务器实例
    HttpServer* getHttpServer() const;

    // 获取gRPC服务器实例
    GrpcServer* getGrpcServer() const;

    // 返回特定类型模型的共享引用
    std::shared_ptr<rknn_lite> getSharedModelReference(int modelType) const;

    // 获取特定名称模型的共享引用
    std::shared_ptr<rknn_lite> getSharedModelByName(const std::string& modelName) const;

    // 返回所有模型的共享访问容器
    std::vector<std::shared_ptr<rknn_lite>> getAllSharedModels() const;

    // 获取特定类型模型的可用状态
    bool isModelEnabled(int modelType) const;

    // 设置特定类型模型的可用状态
    bool setModelEnabled(int modelType, bool enabled);
};

#endif // APPLICATION_MANAGER_H