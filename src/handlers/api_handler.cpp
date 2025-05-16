//
// Created by YJK on 2025/1/2.
//

// src/handlers/api_handler.cpp - 已更新
#include "handlers/api_handler.h"
#include "nlohmann/json.hpp"
#include "exception/GlobalExceptionHandler.h"
#include "common/base64.h"
#include "opencv2/opencv.hpp"
#include "app/ApplicationManager.h"

using json = nlohmann::json;

void Handlers::handle_api_model_process(const httplib::Request& req, httplib::Response& res) {
    ExceptionHandler::handleRequest(req, res, [](const httplib::Request& req, httplib::Response& res) {
        // 检查内容类型
        if (!req.has_header("Content-Type") || req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
            throw APIException("Request must include 'application/json' Content-Type", 415);
        }

        // 解析 JSON 数据
        json received_json;
        try {
            received_json = json::parse(req.body);
        } catch (const json::exception& e) {
            throw JSONParseException(std::string("Invalid JSON format: ") + e.what());
        }

        // 验证必要字段
        if (!received_json.contains("img")) {
            throw APIException("Request must include 'message' field", 400);
        }

        if (!received_json.contains("modelType")) {
            throw APIException("Request must include 'message' field", 400);
        }

        std::string message = received_json["img"];
        int modelType = received_json["modelType"];

        std::vector<unsigned char> decoded_data = std::vector<unsigned char>(base64_decode(message).begin(), base64_decode(message).end());

        cv::Mat ori_img = cv::imdecode(decoded_data, cv::IMREAD_COLOR);

        std::vector<std::vector<std::any>> results_vector;
        for (auto &model: ApplicationManager::getInstance().singleModelPools_) {
            // 选择正确模型
            if (model->modelType != modelType) {
                continue;
            }

            // 模型是否开启
            if (!model->isEnabled) {
                break;
            }

            model->singleRKModel->ori_img = ori_img;
            model->singleRKModel->interf();
            cv::Mat dstMat = model->singleRKModel->ori_img;
            results_vector = model->singleRKModel->results_vector;
//            cv::imwrite("./test.jpg", dstMat);
        }


//        cv::imwrite("./test.jpg", ori_img);

        Logger::info("output ori_img width: "+ std::to_string(ori_img.cols));

        // 手动将嵌套向量转换为 JSON
        json json_data = json::array();
        for (const auto& inner_vec : results_vector) {
            json inner_json = json::array();
            for (const auto& item : inner_vec) {
                inner_json.push_back(any_to_json(item));
            }
            json_data.push_back(inner_json);
        }

        // 构建响应 JSON
        json response_json = {
                {"status", "success"},
                {"message", ori_img.cols},
                {"processed_message", ori_img.rows},
                {"detect_results", json_data},
                {"received", true}
        };

        // 发送响应
        res.set_content(response_json.dump(), "application/json");

        results_vector.clear();
        std::vector<std::vector<std::any>>().swap(results_vector);
    });
}