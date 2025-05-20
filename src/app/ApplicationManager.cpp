//
// Created by YJK on 2025/5/15.
//

#include "app/ApplicationManager.h"
#include "exception/GlobalExceptionHandler.h"
#include "grpc/GrpcServer.h"

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

    // 先初始化日志系统，使用默认值
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
        // 可以在这里设置默认配置值
    } else {
        Logger::get_instance().info("Configuration loaded successfully");
    }

    // 使用加载的设置或默认值重新初始化日志系统
    ExceptionHandler::execute("Initializ Log systerm", [&]() {
        Logger::init(
                AppConfig::getLogToFile(),
                AppConfig::getLogFilePath(),
                static_cast<LogLevel>(AppConfig::getLogLevel())
        );
    });

    // 可以在这里添加模型初始化代码，同样使用异常处理
    bool models_initialized = ExceptionHandler::execute("init model", [&]() {
        if (!initializeModels()) {
            throw ConfigException("model init failed");
        }
    });

    if (!models_initialized) {
        Logger::get_instance().warning("model init failed, program will continue...");
        // 可以在这里决定是否继续，或者根据需求终止程序
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

    // 添加资源清理代码
    singleModelPools_.clear();
    std::vector<std::unique_ptr<SingleModelEntry>>().swap(singleModelPools_);

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

                    std::unique_ptr<rknn_lite> rknn_ptr = std::make_unique<rknn_lite>(const_cast<char *>(config.model_path.c_str()),config.model_type % 3,
                                                                                      config.model_type, config.objectThresh);
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
            Logger::get_instance().error("Model initialization failed: " + config.name + ", continuing to initialize other models");
        }
    }

    if (all_success) {
        Logger::get_instance().info("All models initialized successfully");
    } else {
        Logger::get_instance().warning("Some models failed to initialize, please check the logs");
    }

    return all_success;
}

std::string ApplicationManager::getGrpcServerAddress() const {
    const auto& grpcConfig = AppConfig::getGRPCServerConfig();
    return grpcConfig.host + ":" + std::to_string(grpcConfig.port);
}