#include "common/StreamConfig.h"
#include "common/Logger.h"
#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <random>

// 使用nlohmann/json库
using json = nlohmann::json;

// 初始化静态成员
std::vector<ModelConfig> AppConfig::modelConfigs;
bool AppConfig::logToFile = false;
std::string AppConfig::logFilePath = "./logs";
int AppConfig::logLevel = 1; // INFO
int AppConfig::threadPoolSize = 4;
std::map<std::string, std::string> AppConfig::extraOptions;
std::string AppConfig::dirPath = "/root/data";

GRPCServerConfig AppConfig::grpcServerConfig;

ConcurrencyServerConfig AppConfig::concurrencyConfig;

// ModelConfig 实现
ModelConfig ModelConfig::fromJson(const nlohmann::json& j) {
    ModelConfig config;

    if (j.contains("name") && j["name"].is_string())
        config.name = j["name"];

    if (j.contains("model_path") && j["model_path"].is_string())
        config.model_path = j["model_path"];

    if (j.contains("model_type") && j["model_type"].is_number())
        config.model_type = j["model_type"];

    if (j.contains("objectThresh") && j["objectThresh"].is_number())
        config.objectThresh = j["objectThresh"];

    return config;
}

nlohmann::json ModelConfig::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["model_path"] = model_path;
    j["model_type"] = model_type;
    j["objectThresh"] = objectThresh;
    return j;
}

// AppConfig 相关方法
bool AppConfig::loadFromFile(const std::string& configFilePath) {
    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        return false;
    }

    try {
        // 解析JSON
        json configJson;
        file >> configJson;
        file.close();

        // 清除现有配置
        modelConfigs.clear();
        extraOptions.clear();

        // 加载常规设置
        if (configJson.contains("general")) {
            auto& general = configJson["general"];

            if (general.contains("logToFile") && general["logToFile"].is_boolean())
                logToFile = general["logToFile"];

            if (general.contains("logFilePath") && general["logFilePath"].is_string())
                logFilePath = general["logFilePath"];

            if (general.contains("logLevel") && general["logLevel"].is_number_integer())
                logLevel = general["logLevel"];

            if (general.contains("threadPoolSize") && general["threadPoolSize"].is_number_integer())
                threadPoolSize = general["threadPoolSize"];

            if (general.contains("grpc_server") && general["grpc_server"].is_object()) {
                grpcServerConfig = GRPCServerConfig::fromJson(general["grpc_server"]);
                Logger::info("Loading gRPC server configuration: " + grpcServerConfig.host + ":" +
                             std::to_string(grpcServerConfig.port));
            }

            // 加载并发配置
            if (general.contains("concurrency") && general["concurrency"].is_object()) {
                concurrencyConfig = ConcurrencyServerConfig::fromJson(general["concurrency"]);
                Logger::info("Loading concurrency configuration: pool_size=" +
                             std::to_string(concurrencyConfig.modelPoolSize) +
                             ", max_concurrent=" + std::to_string(concurrencyConfig.maxConcurrentRequests));
            }
        }

        // 加载模型配置
        if (configJson.contains("model") && configJson["model"].is_array()) {
            for (auto& modelJson : configJson["model"]) {
                ModelConfig config = ModelConfig::fromJson(modelJson);
                if (!config.name.empty()) {
                    modelConfigs.push_back(config);
                    Logger::info("Loading model configuration: " + config.name);
                }
            }
        }

        return true;
    }
    catch (const json::exception& e) {
        Logger::error("JSON parsing error: " + std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        Logger::error("Error loading configuration file: " + std::string(e.what()));
        return false;
    }
}

bool AppConfig::saveToFile(const std::string& configFilePath) {
    try {
        json configJson;

        // 创建general部分
        json general;
        general["logToFile"] = logToFile;
        general["logFilePath"] = logFilePath;
        general["logLevel"] = logLevel;
        general["threadPoolSize"] = threadPoolSize;
        general["dirPath"] = dirPath;

        // 添加额外选项
        json extraOptionsJson;
        for (const auto& [key, value] : extraOptions) {
            // 尝试转换数字
            try {
                if (key == "monitorInterval" || key == "periodicReconnectInterval") {
                    extraOptionsJson[key] = std::stoi(value);
                    continue;
                }
            } catch (...) {}

            // 处理布尔值
            if (value == "true" || value == "false") {
                extraOptionsJson[key] = (value == "true");
            } else {
                extraOptionsJson[key] = value;
            }
        }

        if (!extraOptionsJson.empty()) {
            general["extraOptions"] = extraOptionsJson;
        }

        configJson["general"] = general;

        // 添加模型配置
        json modelJson = json::array();
        for (const auto& config : modelConfigs) {
            modelJson.push_back(config.toJson());
        }
        configJson["model"] = modelJson;

        // 写入文件
        std::ofstream file(configFilePath);
        if (!file.is_open()) {
            return false;
        }

        file << configJson.dump(2); // 缩进2个空格的格式化输出
        file.close();

        return true;
    }
    catch (const json::exception& e) {
        Logger::error("JSON error when saving configuration: " + std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        Logger::error("Error saving configuration file: " + std::string(e.what()));
        return false;
    }
}

const ConcurrencyServerConfig& AppConfig::getConcurrencyConfig() {
    return concurrencyConfig;
}

// 模型配置相关方法
const std::vector<ModelConfig>& AppConfig::getModelConfigs() {
    return modelConfigs;
}

ModelConfig AppConfig::findModelConfigByName(const std::string& name) {
    auto it = std::find_if(modelConfigs.begin(), modelConfigs.end(),
                           [&name](const ModelConfig& config) {
                               return config.name == name;
                           });

    if (it != modelConfigs.end()) {
        return *it;
    }

    // 返回默认配置
    ModelConfig defaultConfig;
    defaultConfig.name = "default";
    defaultConfig.model_path = "./model/default.rknn";
    defaultConfig.model_type = 1;
    defaultConfig.objectThresh = 0.5;
    return defaultConfig;
}

void AppConfig::addModelConfig(const ModelConfig& config) {
    modelConfigs.push_back(config);
}

bool AppConfig::updateModelConfig(const ModelConfig& config) {
    auto it = std::find_if(modelConfigs.begin(), modelConfigs.end(),
                           [&config](const ModelConfig& c) {
                               return c.name == config.name;
                           });

    if (it != modelConfigs.end()) {
        *it = config;
        return true;
    }

    return false;
}

bool AppConfig::removeModelConfig(const std::string& name) {
    auto it = std::find_if(modelConfigs.begin(), modelConfigs.end(),
                           [&name](const ModelConfig& config) {
                               return config.name == name;
                           });

    if (it != modelConfigs.end()) {
        modelConfigs.erase(it);
        return true;
    }

    return false;
}

// 实现其他必要的方法
bool AppConfig::getLogToFile() {
    return logToFile;
}

std::string AppConfig::getLogFilePath() {
    return logFilePath;
}

int AppConfig::getLogLevel() {
    return logLevel;
}

int AppConfig::getThreadPoolSize() {
    return threadPoolSize;
}

const std::map<std::string, std::string>& AppConfig::getExtraOptions() {
    return extraOptions;
}

std::string AppConfig::getDirPath() {
    return dirPath;
}

GRPCServerConfig GRPCServerConfig::fromJson(const nlohmann::json& j) {
    GRPCServerConfig config;

    if (j.contains("host") && j["host"].is_string())
        config.host = j["host"];

    if (j.contains("port") && j["port"].is_number_integer())
        config.port = j["port"];

    return config;
}

nlohmann::json GRPCServerConfig::toJson() const {
    nlohmann::json j;
    j["host"] = host;
    j["port"] = port;
    return j;
}

// 添加getter实现
const GRPCServerConfig& AppConfig::getGRPCServerConfig() {
    return grpcServerConfig;
}

ConcurrencyServerConfig ConcurrencyServerConfig::fromJson(const nlohmann::json& j) {
    ConcurrencyServerConfig config;

    if (j.contains("max_concurrent_requests") && j["max_concurrent_requests"].is_number_integer())
        config.maxConcurrentRequests = j["max_concurrent_requests"];

    if (j.contains("model_pool_size") && j["model_pool_size"].is_number_integer())
        config.modelPoolSize = j["model_pool_size"];

    if (j.contains("request_timeout_ms") && j["request_timeout_ms"].is_number_integer())
        config.requestTimeoutMs = j["request_timeout_ms"];

    if (j.contains("model_acquire_timeout_ms") && j["model_acquire_timeout_ms"].is_number_integer())
        config.modelAcquireTimeoutMs = j["model_acquire_timeout_ms"];

    if (j.contains("enable_concurrency_monitoring") && j["enable_concurrency_monitoring"].is_boolean())
        config.enableConcurrencyMonitoring = j["enable_concurrency_monitoring"];

    return config;
}

nlohmann::json ConcurrencyServerConfig::toJson() const {
    nlohmann::json j;
    j["max_concurrent_requests"] = maxConcurrentRequests;
    j["model_pool_size"] = modelPoolSize;
    j["request_timeout_ms"] = requestTimeoutMs;
    j["model_acquire_timeout_ms"] = modelAcquireTimeoutMs;
    j["enable_concurrency_monitoring"] = enableConcurrencyMonitoring;
    return j;
}