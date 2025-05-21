//
// Created by YJK on 2025/5/21.
//

#ifndef GRPC_SERVICE_FACTORY_H
#define GRPC_SERVICE_FACTORY_H

#include "GrpcServiceRegistry.h"

class ApplicationManager;

/**
 * @brief gRPC服务工厂类
 * 负责创建并注册所有gRPC服务
 */
class GrpcServiceFactory {
public:
    /**
     * @brief 初始化并注册所有标准服务
     * @param registry 服务注册表引用
     * @param appManager ApplicationManager引用（用于创建需要应用程序上下文的服务）
     */
    static void initializeAllServices(GrpcServiceRegistry& registry, ApplicationManager& appManager);

private:
    // 禁止实例化（纯静态类）
    GrpcServiceFactory() = delete;
};

#endif // GRPC_SERVICE_FACTORY_H