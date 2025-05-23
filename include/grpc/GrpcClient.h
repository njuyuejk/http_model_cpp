//
// Created by YJK on 2025/5/20.
//

#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>
#include "grpc/message/grpc_service.grpc.pb.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

class GrpcClient {
public:
    // 使用服务器地址初始化客户端
    explicit GrpcClient(const std::string& server_address);

    // 使用AI模型处理图像
    bool processImage(const std::string& base64_image,
                      int model_type,
                      std::vector<std::vector<float>>& detection_results,
                      std::vector<std::string>& plate_results,
                      std::string& error_message);

    // 控制模型状态（启用/禁用）
    bool controlModel(const std::string& model_name,
                      int model_type,
                      bool enable,
                      bool& current_status,
                      std::string& error_message);

    // 状态监控服务方法
    // 获取系统状态
    bool getSystemStatus(grpc_service::SystemStatusResponse& status_response,
                         std::string& error_message);

    // 获取模型池状态
    bool getModelPoolsStatus(grpc_service::ModelPoolsStatusResponse& pools_response,
                             std::string& error_message,
                             int model_type = -1);  // -1表示获取所有模型池

    // 获取并发统计
    bool getConcurrencyStats(grpc_service::ConcurrencyStatsResponse& stats_response,
                             std::string& error_message);

private:
    std::unique_ptr<grpc_service::AIModelService::Stub> stub_;
    std::unique_ptr<grpc_service::StatusService::Stub> status_stub_;
};

#endif // GRPC_CLIENT_H