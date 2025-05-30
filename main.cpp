#include "app/ApplicationManager.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <mutex>

// 全局退出标志
std::atomic<bool> g_shouldExit(false);
std::condition_variable g_exitCondition;
std::mutex g_exitMutex;

// 信号处理函数
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOGGER_INFO("\nReceived termination signal (" + std::to_string(signal) + "), initiating shutdown...");
        g_shouldExit = true;
        g_exitCondition.notify_all();
    }
}

int main() {

    // 设置信号处理器
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        // 初始化应用程序管理器
        auto& appManager = ApplicationManager::getInstance();
        if (!appManager.initialize("./modelConfig.json")) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }

        LOGGER_INFO("Starting 58AI Program... \n"
                     "===============================================\n"
                     "         ███████╗ █████╗  █████╗ ██╗\n"
                     "         ██╔════╝██╔══██╗██╔══██╗██║\n"
                     "         ███████╗╚█████╔╝███████║██║\n"
                     "         ╚════██║██╔══██╗██╔══██║██║\n"
                     "         ███████║╚█████╔╝██║  ██║██║\n"
                     "         ╚══════╝ ╚════╝ ╚═╝  ╚═╝╚═╝\n"
                     "===============================================\n");

        // 程序初始化完成，现在等待退出信号
        LOGGER_INFO("==========================================");
        LOGGER_INFO("Application is running. Press Ctrl+C to shutdown gracefully.");
        LOGGER_INFO("==========================================");

        // 阻塞主线程，等待退出信号
        {
            std::unique_lock<std::mutex> lock(g_exitMutex);
            g_exitCondition.wait(lock, [] { return g_shouldExit.load(); });
        }

        // 收到退出信号，开始关闭程序
        LOGGER_INFO("Shutdown signal received, cleaning up...");

        // ApplicationManager的析构函数会自动调用shutdown()
        // 但我们可以显式调用以确保有序关闭
        appManager.shutdown();

        LOGGER_INFO("Application shutdown completed successfully.");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error occurred during program execution: " << e.what() << std::endl;
        LOGGER_FATAL(std::string("Fatal program error: ") + e.what());
        return -1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred during program execution" << std::endl;
        LOGGER_FATAL("Unknown fatal program error");
        return -1;
    }
}
