#ifndef STREAM_CONFIG_H
#define STREAM_CONFIG_H

#include <string>
#include <map>
#include <vector>
#include "nlohmann/json.hpp"  // 直接包含整个json.hpp

/**
 * @brief 模型配置结构体
 * 包含模型的各种参数配置
 */
struct ModelConfig {
    // 模型名称
    std::string name;

    // 模型路径
    std::string model_path;

    // 模型类型
    int model_type = 1;

    // 对象检测阈值
    float objectThresh = 0.5;

    /**
     * @brief 从JSON创建配置
     * @param j JSON对象
     * @return 配置实例
     */
    static ModelConfig fromJson(const nlohmann::json& j);

    /**
     * @brief 将配置转换为JSON
     * @return JSON对象
     */
    nlohmann::json toJson() const;
};

/**
 * @brief HTTP 服务器配置
 */
struct HTTPServerConfig {
    std::string host;
    int port;
    int connectionTimeout;
    int readTimeout;

    HTTPServerConfig() : host("127.0.0.1"), port(9000),
                         connectionTimeout(5), readTimeout(5) {}

    static HTTPServerConfig fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;
};

/*
 * @brief 并发配置
 * */
struct ConcurrencyServerConfig {
    int maxConcurrentRequests = 10;
    int modelPoolSize = 3;
    int requestTimeoutMs = 30000;
    int modelAcquireTimeoutMs = 5000;
    bool enableConcurrencyMonitoring = true;

    ConcurrencyServerConfig() = default;

    static ConcurrencyServerConfig fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;
};



/**
 * @brief 应用配置类
 * 包含整个应用程序的配置
 */
class AppConfig {
public:
    /**
     * @brief 从配置文件加载应用配置
     * @param configFilePath 配置文件路径
     * @return 成功加载返回true
     */
    static bool loadFromFile(const std::string& configFilePath);

    /**
     * @brief 保存配置到文件
     * @param configFilePath 配置文件路径
     * @return 成功保存返回true
     */
    static bool saveToFile(const std::string& configFilePath);

    /**
     * @brief 获取日志配置
     */
    static bool getLogToFile();
    static std::string getLogFilePath();
    static int getLogLevel();

    /**
     * @brief 获取线程池配置
     */
    static int getThreadPoolSize();

    /**
     * @brief 获取额外选项
     */
    static const std::map<std::string, std::string>& getExtraOptions();

    /**
     * @brief 获取目录路径
     */
    static std::string getDirPath();

    /**
     * @brief 获取HTTP服务器配置
     * @return HTTP服务器配置
     */
    static const HTTPServerConfig& getHTTPServerConfig();

    /**
     * @brief 获取全部模型配置
     * @return 模型配置列表
     */
    static const std::vector<ModelConfig>& getModelConfigs();

    /**
     * @brief 通过名称查找模型配置
     * @param name 模型名称
     * @return 找到的配置，如果未找到则返回默认配置
     */
    static ModelConfig findModelConfigByName(const std::string& name);

    /**
     * @brief 添加模型配置
     * @param config 要添加的配置
     */
    static void addModelConfig(const ModelConfig& config);

    /**
     * @brief 更新模型配置
     * @param config 新配置
     * @return 更新成功返回true
     */
    static bool updateModelConfig(const ModelConfig& config);

    /**
     * @brief 删除模型配置
     * @param name 模型名称
     * @return 删除成功返回true
     */
    static bool removeModelConfig(const std::string& name);

    /**
     * @brief 获取并发配置
     */
    static const ConcurrencyServerConfig& getConcurrencyConfig();

private:
    static bool logToFile;
    static std::string logFilePath;
    static int logLevel;
    static int threadPoolSize;
    static std::map<std::string, std::string> extraOptions;
    static std::string dirPath;
    static HTTPServerConfig httpServerConfig;
    static std::vector<ModelConfig> modelConfigs;
    static ConcurrencyServerConfig concurrencyConfig;
};

#endif // STREAM_CONFIG_H