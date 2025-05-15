#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <atomic>

#ifdef ERROR
#undef ERROR
#endif

/**
 * @brief 日志级别枚举
 */
enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

/**
 * @brief 日志管理类
 * 提供日志记录和管理功能
 */
class Logger {
public:
    /**
     * @brief 初始化日志系统
     * @param logToFile 是否记录到文件
     * @param logDir 日志文件目录
     * @param minLevel 最小记录日志级别
     */
    static void init(bool logToFile = true, const std::string& logDir = "logs", LogLevel minLevel = LogLevel::INFO);

    /**
     * @brief 关闭日志系统
     */
    static void shutdown();

    /**
     * @brief 第一阶段：准备关闭
     */
    static void prepareShutdown();

    /**
     * @brief 第二阶段：最终关闭
     */
    static void finalizeShutdown();

    /**
     * @brief 关闭过程中的日志
     * @param message 日志消息
     */
    static void shutdownMessage(const std::string& message);

    /**
     * @brief 记录调试级别日志
     * @param message 日志消息
     */
    static void debug(const std::string& message);

    /**
     * @brief 记录信息级别日志
     * @param message 日志消息
     */
    static void info(const std::string& message);

    /**
     * @brief 记录警告级别日志
     * @param message 日志消息
     */
    static void warning(const std::string& message);

    /**
     * @brief 记录错误级别日志
     * @param message 日志消息
     */
    static void error(const std::string& message);

    /**
     * @brief 记录致命错误级别日志
     * @param message 日志消息
     */
    static void fatal(const std::string& message);

    /**
     * @brief 获取Logger单例实例
     * @return Logger实例引用
     */
    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 日志记录方法
     * @param message 日志消息
     * @param level 日志级别
     */
    void log(const std::string& message, LogLevel level = LogLevel::INFO) {
        Logger::log(level, message);
    }

private:
    /**
     * @brief 记录日志的主要方法
     * @param level 日志级别
     * @param message 日志消息
     */
    static void log(LogLevel level, const std::string& message);

    /**
     * @brief 获取当前日期的日志文件路径
     * @return 日志文件路径
     */
    static std::string getCurrentLogFilePath();

    /**
     * @brief 检查并清理旧的日志文件
     */
    static void cleanupOldLogs();

    /**
     * @brief 检查并切换日志文件（如果需要）
     */
    static void checkAndRotateLogFile();

    /**
     * @brief 创建目录（如果不存在）
     * @param path 目录路径
     * @return 成功创建返回true
     */
    static bool createDirectory(const std::string& path);

    /**
     * @brief 检查文件是否存在
     * @param filename 文件名
     * @return 文件存在返回true
     */
    static bool fileExists(const std::string& filename);

    /**
     * @brief 获取目录中的所有文件
     * @param directory 目录路径
     * @return 文件路径列表
     */
    static std::vector<std::string> getFilesInDirectory(const std::string& directory);

    // 禁止外部构造和拷贝
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 使用原子变量避免竞态条件
    static std::atomic<bool> isShuttingDown;
    static std::atomic<int> shutdownPhase; // 0=正常, 1=准备关闭, 2=最终关闭

    // 静态成员变量
    static std::mutex logMutex;
    static std::ofstream logFile;
    static bool initialized;
    static bool useFileOutput;
    static LogLevel minimumLevel;
    static std::string logDirectory;
    static std::string currentLogDate;
    static const int MAX_LOG_DAYS = 30;
};

#endif // LOGGER_H