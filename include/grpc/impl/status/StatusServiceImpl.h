//
// Created by YJK on 2025/5/23.
//

#ifndef STATUS_SERVICE_IMPL_H
#define STATUS_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>
#include "grpc/message/grpc_service.grpc.pb.h"
#include "AIService/ModelPool.h"

// Forward declaration
class ApplicationManager;

/**
 * @brief 状态监控服务实现类
 * 提供系统状态、模型池状态和并发统计等监控功能
 */
class StatusServiceImpl final : public grpc_service::StatusService::Service {
public:
    explicit StatusServiceImpl(ApplicationManager& appManager);

    /**
     * @brief 获取系统整体状态
     * @param context gRPC上下文
     * @param request 系统状态请求
     * @param response 系统状态响应
     * @return gRPC状态
     */
    grpc::Status GetSystemStatus(grpc::ServerContext* context,
                                 const grpc_service::SystemStatusRequest* request,
                                 grpc_service::SystemStatusResponse* response) override;

    /**
     * @brief 获取模型池状态
     * @param context gRPC上下文
     * @param request 模型池状态请求
     * @param response 模型池状态响应
     * @return gRPC状态
     */
    grpc::Status GetModelPoolsStatus(grpc::ServerContext* context,
                                     const grpc_service::ModelPoolsStatusRequest* request,
                                     grpc_service::ModelPoolsStatusResponse* response) override;

    /**
     * @brief 获取并发统计信息
     * @param context gRPC上下文
     * @param request 并发统计请求
     * @param response 并发统计响应
     * @return gRPC状态
     */
    grpc::Status GetConcurrencyStats(grpc::ServerContext* context,
                                     const grpc_service::ConcurrencyStatsRequest* request,
                                     grpc_service::ConcurrencyStatsResponse* response) override;

private:
    ApplicationManager& appManager_;

    /**
     * @brief 辅助方法：填充并发统计信息
     * @param stats 源统计数据
     * @param grpcStats 目标gRPC消息
     */
    void fillConcurrencyStats(const ConcurrencyMonitor::Stats& stats,
                              grpc_service::ConcurrencyStats* grpcStats);

    /**
     * @brief 辅助方法：填充模型池信息
     * @param modelType 模型类型
     * @param poolStatus 模型池状态
     * @param poolInfo 目标gRPC消息
     */
    void fillModelPoolInfo(int modelType,
                           const ModelPool::PoolStatus& poolStatus,
                           grpc_service::ModelPoolInfo* poolInfo);
};

#endif // STATUS_SERVICE_IMPL_H