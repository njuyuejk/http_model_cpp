//
// Created by YJK on 2025/5/23.
//

#include "grpc/impl/status/StatusServiceImpl.h"
#include "app/ApplicationManager.h"
#include "common/Logger.h"
#include "grpc/GrpcServer.h"
#include <chrono>
#include <thread>

StatusServiceImpl::StatusServiceImpl(ApplicationManager& appManager)
        : appManager_(appManager) {}

grpc::Status StatusServiceImpl::GetSystemStatus(
        grpc::ServerContext* context,
        const grpc_service::SystemStatusRequest* request,
        grpc_service::SystemStatusResponse* response) {

    auto requestId = std::this_thread::get_id();
    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        Logger::info("Received gRPC GetSystemStatus request, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        response->set_success(true);
        response->set_message("System status retrieved successfully");

        // 系统信息
        response->set_grpc_server_running(
                appManager_.getGrpcServer() != nullptr &&
                appManager_.getGrpcServer()->isRunning()
        );

        // 获取所有模型池状态
        auto allPoolStatus = appManager_.getAllModelPoolStatus();
        response->set_total_model_pools(allPoolStatus.size());

        // 并发配置
        const auto& concurrencyConfig = appManager_.getConcurrencyConfig();
        response->set_max_concurrent_requests(concurrencyConfig.maxConcurrentRequests);
        response->set_model_pool_size(concurrencyConfig.modelPoolSize);
        response->set_request_timeout_ms(concurrencyConfig.requestTimeoutMs);
        response->set_model_acquire_timeout_ms(concurrencyConfig.modelAcquireTimeoutMs);
        response->set_monitoring_enabled(concurrencyConfig.enableConcurrencyMonitoring);

        // gRPC统计
        auto grpcStats = appManager_.getGrpcConcurrencyStats();
        fillConcurrencyStats(grpcStats, response->mutable_grpc_stats());

        // 模型池信息
        for (const auto& pair : allPoolStatus) {
            auto* poolInfo = response->add_model_pools();
            fillModelPoolInfo(pair.first, pair.second, poolInfo);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        Logger::info("gRPC GetSystemStatus completed successfully, time: " +
                     std::to_string(duration.count()) + "ms, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        return grpc::Status::OK;

    } catch (const std::exception& e) {
        Logger::error("gRPC GetSystemStatus error: " + std::string(e.what()) +
                      ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
        response->set_success(false);
        response->set_message("Internal error: " + std::string(e.what()));
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status StatusServiceImpl::GetModelPoolsStatus(
        grpc::ServerContext* context,
        const grpc_service::ModelPoolsStatusRequest* request,
        grpc_service::ModelPoolsStatusResponse* response) {

    auto requestId = std::this_thread::get_id();

    try {
        Logger::info("Received gRPC GetModelPoolsStatus request, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        response->set_success(true);
        response->set_message("Model pools status retrieved successfully");

        // 检查是否请求特定模型类型
        if (request->has_model_type()) {
            int modelType = request->model_type();
            auto poolStatus = appManager_.getModelPoolStatus(modelType);

            // 如果模型池存在（totalModels > 0）
            if (poolStatus.totalModels > 0) {
                auto* poolInfo = response->add_model_pools();
                fillModelPoolInfo(modelType, poolStatus, poolInfo);
            } else {
                response->set_success(false);
                response->set_message("Model pool not found for type: " + std::to_string(modelType));
                return grpc::Status::OK;
            }
        } else {
            // 返回所有模型池状态
            auto allPoolStatus = appManager_.getAllModelPoolStatus();
            for (const auto& pair : allPoolStatus) {
                auto* poolInfo = response->add_model_pools();
                fillModelPoolInfo(pair.first, pair.second, poolInfo);
            }
        }

        Logger::info("gRPC GetModelPoolsStatus completed successfully, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        return grpc::Status::OK;

    } catch (const std::exception& e) {
        Logger::error("gRPC GetModelPoolsStatus error: " + std::string(e.what()) +
                      ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
        response->set_success(false);
        response->set_message("Internal error: " + std::string(e.what()));
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status StatusServiceImpl::GetConcurrencyStats(
        grpc::ServerContext* context,
        const grpc_service::ConcurrencyStatsRequest* request,
        grpc_service::ConcurrencyStatsResponse* response) {

    auto requestId = std::this_thread::get_id();

    try {
        Logger::info("Received gRPC GetConcurrencyStats request, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        response->set_success(true);
        response->set_message("Concurrency statistics retrieved successfully");
        response->set_timestamp(std::time(nullptr));

        // 获取统计数据
        auto grpcStats = appManager_.getGrpcConcurrencyStats();

        // 填充gRPC统计
        fillConcurrencyStats(grpcStats, response->mutable_grpc_stats());

        // 填充综合统计
        response->set_total_active(grpcStats.active);
        response->set_total_processed(grpcStats.total);
        response->set_total_failed(grpcStats.failed);

        double overallFailureRate = 0.0;
        if ((grpcStats.total) > 0) {
            overallFailureRate = static_cast<double>(grpcStats.failed) /
                                 (grpcStats.total);
        }
        response->set_overall_failure_rate(overallFailureRate);

        Logger::info("gRPC GetConcurrencyStats completed successfully, thread: " +
                     std::to_string(std::hash<std::thread::id>{}(requestId)));

        return grpc::Status::OK;

    } catch (const std::exception& e) {
        Logger::error("gRPC GetConcurrencyStats error: " + std::string(e.what()) +
                      ", thread: " + std::to_string(std::hash<std::thread::id>{}(requestId)));
        response->set_success(false);
        response->set_message("Internal error: " + std::string(e.what()));
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

void StatusServiceImpl::fillConcurrencyStats(const ConcurrencyMonitor::Stats& stats,
                                             grpc_service::ConcurrencyStats* grpcStats) {
    if (!grpcStats) return;

    grpcStats->set_active_requests(stats.active);
    grpcStats->set_total_requests(stats.total);
    grpcStats->set_failed_requests(stats.failed);
    grpcStats->set_success_requests(stats.total - stats.failed);
    grpcStats->set_failure_rate(stats.failureRate);
    grpcStats->set_success_rate(1.0 - stats.failureRate);
}

void StatusServiceImpl::fillModelPoolInfo(int modelType,
                                          const ModelPool::PoolStatus& poolStatus,
                                          grpc_service::ModelPoolInfo* poolInfo) {
    if (!poolInfo) return;

    poolInfo->set_model_type(modelType);
    poolInfo->set_enabled(poolStatus.isEnabled);
    poolInfo->set_total_models(poolStatus.totalModels);
    poolInfo->set_available_models(poolStatus.availableModels);
    poolInfo->set_busy_models(poolStatus.busyModels);
    poolInfo->set_model_path(poolStatus.modelPath);
    poolInfo->set_threshold(poolStatus.threshold);

    // 计算利用率和可用率
    double utilizationRate = 0.0;
    double availabilityRate = 0.0;

    if (poolStatus.totalModels > 0) {
        utilizationRate = static_cast<double>(poolStatus.busyModels) / poolStatus.totalModels;
        availabilityRate = static_cast<double>(poolStatus.availableModels) / poolStatus.totalModels;
    }

    poolInfo->set_utilization_rate(utilizationRate);
    poolInfo->set_availability_rate(availabilityRate);
}