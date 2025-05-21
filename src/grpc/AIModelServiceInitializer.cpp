//
// Created by YJK on 2025/5/21.
//

#include "grpc/AIModelServiceInitializer.h"
#include "grpc/GrpcServer.h"
#include "grpc/AIModelServiceImpl.h"
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
        Logger::error("无法初始化AI模型服务: gRPC服务器指针为空");
        return false;
    }

    try {
        // 创建服务实现实例
        serviceImpl_ = std::make_shared<AIModelServiceImpl>(appManager_);

        // 注册服务到gRPC服务器
        bool result = server->registerService(serviceImpl_);

        if (!result) {
            Logger::error("无法注册AI模型服务到gRPC服务器");
            serviceImpl_.reset();
            return false;
        }

        Logger::info("AI模型服务成功注册到gRPC服务器");
        return true;
    }
    catch (const std::exception& e) {
        Logger::error("初始化AI模型服务时发生异常: " + std::string(e.what()));
        serviceImpl_.reset();
        return false;
    }
}

std::string AIModelServiceInitializer::getServiceName() const {
    return "AIModelService";
}
