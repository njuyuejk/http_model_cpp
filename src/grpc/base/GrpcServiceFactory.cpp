//
// Created by YJK on 2025/5/21.
//

#include "grpc/base/GrpcServiceFactory.h"
#include "app/ApplicationManager.h"
#include "grpc/impl/aiModel/AIModelServiceInitializer.h"
#include "grpc/impl/status/StatusServiceInitializer.h"
#include "common/Logger.h"

// 在此处添加其他服务初始化器头文件

void GrpcServiceFactory::initializeAllServices(GrpcServiceRegistry& registry, ApplicationManager& appManager) {
    LOGGER_INFO("Initializing all gRPC services...");

    // 注册AI模型服务
    registry.registerInitializer(std::make_unique<AIModelServiceInitializer>(appManager));

    // 注册状态监控服务
    registry.registerInitializer(std::make_unique<StatusServiceInitializer>(appManager));

    // 在此处注册其他服务
    // 示例: registry.registerInitializer(std::make_unique<SystemMonitorServiceInitializer>());

    LOGGER_INFO("Successfully initialized " + std::to_string(registry.getServiceCount()) + " gRPC services");
}
