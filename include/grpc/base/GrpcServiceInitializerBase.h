//
// Created by YJK on 2025/5/21.
//

#ifndef HTTP_MODEL_GRPCSERVICEINITIALIZERBASE_H
#define HTTP_MODEL_GRPCSERVICEINITIALIZERBASE_H

#include <memory>
#include <string>
#include "grpc/GrpcServer.h"

class GrpcServiceInitializerBase {
public:
    virtual ~GrpcServiceInitializerBase() = default;

    // 初始化并注册服务到gRPC服务器
    virtual bool initialize(GrpcServer* server) = 0;

    // 获取服务名称（用于日志和调试）
    virtual std::string getServiceName() const = 0;
};

#endif //HTTP_MODEL_GRPCSERVICEINITIALIZERBASE_H
