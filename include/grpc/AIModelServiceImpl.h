//
// Created by YJK on 2025/5/20.
//

#ifndef AI_MODEL_SERVICE_IMPL_H
#define AI_MODEL_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>
#include "grpc_service.pb.h"

// Forward declaration
class ApplicationManager;

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

#endif // AI_MODEL_SERVICE_IMPL_H