//
// Created by YJK on 2025/5/15.
//

#include "app/ApplicationManager.h"
#include "exception/GlobalExceptionHandler.h"
#include "grpc/GrpcServer.h"
#include "routeManager/HttpServer.h"
#include "routeManager/RouteManager.h"
#include "routeManager/base/RouteInitializer.h"
#include "grpc/base/GrpcServiceInitializerBase.h"
#include "grpc/base/GrpcServiceRegistry.h"
#include "grpc/base/GrpcServiceFactory.h"

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
    bool config_loaded = ExceptionHandler::execute("Loading configuration file", [&]() {
        if (!AppConfig::loadFromFile(configFilePath)) {
            throw ConfigException("Failed to load configuration from path: " + configFilePath);
        }
    });

    if (!config_loaded) {
        Logger::get_instance().error("Configuration loading failed, using default configuration");
    } else {
        Logger::get_instance().info("Configuration loaded successfully");
    }

    // 使用加载的设置或默认值重新初始化日志系统
    ExceptionHandler::execute("Initializing logging system", [&]() {
        Logger::init(
                AppConfig::getLogToFile(),
                AppConfig::getLogFilePath(),
                static_cast<LogLevel>(AppConfig::getLogLevel())
        );
    });

    // 初始化模型
    bool models_initialized = ExceptionHandler::execute("Initializing models", [&]() {
        if (!initializeModels()) {
            throw ConfigException("Model initialization failed");
        }
    });

    if (!models_initialized) {
        Logger::get_instance().warning("Model initialization failed, program will continue running...");
    }

    // 从注册表注册所有gRPC服务
    bool services_registered = registerGrpcServicesFromRegistry();
    if (!services_registered) {
        Logger::get_instance().warning("gRPC service registration failed, program will continue but gRPC functionality might be unavailable");
    }

    // 初始化gRPC服务器
    bool grpc_initialized = initializeGrpcServer();
    if (!grpc_initialized) {
        Logger::get_instance().warning("gRPC server initialization failed, program will continue without gRPC functionality");
    }

    // 初始化路由
    bool routes_initialized = initializeRoutes();
    if (!routes_initialized) {
        Logger::get_instance().error("Route initialization failed");
        return false;
    }

    // 启动HTTP服务器
    bool http_started = startHttpServer();
    if (!http_started) {
        Logger::get_instance().error("HTTP server start failed");
        return false;
    }

    initialized = true;
    Logger::get_instance().info("Application manager initialized successfully");
    return true;
}

void ApplicationManager::shutdown() {
    if (!initialized) {
        return;
    }

    Logger::get_instance().info("Shutting down application manager...");

    // 停止HTTP服务器
    if (httpServer && httpServer->isRunning()) {
        Logger::get_instance().info("Stopping HTTP server...");
        httpServer->stop();
    }

    // 停止gRPC服务器
    if (grpcServer) {
        Logger::get_instance().info("Stopping gRPC server...");
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
    Logger::get_instance().info("Starting model initialization...");

    // 获取所有模型配置
    const auto& modelConfigs = AppConfig::getModelConfigs();

    if (modelConfigs.empty()) {
        Logger::get_instance().warning("No model configuration found");
        return true; // 没有模型也不是错误
    }

    bool all_success = true;

    // 遍历所有模型配置并尝试初始化
    for (const auto& config : modelConfigs) {
        bool model_initialized = ExceptionHandler::execute(
                "初始化模型: " + config.name,
                [&]() {
                    Logger::get_instance().info("Initializing model: " + config.name);

                    // 验证模型路径
                    if (config.model_path.empty()) {
                        throw ModelException("Model path is empty", config.name);
                    }

                    // 检查模型文件是否存在
                    std::ifstream modelFile(config.model_path);
                    if (!modelFile.good()) {
                        throw ModelException("Model file does not exist or cannot be accessed: " + config.model_path, config.name);
                    }

                    // 验证模型类型
                    if (config.model_type <= 0) {
                        throw ModelException("Invalid model type: " + std::to_string(config.model_type), config.name);
                    }

                    // 验证阈值范围
                    if (config.objectThresh < 0.0 || config.objectThresh > 1.0) {
                        throw ModelException(
                                "Invalid object detection threshold: " + std::to_string(config.objectThresh) +
                                ", threshold must be between 0.0 and 1.0",
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
                    Logger::get_instance().info("Model initialized successfully: " + config.name);
                }
        );

        // 如果某个模型初始化失败，记录但继续初始化其他模型
        if (!model_initialized) {
            all_success = false;
            Logger::get_instance().error("Model initialization failed: " + config.name + ", continuing with initializing other models");
        }
    }

    if (all_success) {
        Logger::get_instance().info("All models initialized successfully");
    } else {
        Logger::get_instance().warning("Some models failed to initialize, please check logs");
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
        Logger::info("Registering gRPC service initializer: " + initializer->getServiceName());
        grpcServiceInitializers.push_back(std::move(initializer));
    }
}

bool ApplicationManager::initializeGrpcServices() {
    if (!grpcServer) {
        Logger::error("Cannot initialize gRPC services: server not created");
        return false;
    }

    bool allSucceeded = true;

    for (const auto& initializer : grpcServiceInitializers) {
        if (initializer) {
            Logger::info("Initializing gRPC service: " + initializer->getServiceName());

            if (!initializer->initialize(grpcServer.get())) {
                Logger::error("Failed to initialize gRPC service: " + initializer->getServiceName());
                allSucceeded = false;
                // 继续初始化其他服务，即使一个失败
            } else {
                Logger::info("Successfully initialized gRPC service: " + initializer->getServiceName());
            }
        }
    }

    return allSucceeded;
}

bool ApplicationManager::registerGrpcServicesFromRegistry() {
    return ExceptionHandler::execute("Registering gRPC services from registry", [&]() {
        Logger::info("Registering gRPC services from registry");
        auto& registry = GrpcServiceRegistry::getInstance();

        // 使用工厂初始化所有服务
        GrpcServiceFactory::initializeAllServices(registry, *this);

        // 注册所有服务
        return registry.registerAllServices(*this);
    });
}

bool ApplicationManager::initializeGrpcServer() {
    return ExceptionHandler::execute("Initializing gRPC server", [&]() {
        std::string grpcAddress = getGrpcServerAddress();
        Logger::info("Initializing gRPC server, address: " + grpcAddress);

        // 创建gRPC服务器
        grpcServer = std::make_unique<GrpcServer>(grpcAddress);

        // 初始化注册的服务
        if (!initializeGrpcServices()) {
            Logger::warning("Some gRPC services failed to initialize");
        }

        // 启动服务器
        if (!grpcServer->start()) {
            Logger::warning("Failed to start gRPC server at " + grpcAddress + ", will continue running without gRPC functionality");
            return false;
        }

        Logger::info("gRPC server successfully started at " + grpcAddress);
        return true;
    });
}

bool ApplicationManager::initializeRoutes() {
    return ExceptionHandler::execute("Initializing routes", [&]() {
        // 初始化所有路由组
        RouteInitializer::initializeRoutes();
        Logger::info("Routes initialized successfully");
        return true;
    });
}

bool ApplicationManager::startHttpServer() {
    return ExceptionHandler::execute("Starting HTTP server", [&]() {
        // 创建HTTP服务器
        httpServer = std::make_unique<HttpServer>(getHTTPServerConfig());

        // 配置所有路由
        RouteManager::getInstance().configureRoutes(*httpServer);

        // 启动服务器
        if (!httpServer->start()) {
            Logger::error("Failed to start HTTP server");
            return false;
        }

        Logger::info("HTTP server successfully started at " + getHTTPServerConfig().host + ":" +
                     std::to_string(getHTTPServerConfig().port));
        return true;
    });
}
