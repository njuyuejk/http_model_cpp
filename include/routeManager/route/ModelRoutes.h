//
// Created by YJK on 2025/5/16.
//

#ifndef HTTP_MODEL_MODELROUTES_H
#define HTTP_MODEL_MODELROUTES_H

#include "routeManager/RouteManager.h"
#include "handlers/api_handler.h"

/**
 * @brief 模型相关路由组
 * 包含模型管理相关接口
 */
class ModelRoutes : public BaseRouteGroup {
public:
    ModelRoutes() : BaseRouteGroup("model", "/api/model", "模型相关接口") {}

    void registerRoutes(HttpServer& server) override {
        // 模型列表接口
        server.addGet("/api/model/inference", Handlers::handle_api_model_process, "模型推理")
                .addPost("/api/model/inference", Handlers::handle_api_model_process, "模型推理");

        // 这里可以添加更多模型相关接口
    }
};

#endif //HTTP_MODEL_MODELROUTES_H
