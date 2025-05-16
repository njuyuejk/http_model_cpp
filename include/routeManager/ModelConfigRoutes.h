//
// Created by YJK on 2025/5/15.
//

#ifndef USER_ROUTES_H
#define USER_ROUTES_H

#include "RouteManager.h"
#include "handlers/modelConfig_handler.h"

/**
 * @brief 用户相关路由组
 * 包含用户管理相关接口
 */
class ModelConfigRoutes : public BaseRouteGroup {
public:
    ModelConfigRoutes() : BaseRouteGroup("model_config", "/api/model/model_config", "模型配置相关接口") {}

    void registerRoutes(HttpServer& server) override {
        // 用户信息接口
        server.addGet(R"(/api/model/model_config/(\w+))", Handlers::handle_model_config, "获取模型配置信息")
                .addPost(R"(/api/model/model_config/(\w+))", Handlers::handle_model_config, "获取模型配置信息");

        // 这里可以添加更多用户相关接口，例如：
        // server.addPost("/user/register", Handlers::handle_user_register, "用户注册");
        // server.addPost("/user/login", Handlers::handle_user_login, "用户登录");
        // server.addPut("/user/update", Handlers::handle_user_update, "更新用户信息");
        // server.addDelete("/user/delete", Handlers::handle_user_delete, "删除用户");
    }
};

#endif // USER_ROUTES_H