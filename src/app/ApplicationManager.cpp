//
// Created by YJK on 2025/5/15.
//

#include "app/ApplicationManager.h"
#include "exception/GlobalExceptionHandler.h"
#include "routeManager/HttpServer.h"
#include "routeManager/RouteManager.h"
#include "routeManager/base/RouteInitializer.h"
#include "AIService/ModelPool.h"

// 初始化静态成员
ApplicationManager* ApplicationManager::instance = nullptr;
std::mutex ApplicationManager::instanceMutex;

ApplicationManager::ApplicationManager()
        : initialized(false), configFilePath("") {
    // 私有构造函数
    LOGGER_DEBUG("ApplicationManager constructor called");
}

ApplicationManager::~ApplicationManager() {
    if (initialized) {
        shutdown();
    }
    LOGGER_DEBUG("ApplicationManager destructor completed");
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
        LOGGER_WARNING("Application manager already initialized");
        return true;
    }

    configFilePath = configPath;

    // 日志初始化
    bool config_loaded = false;
    try {
        config_loaded = AppConfig::loadFromFile(configFilePath);
    } catch (const std::exception& e) {
        // 配置加载失败，但不记录日志（因为Logger还未初始化）
        std::cerr << "Configuration loading failed: " << e.what() << std::endl;
    }

    // 根据配置文件的设置初始化日志系统**
    if (config_loaded) {
        // 使用配置文件中的设置初始化日志系统
        Logger::init(
                AppConfig::getLogToFile(),
                AppConfig::getLogFilePath(),
                static_cast<LogLevel>(AppConfig::getLogLevel())
        );
    } else {
        // 配置加载失败，使用默认设置初始化日志系统
        Logger::init(false, "./logs", LogLevel::INFO);
    }

    // **现在Logger已经完全初始化，可以安全地记录日志**
    LOGGER_INFO("Initializing application manager...");

    if (config_loaded) {
        LOGGER_INFO("Configuration loaded successfully from: " + configFilePath);
    } else {
        LOGGER_WARNING("Configuration loading failed, using default configuration");
        // 尝试重新加载配置（这次记录详细错误）
        ExceptionHandler::execute("Loading configuration file", [&]() {
            if (!AppConfig::loadFromFile(configFilePath)) {
                throw ConfigException("Failed to load configuration from path: " + configFilePath);
            }
        });
    }

    // 加载并发配置
    const auto& config = AppConfig::getConcurrencyConfig();
    concurrencyConfig_.maxConcurrentRequests = config.maxConcurrentRequests;
    concurrencyConfig_.modelPoolSize = config.modelPoolSize;
    concurrencyConfig_.requestTimeoutMs = config.requestTimeoutMs;
    concurrencyConfig_.modelAcquireTimeoutMs = config.modelAcquireTimeoutMs;
    concurrencyConfig_.enableConcurrencyMonitoring = config.enableConcurrencyMonitoring;

    LOGGER_INFO("Concurrency configuration loaded - max_concurrent: " +
                 std::to_string(concurrencyConfig_.maxConcurrentRequests) +
                 ", pool_size: " + std::to_string(concurrencyConfig_.modelPoolSize) +
                 ", acquire_timeout: " + std::to_string(concurrencyConfig_.modelAcquireTimeoutMs) + "ms");

    // 初始化并发监控器
    if (concurrencyConfig_.enableConcurrencyMonitoring) {
        httpMonitor_ = std::make_unique<ConcurrencyMonitor>();
        LOGGER_INFO("Concurrency monitoring enabled");
    } else {
        LOGGER_INFO("Concurrency monitoring disabled");
    }

    // 初始化模型池而不是单个模型
    bool pools_initialized = ExceptionHandler::execute("Initializing model pools", [&]() {
        if (!initializeModelPools()) {
            throw ConfigException("Model pool initialization failed");
        }
    });

    if (!pools_initialized) {
        LOGGER_WARNING("Model pool initialization failed, program will continue running...");
    }

    // 初始化路由
    bool routes_initialized = initializeRoutes();
    if (!routes_initialized) {
        LOGGER_ERROR("Route initialization failed");
        return false;
    }

    // 启动HTTP服务器
    bool http_started = startHttpServer();
    if (!http_started) {
        LOGGER_ERROR("HTTP server start failed");
        return false;
    }

    initialized = true;
    LOGGER_INFO("Application manager initialized successfully");

    // 记录初始化摘要
    logInitializationSummary();

    return true;
}

void ApplicationManager::shutdown() {
    if (!initialized) {
        return;
    }

    LOGGER_INFO("Shutting down application manager...");

    // 停止HTTP服务器
    if (httpServer && httpServer->isRunning()) {
        LOGGER_INFO("Stopping HTTP server...");
        httpServer->stop();
    }

    // 关闭所有模型池
    {
        std::unique_lock<std::shared_mutex> lock(modelPoolsMutex_);
        LOGGER_INFO("Shutting down " + std::to_string(modelPools_.size()) + " model pools...");

        for (auto& pair : modelPools_) {
            if (pair.second) {
                LOGGER_INFO("Shutting down model pool for type: " + std::to_string(pair.first));
                pair.second->shutdown();
            }
        }

        modelPools_.clear();
        LOGGER_INFO("All model pools shutdown completed");
    }

    // 清理并发监控器
    if (httpMonitor_) {
        auto httpStats = httpMonitor_->getStats();
        LOGGER_INFO("HTTP final stats - total: " + std::to_string(httpStats.total) +
                     ", failed: " + std::to_string(httpStats.failed) +
                     ", failure_rate: " + std::to_string(httpStats.failureRate * 100) + "%");
        httpMonitor_.reset();
    }

    // 关闭日志系统
    LOGGER_INFO("Application manager shutdown completed");
    Logger::shutdown();

    initialized = false;
}

const HTTPServerConfig& ApplicationManager::getHTTPServerConfig() const {
    return AppConfig::getHTTPServerConfig();
}

bool ApplicationManager::initializeModelPools() {
    LOGGER_INFO("Starting model pool initialization...");

    // 获取所有模型配置
    const auto& modelConfigs = AppConfig::getModelConfigs();

    if (modelConfigs.empty()) {
        LOGGER_WARNING("No model configuration found");
        return true; // 没有模型也不是错误
    }

    bool all_success = true;
    std::unique_lock<std::shared_mutex> lock(modelPoolsMutex_);

    // 清理现有模型池
    modelPools_.clear();

    LOGGER_INFO("Found " + std::to_string(modelConfigs.size()) +
                 " model configurations, initializing pools with size " +
                 std::to_string(concurrencyConfig_.modelPoolSize));

    // 遍历所有模型配置并尝试初始化模型池
    for (const auto& config : modelConfigs) {
        bool pool_initialized = ExceptionHandler::execute(
                "初始化模型池: " + config.name,
                [&]() {
                    LOGGER_INFO("Initializing model pool: " + config.name +
                                 " (type: " + std::to_string(config.model_type) +
                                 ", path: " + config.model_path + ")");

                    // 验证模型路径
                    if (config.model_path.empty()) {
                        throw ModelException("Model path is empty", config.name);
                    }

                    // 检查模型文件是否存在
                    std::ifstream modelFile(config.model_path);
                    if (!modelFile.good()) {
                        throw ModelException("Model file does not exist or cannot be accessed: " +
                                             config.model_path, config.name);
                    }
                    modelFile.close();

                    // 验证模型类型
                    if (config.model_type <= 0) {
                        throw ModelException("Invalid model type: " + std::to_string(config.model_type),
                                             config.name);
                    }

                    // 验证阈值范围
                    if (config.objectThresh < 0.0 || config.objectThresh > 1.0) {
                        throw ModelException(
                                "Invalid object detection threshold: " + std::to_string(config.objectThresh) +
                                ", threshold must be between 0.0 and 1.0",
                                config.name
                        );
                    }

                    // 检查模型类型是否已存在
                    if (modelPools_.find(config.model_type) != modelPools_.end()) {
                        LOGGER_WARNING("Model type " + std::to_string(config.model_type) +
                                        " already exists, skipping " + config.name);
                        return;
                    }

                    // 创建模型池
                    auto pool = std::make_unique<ModelPool>(concurrencyConfig_.modelPoolSize);

                    if (!pool->initialize(config.model_path, config.model_type, config.objectThresh)) {
                        throw ModelException("Failed to initialize model pool", config.name);
                    }

                    // 存储模型池
                    modelPools_[config.model_type] = std::move(pool);

                    LOGGER_INFO("Model pool initialized successfully: " + config.name +
                                 " (type: " + std::to_string(config.model_type) + ") with " +
                                 std::to_string(concurrencyConfig_.modelPoolSize) + " instances");
                }
        );

        if (!pool_initialized) {
            all_success = false;
            LOGGER_ERROR("Model pool initialization failed: " + config.name +
                          ", continuing with initializing other model pools");
        }
    }

    if (all_success) {
        LOGGER_INFO("All model pools initialized successfully, total pools: " +
                     std::to_string(modelPools_.size()));
    } else {
        LOGGER_WARNING("Some model pools failed to initialize, please check logs");
    }

    return all_success;
}

HttpServer* ApplicationManager::getHttpServer() const {
    return httpServer.get();
}

// 模型池访问方法实现
bool ApplicationManager::executeModelInference(int modelType,
                                               const cv::Mat& imageData,
                                               std::vector<std::vector<std::any>>& results,
                                               std::vector<std::string>& plateResults,
                                               double startValue,
                                               double endValue,
                                               double& targetResult,
                                               int timeoutMs) {

    if (timeoutMs <= 0) {
        timeoutMs = concurrencyConfig_.modelAcquireTimeoutMs;
    }

    LOGGER_DEBUG("Executing model inference for type: " + std::to_string(modelType) +
                  ", timeout: " + std::to_string(timeoutMs) + "ms");

    std::shared_lock<std::shared_mutex> lock(modelPoolsMutex_);

    auto poolIt = modelPools_.find(modelType);
    if (poolIt == modelPools_.end()) {
        LOGGER_ERROR("Model pool not found for type: " + std::to_string(modelType));
        return false;
    }

    if (!poolIt->second->isEnabled()) {
        LOGGER_WARNING("Model pool disabled for type: " + std::to_string(modelType));
        return false;
    }

    lock.unlock(); // 释放读锁

    // 记录模型池状态
    auto poolStatus = poolIt->second->getStatus();
    LOGGER_DEBUG("Model pool status for type " + std::to_string(modelType) +
                  " - available: " + std::to_string(poolStatus.availableModels) +
                  "/" + std::to_string(poolStatus.totalModels));

    // 使用RAII获取模型
    ModelAcquirer acquirer(*poolIt->second, timeoutMs);

    if (!acquirer.isValid()) {
        LOGGER_ERROR("Failed to acquire model from pool within timeout (" +
                      std::to_string(timeoutMs) + "ms) for type: " + std::to_string(modelType));
        return false;
    }

    try {
        // 安全地使用模型进行推理
        acquirer->ori_img = imageData;

        acquirer->startValue = startValue;
        acquirer->endValue = endValue;

        LOGGER_DEBUG("Starting model inference for type: " + std::to_string(modelType));

        if (!acquirer->interf()) {
            LOGGER_ERROR("Model inference failed for type: " + std::to_string(modelType));
            return false;
        }

        // 获取结果
//        results = acquirer->results_vector;
        results = std::move(acquirer->results_vector);

        if (modelType == 4) {
            plateResults = std::move(acquirer->plateResults);
            LOGGER_DEBUG("Retrieved " + std::to_string(plateResults.size()) + " plate results");
        } else if (modelType == 5) {
            targetResult = acquirer->value;
        }

        // 清空原始向量以释放内存
        acquirer->results_vector.clear();
        acquirer->results_vector.shrink_to_fit();

        if (modelType == 4) {
            acquirer->plateResults.clear();
            acquirer->plateResults.shrink_to_fit();
        }

        LOGGER_DEBUG("Model inference completed successfully for type: " +
                      std::to_string(modelType) + ", results count: " + std::to_string(results.size()));
        return true;

    } catch (const std::exception& e) {
        LOGGER_ERROR("Model inference exception for type " + std::to_string(modelType) +
                      ": " + std::string(e.what()));
        return false;
    }
}

bool ApplicationManager::setModelEnabled(int modelType, bool enabled) {
    std::shared_lock<std::shared_mutex> lock(modelPoolsMutex_);

    auto poolIt = modelPools_.find(modelType);
    if (poolIt == modelPools_.end()) {
        LOGGER_ERROR("Model pool not found for type: " + std::to_string(modelType));
        return false;
    }

    poolIt->second->setEnabled(enabled);
    LOGGER_INFO("Model pool " + std::to_string(modelType) +
                 " status changed to: " + (enabled ? "enabled" : "disabled"));
    return true;
}

bool ApplicationManager::isModelEnabled(int modelType) const {
    std::shared_lock<std::shared_mutex> lock(modelPoolsMutex_);

    auto poolIt = modelPools_.find(modelType);
    if (poolIt == modelPools_.end()) {
        return false;
    }

    return poolIt->second->isEnabled();
}

ModelPool::PoolStatus ApplicationManager::getModelPoolStatus(int modelType) const {
    std::shared_lock<std::shared_mutex> lock(modelPoolsMutex_);

    auto poolIt = modelPools_.find(modelType);
    if (poolIt == modelPools_.end()) {
        // 返回默认状态
        ModelPool::PoolStatus defaultStatus;
        defaultStatus.totalModels = 0;
        defaultStatus.availableModels = 0;
        defaultStatus.busyModels = 0;
        defaultStatus.isEnabled = false;
        defaultStatus.modelType = modelType;
        return defaultStatus;
    }

    return poolIt->second->getStatus();
}

std::unordered_map<int, ModelPool::PoolStatus> ApplicationManager::getAllModelPoolStatus() const {
    std::shared_lock<std::shared_mutex> lock(modelPoolsMutex_);

    std::unordered_map<int, ModelPool::PoolStatus> statusMap;
    for (const auto& pair : modelPools_) {
        statusMap[pair.first] = pair.second->getStatus();
    }

    return statusMap;
}

// 并发监控方法实现
void ApplicationManager::startHttpRequest() {
    if (httpMonitor_ && concurrencyConfig_.enableConcurrencyMonitoring) {
        httpMonitor_->requestStarted();
    }
}

void ApplicationManager::completeHttpRequest() {
    if (httpMonitor_ && concurrencyConfig_.enableConcurrencyMonitoring) {
        httpMonitor_->requestCompleted();
    }
}

void ApplicationManager::failHttpRequest() {
    if (httpMonitor_ && concurrencyConfig_.enableConcurrencyMonitoring) {
        httpMonitor_->requestFailed();
        httpMonitor_->requestCompleted();
    }
}

ConcurrencyMonitor::Stats ApplicationManager::getHttpConcurrencyStats() const {
    if (httpMonitor_) {
        return httpMonitor_->getStats();
    }
    return ConcurrencyMonitor::Stats{0, 0, 0, 0.0};
}

const ConcurrencyConfig& ApplicationManager::getConcurrencyConfig() const {
    return concurrencyConfig_;
}


bool ApplicationManager::initializeRoutes() {
    return ExceptionHandler::execute("Initializing routes", [&]() {
        LOGGER_INFO("Initializing HTTP routes");

        // 初始化所有路由组
        RouteInitializer::initializeRoutes();

        LOGGER_INFO("HTTP routes initialized successfully");
        return true;
    });
}

bool ApplicationManager::startHttpServer() {
    return ExceptionHandler::execute("Starting HTTP server", [&]() {
        const auto& httpConfig = getHTTPServerConfig();
        LOGGER_INFO("Creating HTTP server with config: " + httpConfig.host + ":" +
                     std::to_string(httpConfig.port));

        // 创建HTTP服务器
        httpServer = std::make_unique<HttpServer>(httpConfig);

        // 配置所有路由
        LOGGER_INFO("Configuring HTTP routes");
        RouteManager::getInstance().configureRoutes(*httpServer);

        // 启动服务器
        LOGGER_INFO("Starting HTTP server...");
        if (!httpServer->start()) {
            LOGGER_ERROR("Failed to start HTTP server");
            return false;
        }

        LOGGER_INFO("HTTP server successfully started at " + httpConfig.host + ":" +
                     std::to_string(httpConfig.port));
        return true;
    });
}

void ApplicationManager::logInitializationSummary() {
    LOGGER_INFO("=== Application Manager Initialization Summary ===");

    // HTTP服务器状态
    if (httpServer && httpServer->isRunning()) {
        const auto& httpConfig = getHTTPServerConfig();
        LOGGER_INFO("✓ HTTP Server: Running at " + httpConfig.host + ":" + std::to_string(httpConfig.port));

        // 统计路由数量
        const auto& routes = httpServer->getRoutes();
        LOGGER_INFO("  - Routes registered: " + std::to_string(routes.size()));
    } else {
        LOGGER_INFO("✗ HTTP Server: Not running");
    }

    // 模型池状态
    {
        std::shared_lock<std::shared_mutex> lock(modelPoolsMutex_);
        LOGGER_INFO("✓ Model Pools: " + std::to_string(modelPools_.size()) + " pools initialized");

        for (const auto& pair : modelPools_) {
            auto status = pair.second->getStatus();
            LOGGER_INFO("  - Type " + std::to_string(pair.first) + ": " +
                         std::to_string(status.totalModels) + " instances, " +
                         (status.isEnabled ? "enabled" : "disabled"));
        }
    }

    // 并发配置
    LOGGER_INFO("✓ Concurrency Config:");
    LOGGER_INFO("  - Max concurrent requests: " + std::to_string(concurrencyConfig_.maxConcurrentRequests));
    LOGGER_INFO("  - Model pool size: " + std::to_string(concurrencyConfig_.modelPoolSize));
    LOGGER_INFO("  - Model acquire timeout: " + std::to_string(concurrencyConfig_.modelAcquireTimeoutMs) + "ms");
    LOGGER_INFO("  - Monitoring: " + std::string(concurrencyConfig_.enableConcurrencyMonitoring ? "enabled" : "disabled"));

    LOGGER_INFO("=== Initialization Summary End ===");
}
