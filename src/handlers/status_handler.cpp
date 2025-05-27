//
// Created by YJK on 2025/5/22.
//

#include "handlers/status_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"
#include "app/ApplicationManager.h"

using json = nlohmann::json;

void Handlers::handle_system_status(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        auto& appManager = ApplicationManager::getInstance();

        // 获取所有模型池状态
        auto allPoolStatus = appManager.getAllModelPoolStatus();

        // 获取并发统计
        auto httpStats = appManager.getHttpConcurrencyStats();

        // 获取配置信息
        auto concurrencyConfig = appManager.getConcurrencyConfig();

        json response_json = {
                {"status", "success"},
                {"system_info", {
                                   {"http_server_running", appManager.getHttpServer() != nullptr},
                                   {"total_model_pools", allPoolStatus.size()}
                           }},
                {"concurrency_config", {
                                   {"max_concurrent_requests", concurrencyConfig.maxConcurrentRequests},
                                   {"model_pool_size", concurrencyConfig.modelPoolSize},
                                   {"request_timeout_ms", concurrencyConfig.requestTimeoutMs},
                                   {"model_acquire_timeout_ms", concurrencyConfig.modelAcquireTimeoutMs},
                                   {"monitoring_enabled", concurrencyConfig.enableConcurrencyMonitoring}
                           }},
                {"http_stats", {
                                   {"active_requests", httpStats.active},
                                   {"total_requests", httpStats.total},
                                   {"failed_requests", httpStats.failed},
                                   {"failure_rate", httpStats.failureRate}
                           }}
        };

        // 添加模型池摘要
        json poolsSummary = json::array();
        for (const auto& pair : allPoolStatus) {
            poolsSummary.push_back({
                                           {"model_type", pair.first},
                                           {"enabled", pair.second.isEnabled},
                                           {"total_models", pair.second.totalModels},
                                           {"available_models", pair.second.availableModels},
                                           {"busy_models", pair.second.busyModels}
                                   });
        }
        response_json["model_pools_summary"] = poolsSummary;

        res.set_content(response_json.dump(2), "application/json");
    });
}

void Handlers::handle_model_pools_status(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        auto& appManager = ApplicationManager::getInstance();

        // 获取所有模型池状态
        auto allPoolStatus = appManager.getAllModelPoolStatus();

        json response_json = {
                {"status", "success"},
                {"model_pools", json::object()}
        };

        for (const auto& pair : allPoolStatus) {
            int modelType = pair.first;
            const auto& status = pair.second;

            response_json["model_pools"][std::to_string(modelType)] = {
                    {"model_type", modelType},
                    {"enabled", status.isEnabled},
                    {"model_path", status.modelPath},
                    {"threshold", status.threshold},
                    {"pool_info", {
                                           {"total_models", status.totalModels},
                                           {"available_models", status.availableModels},
                                           {"busy_models", status.busyModels}
                                   }},
                    {"efficiency", {
                                           {"utilization_rate", status.totalModels > 0 ?
                                                                (double)status.busyModels / status.totalModels : 0.0},
                                           {"availability_rate", status.totalModels > 0 ?
                                                                 (double)status.availableModels / status.totalModels : 0.0}
                                   }}
            };
        }

        res.set_content(response_json.dump(2), "application/json");
    });
}

void Handlers::handle_concurrency_stats(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        auto& appManager = ApplicationManager::getInstance();

        auto httpStats = appManager.getHttpConcurrencyStats();

        json response_json = {
                {"status", "success"},
                {"timestamp", std::time(nullptr)},
                {"http_concurrency", {
                                   {"active_requests", httpStats.active},
                                   {"total_requests", httpStats.total},
                                   {"failed_requests", httpStats.failed},
                                   {"success_requests", httpStats.total - httpStats.failed},
                                   {"failure_rate", httpStats.failureRate},
                                   {"success_rate", 1.0 - httpStats.failureRate}
                           }},
                {"combined_stats", {
                                   {"total_active", httpStats.active},
                                   {"total_processed", httpStats.total},
                                   {"total_failed", httpStats.failed},
                                   {"overall_failure_rate", (httpStats.total) > 0 ?
                                                            (double)(httpStats.failed) / (httpStats.total) : 0.0}
                           }}
        };

        res.set_content(response_json.dump(2), "application/json");
    });
}
