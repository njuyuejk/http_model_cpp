//
// Created by YJK on 2025/5/23.
//

#include "grpc/impl/status/StatusServiceInitializer.h"
#include "grpc/GrpcServer.h"
#include "grpc/impl/status/StatusServiceImpl.h"
#include "common/Logger.h"

StatusServiceInitializer::StatusServiceInitializer(ApplicationManager& appManager)
        : appManager_(appManager), serviceImpl_(nullptr) {
    // 构造函数实现
}

StatusServiceInitializer::~StatusServiceInitializer() {
    // 析构函数实现
    // 服务实例由shared_ptr管理，无需额外清理
}

bool StatusServiceInitializer::initialize(GrpcServer* server) {
    if (!server) {
        LOGGER_ERROR("Cannot initialize status service: gRPC server pointer is null");
        return false;
    }

    try {
        // 创建服务实现实例
        serviceImpl_ = std::make_shared<StatusServiceImpl>(appManager_);

        // 注册服务到gRPC服务器
        bool result = server->registerService(serviceImpl_);

        if (!result) {
            LOGGER_ERROR("Unable to register status service to gRPC server");
            serviceImpl_.reset();
            return false;
        }

        LOGGER_INFO("Status service successfully registered to gRPC server");
        return true;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR("Exception occurred while initializing status service: " + std::string(e.what()));
        serviceImpl_.reset();
        return false;
    }
}

std::string StatusServiceInitializer::getServiceName() const {
    return "StatusService";
}