//
// Created by YJK on 2025/5/15.
//

#include "app/ApplicationManager.h"

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

    // 加载配置
    if (!AppConfig::loadFromFile(configFilePath)) {
        Logger::get_instance().error("无法从以下路径加载配置: " + configFilePath);
        return false;
    }

    Logger::get_instance().info("配置加载成功");

    // 使用加载的设置重新初始化日志系统
    Logger::init(
            AppConfig::getLogToFile(),
            AppConfig::getLogFilePath(),
            static_cast<LogLevel>(AppConfig::getLogLevel())
    );

    // 可以在这里添加模型初始化代码
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
    // 模型初始化占位符，供未来扩展使用
    Logger::get_instance().info("模型初始化（占位符）");
    return true;
}