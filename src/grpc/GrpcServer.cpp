// src/grpc/GrpcServer.cpp
#include "grpc/GrpcServer.h"
#include "common/Logger.h"

GrpcServer::GrpcServer(const std::string& server_address)
        : server_address_(server_address), running_(false) {
    // 在构造函数中设置服务器选项
    builder_.AddListeningPort(server_address_, grpc::InsecureServerCredentials());

    // 配置服务器设置
    builder_.SetMaxReceiveMessageSize(8 * 1024 * 1024); // 8MB最大接收消息大小
    builder_.SetMaxSendMessageSize(8 * 1024 * 1024);    // 8MB最大发送消息大小

    // 设置压缩选项
    builder_.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
}

GrpcServer::~GrpcServer() {
    if (running_) {
        stop();
    }
}

bool GrpcServer::start() {
    std::lock_guard<std::mutex> lock(server_mutex_);

    if (running_) {
        Logger::warning("gRPC服务器已在 " + server_address_ + " 上运行");
        return true;
    }

    // 验证至少注册了一个服务
    if (services_.empty()) {
        Logger::error("无法启动gRPC服务器：未注册服务");
        return false;
    }

    try {
        // 构建并启动服务器
        server_ = builder_.BuildAndStart();

        if (!server_) {
            Logger::error("在 " + server_address_ + " 上启动gRPC服务器失败");
            return false;
        }

        running_ = true;
        Logger::info("gRPC服务器在 " + server_address_ + " 上成功启动");
        return true;
    }
    catch (const std::exception& e) {
        Logger::error("启动gRPC服务器时发生异常: " + std::string(e.what()));
        return false;
    }
}

void GrpcServer::stop() {
    std::lock_guard<std::mutex> lock(server_mutex_);

    if (!running_) {
        return;
    }

    Logger::info("正在停止 " + server_address_ + " 上的gRPC服务器");

    // 设置超时关闭
    constexpr int SHUTDOWN_TIMEOUT_SECONDS = 5;

    auto deadline = std::chrono::system_clock::now() +
                    std::chrono::seconds(SHUTDOWN_TIMEOUT_SECONDS);

    server_->Shutdown(deadline);
    running_ = false;

    // 服务器停止时清理服务
    services_.clear();

    Logger::info("gRPC服务器已停止");
}

bool GrpcServer::isRunning() const {
    std::lock_guard<std::mutex> lock(server_mutex_);
    return running_;
}
