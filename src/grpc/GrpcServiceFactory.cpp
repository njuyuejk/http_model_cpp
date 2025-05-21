//
// Created by YJK on 2025/5/21.
//

#include "grpc/GrpcServiceFactory.h"
#include "app/ApplicationManager.h"
#include "grpc/AIModelServiceInitializer.h"
#include "common/Logger.h"

// 在此处添加其他服务初始化器头文件

void GrpcServiceFactory::initializeAllServices(GrpcServiceRegistry& registry, ApplicationManager& appManager) {
    Logger::info("初始化所有gRPC服务...");

    // 注册AI模型服务
    registry.registerInitializer(std::make_unique<AIModelServiceInitializer>(appManager));

    // 在此处注册其他服务
    // 示例: registry.registerInitializer(std::make_unique<SystemMonitorServiceInitializer>());

    Logger::info("已成功初始化 " + std::to_string(registry.getServiceCount()) + " 个gRPC服务");
}
