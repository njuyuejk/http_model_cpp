//
// Created by YJK on 2025/5/21.
//

#include "grpc/impl/aiModel/AIModelServiceInitializer.h"
#include "grpc/GrpcServer.h"
#include "grpc/impl/aiModel/AIModelServiceImpl.h"
#include "common/Logger.h"

AIModelServiceInitializer::AIModelServiceInitializer(ApplicationManager& appManager)
        : appManager_(appManager), serviceImpl_(nullptr) {
    // 构造函数实现
}

AIModelServiceInitializer::~AIModelServiceInitializer() {
    // 析构函数实现
    // 服务实例由shared_ptr管理，无需额外清理
}

bool AIModelServiceInitializer::initialize(GrpcServer* server) {
    if (!server) {
        LOGGER_ERROR("Cannot initialize AI model service: gRPC server pointer is null");
        return false;
    }

    try {
        // 创建服务实现实例
        serviceImpl_ = std::make_shared<AIModelServiceImpl>(appManager_);

        // 注册服务到gRPC服务器
        bool result = server->registerService(serviceImpl_);

        if (!result) {
            LOGGER_ERROR("Unable to register AI model service to gRPC server");
            serviceImpl_.reset();
            return false;
        }

        LOGGER_INFO("AI model service successfully registered to gRPC server");
        return true;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR("Exception occurred while initializing AI model service: " + std::string(e.what()));
        serviceImpl_.reset();
        return false;
    }
}

std::string AIModelServiceInitializer::getServiceName() const {
    return "AIModelService";
}
