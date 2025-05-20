//
// Created by YJK on 2025/5/20.
//

#include "grpc/GrpcClient.h"
#include "common/Logger.h"

GrpcClient::GrpcClient(const std::string& server_address) {
    // 创建通道和存根
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    stub_ = grpc_service::AIModelService::NewStub(channel);

    Logger::info("gRPC客户端已初始化，服务器地址: " + server_address);
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
    Logger::info("发送gRPC ProcessImage请求，model_type=" + std::to_string(model_type));
    grpc::Status status = stub_->ProcessImage(&context, request, &response);

    if (!status.ok()) {
        error_message = status.error_message();
        Logger::error("gRPC ProcessImage失败: " + error_message);
        return false;
    }

    if (!response.success()) {
        error_message = response.message();
        Logger::error("ProcessImage报告失败: " + error_message);
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

    Logger::info("gRPC ProcessImage成功完成");
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
    Logger::info("发送gRPC ControlModel请求，model_type=" +
                 std::to_string(model_type) + ", enable=" + (enable ? "true" : "false"));
    grpc::Status status = stub_->ControlModel(&context, request, &response);

    if (!status.ok()) {
        error_message = status.error_message();
        Logger::error("gRPC ControlModel失败: " + error_message);
        return false;
    }

    if (!response.success()) {
        error_message = "控制模型失败";
        Logger::error("ControlModel报告失败");
        return false;
    }

    current_status = response.enabled();
    Logger::info("gRPC ControlModel成功完成");
    return true;
}
