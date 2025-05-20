//
// Created by YJK on 2025/5/20.
//

#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "grpc_service.pb.h"
#include "app/ApplicationManager.h"

class AIModelServiceImpl final : public grpc_service::AIModelService::Service {
public:
    explicit AIModelServiceImpl(ApplicationManager& appManager);

    grpc::Status ProcessImage(grpc::ServerContext* context,
                              const grpc_service::ImageRequest* request,
                              grpc_service::ImageResponse* response) override;

    grpc::Status ControlModel(grpc::ServerContext* context,
                              const grpc_service::ModelControlRequest* request,
                              grpc_service::ModelControlResponse* response) override;

private:
    ApplicationManager& appManager_;
};

class GrpcServer {
public:
    GrpcServer(const std::string& server_address, ApplicationManager& appManager);
    ~GrpcServer();

    // 启动gRPC服务器
    bool start();

    // 停止gRPC服务器
    void stop();

    // 检查服务器是否运行中
    bool isRunning() const;

private:
    std::string server_address_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<AIModelServiceImpl> service_;
    bool running_;
    ApplicationManager& appManager_;
};

#endif // GRPC_SERVER_H