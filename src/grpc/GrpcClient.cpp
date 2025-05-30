//
// Created by YJK on 2025/5/20.
//

#include "grpc/GrpcClient.h"
#include "common/Logger.h"

GrpcClient::GrpcClient(const std::string& server_address) {
    try {
        // 创建通道
        auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());

        // 正确方式：直接使用 NewStub 返回的 unique_ptr
        stub_ = grpc_service::AIModelService::NewStub(channel);

        // 创建状态监控服务存根
        status_stub_ = grpc_service::StatusService::NewStub(channel);

        LOGGER_INFO("gRPC client initialized, server address: " + server_address);
    }
    catch (const std::exception& e) {
        LOGGER_ERROR("gRPC client initialization failed: " + std::string(e.what()));
        throw;
    }
}

bool GrpcClient::processImage(const std::string& base64_image,
                              int model_type,
                              std::vector<std::vector<float>>& detection_results,
                              std::vector<std::string>& plate_results,
                              std::string& error_message) {

    // 准备请求
    grpc_service::ImageRequest request;
    request.set_image_base64(base64_image);
    request.set_model_type(model_type);

    // 准备响应
    grpc_service::ImageResponse response;

    // 客户端上下文
    grpc::ClientContext context;

    // 调用RPC
    LOGGER_INFO("Sending gRPC ProcessImage request, model_type=" + std::to_string(model_type));
    grpc::Status status = stub_->ProcessImage(&context, request, &response);

    if (!status.ok()) {
        error_message = status.error_message();
        LOGGER_ERROR("gRPC ProcessImage failed: " + error_message);
        return false;
    }

    if (!response.success()) {
        error_message = response.message();
        LOGGER_ERROR("ProcessImage reported failure: " + error_message);
        return false;
    }

    // 提取结果
    detection_results.clear();
    for (const auto& detection : response.detection_results()) {
        std::vector<float> values;
        for (const auto& value : detection.values()) {
            values.push_back(value);
        }
        detection_results.push_back(values);
    }

    // 提取车牌结果
    plate_results.clear();
    for (const auto& plate : response.plate_results()) {
        plate_results.push_back(plate);
    }

    LOGGER_INFO("gRPC ProcessImage successfully completed");
    return true;
}

bool GrpcClient::controlModel(const std::string& model_name,
                              int model_type,
                              bool enable,
                              bool& current_status,
                              std::string& error_message) {

    // 准备请求
    grpc_service::ModelControlRequest request;
    request.set_model_name(model_name);
    request.set_model_type(model_type);
    request.set_enabled(enable);

    // 准备响应
    grpc_service::ModelControlResponse response;

    // 客户端上下文
    grpc::ClientContext context;

    // 调用RPC
    LOGGER_INFO("Sending gRPC ControlModel request, model_type=" +
                 std::to_string(model_type) + ", enable=" + (enable ? "true" : "false"));
    grpc::Status status = stub_->ControlModel(&context, request, &response);

    if (!status.ok()) {
        error_message = status.error_message();
        LOGGER_ERROR("gRPC ControlModel failed: " + error_message);
        return false;
    }

    if (!response.success()) {
        error_message = "Control model failed";
        LOGGER_ERROR("ControlModel reported failure");
        return false;
    }

    current_status = response.enabled();
    LOGGER_INFO("gRPC ControlModel successfully completed");
    return true;
}

bool GrpcClient::getSystemStatus(grpc_service::SystemStatusResponse &status_response, std::string &error_message) {

}

bool GrpcClient::getModelPoolsStatus(grpc_service::ModelPoolsStatusResponse &pools_response, std::string &error_message,
                                     int model_type) {

}

bool GrpcClient::getConcurrencyStats(grpc_service::ConcurrencyStatsResponse &stats_response,
                                     std::string &error_message) {

}
