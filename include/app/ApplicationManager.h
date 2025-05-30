//
// Created by YJK on 2025/5/15.
//

#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include "common/Logger.h"
#include "common/StreamConfig.h"
#include "AIService/ModelPool.h"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <condition_variable>

// Forward declarations
class GrpcServer;
class GrpcServiceInitializerBase;

/**
 * @brief 并发配置结构体
 */
struct ConcurrencyConfig {
    int maxConcurrentRequests = 10;
    int modelPoolSize = 3;
    int requestTimeoutMs = 30000;
    int modelAcquireTimeoutMs = 5000;
    bool enableConcurrencyMonitoring = true;
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

    // gRPC服务初始化器集合
    std::vector<std::unique_ptr<GrpcServiceInitializerBase>> grpcServiceInitializers;

    // 模型池管理
    std::unordered_map<int, std::unique_ptr<ModelPool>> modelPools_;
    mutable std::shared_mutex modelPoolsMutex_;

    // 并发监控
    std::unique_ptr<ConcurrencyMonitor> grpcMonitor_;
    ConcurrencyConfig concurrencyConfig_;

    // 初始化方法
    bool initializeGrpcServer();

    // 记录初始化摘要 - 添加这一行
    void logInitializationSummary();

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
     * @brief 初始化模型池
     * @return 初始化是否成功
     */
    bool initializeModelPools();

    // 获取格式化为host:port的gRPC服务器地址
    std::string getGrpcServerAddress() const;

    // 获取gRPC服务器实例
    GrpcServer* getGrpcServer() const;

    // 模型池访问方法

    /**
     * @brief 使用模型池执行推理
     * @param modelType 模型类型
     * @param imageData 图像数据
     * @param results 检测结果
     * @param plateResults 车牌结果
     * @param timeoutMs 超时时间
     * @return 执行是否成功
     */
    bool executeModelInference(int modelType,
                               const cv::Mat& imageData,
                               std::vector<std::vector<std::any>>& results,
                               std::vector<std::string>& plateResults,
                               double startValue,
                               double endValue,
                               double& targetResult,
                               int timeoutMs = 0);

    /**
     * @brief 设置模型池状态
     * @param modelType 模型类型
     * @param enabled 是否启用
     * @return 设置是否成功
     */
    bool setModelEnabled(int modelType, bool enabled);

    /**
     * @brief 检查模型池状态
     * @param modelType 模型类型
     * @return 模型是否启用
     */
    bool isModelEnabled(int modelType) const;

    /**
     * @brief 获取模型池状态
     * @param modelType 模型类型
     * @return 模型池状态
     */
    ModelPool::PoolStatus getModelPoolStatus(int modelType) const;

    /**
     * @brief 获取所有模型池状态
     * @return 所有模型池状态映射
     */
    std::unordered_map<int, ModelPool::PoolStatus> getAllModelPoolStatus() const;

    // 并发监控方法

    /**
     * @brief 获取gRPC并发统计
     */
    ConcurrencyMonitor::Stats getGrpcConcurrencyStats() const;

    /**
     * @brief 开始gRPC请求监控
     */
    void startGrpcRequest();

    /**
     * @brief 完成gRPC请求监控
     */
    void completeGrpcRequest();

    /**
     * @brief gRPC请求失败监控
     */
    void failGrpcRequest();

    /**
     * @brief 获取并发配置
     */
    const ConcurrencyConfig& getConcurrencyConfig() const;

    // gRPC服务注册方法
    void registerGrpcServiceInitializer(std::unique_ptr<GrpcServiceInitializerBase> initializer);
    bool initializeGrpcServices();
    bool registerGrpcServicesFromRegistry();
};

#endif // APPLICATION_MANAGER_H