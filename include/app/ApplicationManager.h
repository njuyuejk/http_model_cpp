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
#include "grpc/GrpcServer.h"

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

    // gRPC服务器实例
    std::unique_ptr<GrpcServer> grpcServer_;

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
     * @brief 初始化模型（未来扩展）
     * @return 初始化是否成功
     */
    bool initializeModels();

    // 初始化gRPC服务器
    bool initializeGrpcServer(const std::string& address);

    // 获取gRPC服务器引用
    GrpcServer* getGrpcServer() const;
};

#endif // APPLICATION_MANAGER_H
