//
// Created by YJK on 2025/5/20.
//

#include "grpc/GrpcServer.h"
#include "common/Logger.h"

// GrpcServer的实现
GrpcServer::GrpcServer(const std::string& server_address, ApplicationManager& appManager)
        : server_address_(server_address), running_(false), appManager_(appManager) {
    service_ = std::make_unique<AIModelServiceImpl>(appManager);
}

GrpcServer::~GrpcServer() {
    if (running_) {
        stop();
    }
}

bool GrpcServer::start() {
    if (running_) {
        Logger::warning("gRPC服务器已在运行");
        return true;
    }

    grpc::ServerBuilder builder;
    // 在给定地址上监听，无认证机制
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    // 注册服务
    builder.RegisterService(service_.get());

    // 启动服务器
    server_ = builder.BuildAndStart();
    if (!server_) {
        Logger::error("启动gRPC服务器失败");
        return false;
    }

    running_ = true;
    Logger::info("gRPC服务器已在 " + server_address_ + " 上启动");

    return true;
}

void GrpcServer::stop() {
    if (!running_) {
        return;
    }

    Logger::info("正在停止gRPC服务器");
    server_->Shutdown();
    running_ = false;
}

bool GrpcServer::isRunning() const {
    return running_;
}