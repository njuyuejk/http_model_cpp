//
// Created by YJK on 2025/5/15.
//

#include "app/ApplicationManager.h"
#include "exception/GlobalExceptionHandler.h"
#include "grpc/GrpcServer.h"
#include "routeManager/HttpServer.h"
#include "routeManager/RouteManager.h"
#include "routeManager/RouteInitializer.h"
#include "grpc/GrpcServiceInitializerBase.h"
#include "grpc/GrpcServiceRegistry.h"
#include "grpc/GrpcServiceFactory.h"

// 初始化静态成员
ApplicationManager* ApplicationManager::instance = nullptr;
std::mutex ApplicationManager::instanceMutex;

ApplicationManager::ApplicationManager() : initialized(false), configFilePath("") {
    // 私有构造函数
}

ApplicationManager::~ApplicationManager() {
    if (initialized) {
        shutdown();
    }
}

ApplicationManager& ApplicationManager::getInstance() {
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance == nullptr) {
            instance = new ApplicationManager();
        }
    }
    return *instance;
}

bool ApplicationManager::initialize(const std::string& configPath) {
    if (initialized) {
        Logger::get_instance().warning("Application manager already initialized");
        return true;
    }

    configFilePath = configPath;

    // 初始化日志系统，使用默认值
    Logger::init();

    Logger::get_instance().info("Initializing application manager...");

    // 使用异常处理加载配置
    bool config_loaded = ExceptionHandler::execute("加载配置文件", [&]() {
        if (!AppConfig::loadFromFile(configFilePath)) {
            throw ConfigException("无法从以下路径加载配置: " + configFilePath);
        }
    });

    if (!config_loaded) {
        Logger::get_instance().error("Configuration loading failed, using default configuration");
    } else {
        Logger::get_instance().info("Configuration loaded successfully");
    }

    // 使用加载的设置或默认值重新初始化日志系统
    ExceptionHandler::execute("初始化日志系统", [&]() {
        Logger::init(
                AppConfig::getLogToFile(),
                AppConfig::getLogFilePath(),
                static_cast<LogLevel>(AppConfig::getLogLevel())
        );
    });

    // 初始化模型
    bool models_initialized = ExceptionHandler::execute("初始化模型", [&]() {
        if (!initializeModels()) {
            throw ConfigException("模型初始化失败");
        }
    });

    if (!models_initialized) {
        Logger::get_instance().warning("模型初始化失败，程序将继续运行...");
    }

    // 从注册表注册所有gRPC服务
    bool services_registered = registerGrpcServicesFromRegistry();
    if (!services_registered) {
        Logger::get_instance().warning("gRPC服务注册失败，程序将继续但gRPC功能可能不可用");
    }

    // 初始化gRPC服务器
    bool grpc_initialized = initializeGrpcServer();
    if (!grpc_initialized) {
        Logger::get_instance().warning("gRPC服务器初始化失败，程序将继续但不包含gRPC功能");
    }

    // 初始化路由
    bool routes_initialized = initializeRoutes();
    if (!routes_initialized) {
        Logger::get_instance().error("路由初始化失败");
        return false;
    }

    // 启动HTTP服务器
    bool http_started = startHttpServer();
    if (!http_started) {
        Logger::get_instance().error("HTTP服务器启动失败");
        return false;
    }

    initialized = true;
    Logger::get_instance().info("应用程序管理器初始化成功");
    return true;
}

void ApplicationManager::shutdown() {
    if (!initialized) {
        return;
    }

    Logger::get_instance().info("关闭应用程序管理器...");

    // 停止HTTP服务器
    if (httpServer && httpServer->isRunning()) {
        Logger::get_instance().info("停止HTTP服务器...");
        httpServer->stop();
    }

    // 停止gRPC服务器
    if (grpcServer) {
        Logger::get_instance().info("停止gRPC服务器...");
        grpcServer->stop();
    }

    // 清理模型资源
    singleModelPools_.clear();
    std::vector<std::unique_ptr<SingleModelEntry>>().swap(singleModelPools_);

    // 清理gRPC服务初始化器
    grpcServiceInitializers.clear();
    std::vector<std::unique_ptr<GrpcServiceInitializerBase>>().swap(grpcServiceInitializers);

    // 关闭日志系统
    Logger::shutdown();

    initialized = false;
}

const HTTPServerConfig& ApplicationManager::getHTTPServerConfig() const {
    return AppConfig::getHTTPServerConfig();
}

bool ApplicationManager::initializeModels() {
    Logger::get_instance().info("开始模型初始化...");

    // 获取所有模型配置
    const auto& modelConfigs = AppConfig::getModelConfigs();

    if (modelConfigs.empty()) {
        Logger::get_instance().warning("未找到模型配置");
        return true; // 没有模型也不是错误
    }

    bool all_success = true;

    // 遍历所有模型配置并尝试初始化
    for (const auto& config : modelConfigs) {
        bool model_initialized = ExceptionHandler::execute(
                "初始化模型: " + config.name,
                [&]() {
                    Logger::get_instance().info("初始化模型: " + config.name);

                    // 验证模型路径
                    if (config.model_path.empty()) {
                        throw ModelException("模型路径为空", config.name);
                    }

                    // 检查模型文件是否存在
                    std::ifstream modelFile(config.model_path);
                    if (!modelFile.good()) {
                        throw ModelException("模型文件不存在或无法访问: " + config.model_path, config.name);
                    }

                    // 验证模型类型
                    if (config.model_type <= 0) {
                        throw ModelException("无效的模型类型: " + std::to_string(config.model_type), config.name);
                    }

                    // 验证阈值范围
                    if (config.objectThresh < 0.0 || config.objectThresh > 1.0) {
                        throw ModelException(
                                "无效的对象检测阈值: " + std::to_string(config.objectThresh) +
                                ", 阈值必须在0.0到1.0之间",
                                config.name
                        );
                    }

                    std::unique_ptr<rknn_lite> rknn_ptr = std::make_unique<rknn_lite>(
                            const_cast<char*>(config.model_path.c_str()),
                            config.model_type % 3,
                            config.model_type,
                            config.objectThresh
                    );

                    std::unique_ptr<SingleModelEntry> aiPool = std::make_unique<SingleModelEntry>();

                    aiPool->modelType = config.model_type;
                    aiPool->modelName = config.name;
                    aiPool->singleRKModel = std::move(rknn_ptr);
                    aiPool->isEnabled = false;

                    singleModelPools_.emplace_back(std::move(aiPool));

                    // 添加初始化完成的日志
                    Logger::get_instance().info("模型初始化成功: " + config.name);
                }
        );

        // 如果某个模型初始化失败，记录但继续初始化其他模型
        if (!model_initialized) {
            all_success = false;
            Logger::get_instance().error("模型初始化失败: " + config.name + ", 继续初始化其他模型");
        }
    }

    if (all_success) {
        Logger::get_instance().info("所有模型初始化成功");
    } else {
        Logger::get_instance().warning("部分模型初始化失败，请检查日志");
    }

    return all_success;
}

std::string ApplicationManager::getGrpcServerAddress() const {
    const auto& grpcConfig = AppConfig::getGRPCServerConfig();
    return grpcConfig.host + ":" + std::to_string(grpcConfig.port);
}

HttpServer* ApplicationManager::getHttpServer() const {
    return httpServer.get();
}

GrpcServer* ApplicationManager::getGrpcServer() const {
    return grpcServer.get();
}

// 模型访问方法实现
std::shared_ptr<rknn_lite> ApplicationManager::getSharedModelReference(int modelType) const {
    for (const auto& model : singleModelPools_) {
        if (model && model->modelType == modelType) {
            // 创建共享引用但不负责删除资源
            return std::shared_ptr<rknn_lite>(model->singleRKModel.get(), [](rknn_lite*){});
        }
    }
    return nullptr;
}

std::shared_ptr<rknn_lite> ApplicationManager::getSharedModelByName(const std::string& modelName) const {
    for (const auto& model : singleModelPools_) {
        if (model && model->modelName == modelName) {
            return std::shared_ptr<rknn_lite>(model->singleRKModel.get(), [](rknn_lite*){});
        }
    }
    return nullptr;
}

bool ApplicationManager::isModelEnabled(int modelType) const {
    for (const auto& model : singleModelPools_) {
        if (model && model->modelType == modelType) {
            return model->isEnabled;
        }
    }
    return false;
}

bool ApplicationManager::setModelEnabled(int modelType, bool enabled) {
    for (auto& model : singleModelPools_) {
        if (model && model->modelType == modelType) {
            model->isEnabled = enabled;
            return true;
        }
    }
    return false;
}

// gRPC服务方法实现
void ApplicationManager::registerGrpcServiceInitializer(std::unique_ptr<GrpcServiceInitializerBase> initializer) {
    if (initializer) {
        Logger::info("注册gRPC服务初始化器: " + initializer->getServiceName());
        grpcServiceInitializers.push_back(std::move(initializer));
    }
}

bool ApplicationManager::initializeGrpcServices() {
    if (!grpcServer) {
        Logger::error("无法初始化gRPC服务: 服务器未创建");
        return false;
    }

    bool allSucceeded = true;

    for (const auto& initializer : grpcServiceInitializers) {
        if (initializer) {
            Logger::info("初始化gRPC服务: " + initializer->getServiceName());

            if (!initializer->initialize(grpcServer.get())) {
                Logger::error("初始化gRPC服务失败: " + initializer->getServiceName());
                allSucceeded = false;
                // 继续初始化其他服务，即使一个失败
            } else {
                Logger::info("成功初始化gRPC服务: " + initializer->getServiceName());
            }
        }
    }

    return allSucceeded;
}

bool ApplicationManager::registerGrpcServicesFromRegistry() {
    return ExceptionHandler::execute("从注册表注册gRPC服务", [&]() {
        Logger::info("从注册表注册gRPC服务");
        auto& registry = GrpcServiceRegistry::getInstance();

        // 使用工厂初始化所有服务
        GrpcServiceFactory::initializeAllServices(registry, *this);

        // 注册所有服务
        return registry.registerAllServices(*this);
    });
}

bool ApplicationManager::initializeGrpcServer() {
    return ExceptionHandler::execute("初始化gRPC服务器", [&]() {
        std::string grpcAddress = getGrpcServerAddress();
        Logger::info("正在初始化gRPC服务器，地址: " + grpcAddress);

        // 创建gRPC服务器
        grpcServer = std::make_unique<GrpcServer>(grpcAddress);

        // 初始化注册的服务
        if (!initializeGrpcServices()) {
            Logger::warning("部分gRPC服务初始化失败");
        }

        // 启动服务器
        if (!grpcServer->start()) {
            Logger::warning("在 " + grpcAddress + " 上启动gRPC服务器失败，将继续运行但不包含gRPC功能");
            return false;
        }

        Logger::info("gRPC服务器在 " + grpcAddress + " 上成功启动");
        return true;
    });
}

bool ApplicationManager::initializeRoutes() {
    return ExceptionHandler::execute("初始化路由", [&]() {
        // 初始化所有路由组
        RouteInitializer::initializeRoutes();
        Logger::info("路由初始化成功");
        return true;
    });
}

bool ApplicationManager::startHttpServer() {
    return ExceptionHandler::execute("启动HTTP服务器", [&]() {
        // 创建HTTP服务器
        httpServer = std::make_unique<HttpServer>(getHTTPServerConfig());

        // 配置所有路由
        RouteManager::getInstance().configureRoutes(*httpServer);

        // 启动服务器
        if (!httpServer->start()) {
            Logger::error("启动HTTP服务器失败");
            return false;
        }

        Logger::info("HTTP服务器在 " + getHTTPServerConfig().host + ":" +
                     std::to_string(getHTTPServerConfig().port) + " 上成功启动");
        return true;
    });
}
