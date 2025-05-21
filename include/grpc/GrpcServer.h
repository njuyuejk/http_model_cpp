// include/grpc/GrpcServer.h
#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <grpcpp/grpcpp.h>

class GrpcServer {
public:
    explicit GrpcServer(const std::string& server_address);
    ~GrpcServer();

    // 向服务器注册gRPC服务
    template <typename ServiceImpl>
    bool registerService(std::shared_ptr<ServiceImpl> service) {
        std::lock_guard<std::mutex> lock(server_mutex_);

        if (running_) {
            return false; // 服务器运行时不能注册服务
        }

        // 存储服务
        services_.push_back(service);

        // 向构建器注册服务
        builder_.RegisterService(service.get());

        return true;
    }

    // 启动gRPC服务器
    bool start();

    // 停止gRPC服务器
    void stop();

    // 检查服务器是否运行中
    bool isRunning() const;

private:
    std::string server_address_;
    std::unique_ptr<grpc::Server> server_;
    grpc::ServerBuilder builder_;
    bool running_;

    // 服务容器（使用void进行类型擦除）
    std::vector<std::shared_ptr<void>> services_;

    // 用于线程安全的互斥锁
    mutable std::mutex server_mutex_;
};

#endif // GRPC_SERVER_H