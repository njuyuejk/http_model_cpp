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
        LOGGER_WARNING("gRPC server is already running at " + server_address_);
        return true;
    }

    // 验证至少注册了一个服务
    if (services_.empty()) {
        LOGGER_ERROR("Unable to start gRPC server: no services registered");
        return false;
    }

    try {
        // 构建并启动服务器
        server_ = builder_.BuildAndStart();

        if (!server_) {
            LOGGER_ERROR("Failed to start gRPC server at " + server_address_);
            return false;
        }

        running_ = true;
        LOGGER_INFO("gRPC server successfully started at " + server_address_);
        return true;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR("Exception occurred while starting gRPC server: " + std::string(e.what()));
        return false;
    }
}

void GrpcServer::stop() {
    std::lock_guard<std::mutex> lock(server_mutex_);

    if (!running_) {
        return;
    }

    LOGGER_INFO("Stopping gRPC server at " + server_address_);

    // 设置超时关闭
    constexpr int SHUTDOWN_TIMEOUT_SECONDS = 5;

    auto deadline = std::chrono::system_clock::now() +
                    std::chrono::seconds(SHUTDOWN_TIMEOUT_SECONDS);

    server_->Shutdown(deadline);
    running_ = false;

    // 服务器停止时清理服务
    services_.clear();

    LOGGER_INFO("gRPC server stopped");
}

bool GrpcServer::isRunning() const {
    std::lock_guard<std::mutex> lock(server_mutex_);
    return running_;
}
