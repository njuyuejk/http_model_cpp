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
        Logger::info("向注册表添加gRPC服务初始化器: " + initializer->getServiceName());
        initializers_.push_back(std::move(initializer));
    }
}

bool GrpcServiceRegistry::registerAllServices(ApplicationManager& appManager) {
    if (initializers_.empty()) {
        Logger::warning("没有gRPC服务初始化器可注册");
        return true;  // 没有服务也算成功，而不是失败
    }

    Logger::info("开始注册 " + std::to_string(initializers_.size()) + " 个gRPC服务");

    for (auto& initializer : initializers_) {
        if (initializer) {
            Logger::info("注册gRPC服务: " + initializer->getServiceName());
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
