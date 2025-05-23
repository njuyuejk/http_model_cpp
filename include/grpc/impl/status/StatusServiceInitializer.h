//
// Created by YJK on 2025/5/23.
//

#ifndef STATUS_SERVICE_INITIALIZER_H
#define STATUS_SERVICE_INITIALIZER_H

#include "grpc/base/GrpcServiceInitializerBase.h"
#include "app/ApplicationManager.h"

// 前向声明减少头文件依赖
class GrpcServer;
class StatusServiceImpl;

/**
 * @brief 状态监控服务初始化器
 * 负责创建并向gRPC服务器注册状态监控服务
 */
class StatusServiceInitializer : public GrpcServiceInitializerBase {
public:
    /**
     * @brief 构造函数
     * @param appManager 应用程序管理器引用
     */
    explicit StatusServiceInitializer(ApplicationManager& appManager);

    /**
     * @brief 析构函数
     */
    ~StatusServiceInitializer() override;

    /**
     * @brief 初始化并注册状态监控服务到gRPC服务器
     * @param server gRPC服务器指针
     * @return 注册是否成功
     */
    bool initialize(GrpcServer* server) override;

    /**
     * @brief 获取服务名称
     * @return 服务名称字符串
     */
    std::string getServiceName() const override;

private:
    ApplicationManager& appManager_;
    std::shared_ptr<StatusServiceImpl> serviceImpl_;
};

#endif // STATUS_SERVICE_INITIALIZER_H