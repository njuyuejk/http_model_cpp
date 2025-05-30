//
// Created by YJK on 2025/5/21.
//

#include "grpc/base/GrpcServiceRegistry.h"
#include "app/ApplicationManager.h"
#include "common/Logger.h"

// 初始化静态成员
GrpcServiceRegistry* GrpcServiceRegistry::instance_ = nullptr;
std::mutex GrpcServiceRegistry::instanceMutex_;

GrpcServiceRegistry& GrpcServiceRegistry::getInstance() {
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        if (instance_ == nullptr) {
            instance_ = new GrpcServiceRegistry();
        }
    }
    return *instance_;
}

void GrpcServiceRegistry::registerInitializer(std::unique_ptr<GrpcServiceInitializerBase> initializer) {
    if (initializer) {
        LOGGER_INFO("Adding gRPC service initializer to registry: " + initializer->getServiceName());
        initializers_.push_back(std::move(initializer));
    }
}

bool GrpcServiceRegistry::registerAllServices(ApplicationManager& appManager) {
    if (initializers_.empty()) {
        LOGGER_WARNING("No gRPC service initializers to register");
        return true;  // 没有服务也算成功，而不是失败
    }

    LOGGER_INFO("Starting registration of " + std::to_string(initializers_.size()) + " gRPC services");

    for (auto& initializer : initializers_) {
        if (initializer) {
            LOGGER_INFO("Registering gRPC service: " + initializer->getServiceName());
            appManager.registerGrpcServiceInitializer(std::move(initializer));
        }
    }

    // 清空已注册的初始化器（因为所有权已转移）
    initializers_.clear();

    return true;
}

size_t GrpcServiceRegistry::getServiceCount() const {
    return initializers_.size();
}

void GrpcServiceRegistry::clear() {
    initializers_.clear();
}
