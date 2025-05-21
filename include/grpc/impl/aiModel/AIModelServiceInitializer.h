//
// Created by YJK on 2025/5/21.
//

#ifndef HTTP_MODEL_AIMODELSERVICEINITIALIZER_H
#define HTTP_MODEL_AIMODELSERVICEINITIALIZER_H

#include "grpc/base/GrpcServiceInitializerBase.h"
#include "app/ApplicationManager.h"

// 前向声明减少头文件依赖
class GrpcServer;
class AIModelServiceImpl;

/**
 * @brief AI模型服务初始化器
 * 负责创建并向gRPC服务器注册AI模型服务
 */
class AIModelServiceInitializer : public GrpcServiceInitializerBase {
public:
    /**
     * @brief 构造函数
     * @param appManager 应用程序管理器引用
     */
    explicit AIModelServiceInitializer(ApplicationManager& appManager);

    /**
     * @brief 析构函数
     */
    ~AIModelServiceInitializer() override;

    /**
     * @brief 初始化并注册AI模型服务到gRPC服务器
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
    std::shared_ptr<AIModelServiceImpl> serviceImpl_;
};

#endif //HTTP_MODEL_AIMODELSERVICEINITIALIZER_H
