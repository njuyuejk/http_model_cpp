//
// Created by YJK on 2025/5/15.
//

#include "app/ApplicationManager.h"
#include "exception/GlobalExceptionHandler.h"

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
        Logger::get_instance().warning("应用程序管理器已经初始化");
        return true;
    }

    configFilePath = configPath;

    // 先初始化日志系统，使用默认值
    Logger::init();

    Logger::get_instance().info("应用程序管理器正在初始化...");

    // 使用异常处理加载配置
    bool config_loaded = ExceptionHandler::execute("加载配置文件", [&]() {
        if (!AppConfig::loadFromFile(configFilePath)) {
            throw ConfigException("无法从以下路径加载配置: " + configFilePath);
        }
    });

    if (!config_loaded) {
        Logger::get_instance().error("配置加载失败，使用默认配置");
        // 可以在这里设置默认配置值
    } else {
        Logger::get_instance().info("配置加载成功");
    }

    // 使用加载的设置或默认值重新初始化日志系统
    ExceptionHandler::execute("初始化日志系统", [&]() {
        Logger::init(
                AppConfig::getLogToFile(),
                AppConfig::getLogFilePath(),
                static_cast<LogLevel>(AppConfig::getLogLevel())
        );
    });

    // 可以在这里添加模型初始化代码，同样使用异常处理
    // initializeModels();

    initialized = true;
    Logger::get_instance().info("应用程序管理器初始化成功");
    return true;
}

void ApplicationManager::shutdown() {
    if (!initialized) {
        return;
    }

    Logger::get_instance().info("应用程序管理器正在关闭...");

    // 添加资源清理代码

    // 关闭日志系统
    Logger::shutdown();

    initialized = false;
}

const HTTPServerConfig& ApplicationManager::getHTTPServerConfig() const {
    return AppConfig::getHTTPServerConfig();
}

bool ApplicationManager::initializeModels() {
    Logger::get_instance().info("开始初始化模型...");

    // 获取所有模型配置
    const auto& modelConfigs = AppConfig::getModelConfigs();

    if (modelConfigs.empty()) {
        Logger::get_instance().warning("没有找到模型配置");
        return true; // 没有模型也不是错误
    }

    bool all_success = true;

    // 遍历所有模型配置并尝试初始化
    for (const auto& config : modelConfigs) {
        bool model_initialized = ExceptionHandler::execute(
                "初始化模型: " + config.name,
                [&]() {
                    Logger::get_instance().info("正在初始化模型: " + config.name);

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
                                "，阈值必须在0.0到1.0之间",
                                config.name
                        );
                    }

                    // 这里是模型初始化的实际逻辑，根据模型类型进行不同的初始化
                    switch (config.model_type) {
                        case 1: // 例如：YOLO模型
                            Logger::get_instance().info("正在初始化YOLO模型: " + config.name);
                            // 这里是YOLO模型初始化代码
                            // 可能包括: 加载权重、配置GPU资源、设置检测参数等
                            break;

                        case 5: // 例如：UAV检测模型
                            Logger::get_instance().info("正在初始化UAV检测模型: " + config.name);
                            // 这里是UAV模型初始化代码
                            break;

                        default:
                            throw ModelException("未知的模型类型: " + std::to_string(config.model_type), config.name);
                    }

                    // 添加初始化完成的日志
                    Logger::get_instance().info("模型初始化成功: " + config.name);
                }
        );

        // 如果某个模型初始化失败，记录但继续初始化其他模型
        if (!model_initialized) {
            all_success = false;
            Logger::get_instance().error("模型初始化失败: " + config.name + "，将继续初始化其他模型");
        }
    }

    if (all_success) {
        Logger::get_instance().info("所有模型初始化成功");
    } else {
        Logger::get_instance().warning("部分模型初始化失败，请检查日志");
    }

    return all_success;
}