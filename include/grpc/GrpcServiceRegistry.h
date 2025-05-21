//
// Created by YJK on 2025/5/21.
//

#ifndef GRPC_SERVICE_REGISTRY_H
#define GRPC_SERVICE_REGISTRY_H

#include <memory>
#include <vector>
#include <string>
#include "grpc/GrpcServiceInitializerBase.h"

class ApplicationManager;

/**
 * @brief gRPC服务注册表类
 * 集中管理所有gRPC服务初始化器，便于统一注册和初始化
 */
class GrpcServiceRegistry {
public:
    /**
     * @brief 获取单例实例
     * @return 注册表实例引用
     */
    static GrpcServiceRegistry& getInstance();

    /**
     * @brief 注册服务初始化器
     * @param initializer 服务初始化器
     */
    void registerInitializer(std::unique_ptr<GrpcServiceInitializerBase> initializer);

    /**
     * @brief 将所有注册的服务初始化器添加到ApplicationManager
     * @param appManager ApplicationManager引用
     * @return 注册是否成功
     */
    bool registerAllServices(ApplicationManager& appManager);

    /**
     * @brief 获取已注册的服务数量
     * @return 服务数量
     */
    size_t getServiceCount() const;

    /**
     * @brief 清空注册表
     */
    void clear();

private:
    // 私有构造函数（单例模式）
    GrpcServiceRegistry() = default;

    // 禁止拷贝和移动
    GrpcServiceRegistry(const GrpcServiceRegistry&) = delete;
    GrpcServiceRegistry& operator=(const GrpcServiceRegistry&) = delete;
    GrpcServiceRegistry(GrpcServiceRegistry&&) = delete;
    GrpcServiceRegistry& operator=(GrpcServiceRegistry&&) = delete;

    // 服务初始化器列表
    std::vector<std::unique_ptr<GrpcServiceInitializerBase>> initializers_;

    // 单例实例
    static GrpcServiceRegistry* instance_;
    static std::mutex instanceMutex_;
};

#endif // GRPC_SERVICE_REGISTRY_H