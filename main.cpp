#include "app/ApplicationManager.h"
#include <iostream>

int main() {
    Logger::info("Starting 58AI Program... \n"
                 "===============================================\n"
                 "         ███████╗ █████╗  █████╗ ██╗\n"
                 "         ██╔════╝██╔══██╗██╔══██╗██║\n"
                 "         ███████╗╚█████╔╝███████║██║\n"
                 "         ╚════██║██╔══██╗██╔══██║██║\n"
                 "         ███████║╚█████╔╝██║  ██║██║\n"
                 "         ╚══════╝ ╚════╝ ╚═╝  ╚═╝╚═╝\n"
                 "===============================================\n");

    try {
        // 初始化应用程序管理器
        auto& appManager = ApplicationManager::getInstance();
        if (!appManager.initialize("./modelConfig.json")) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }

        // 程序会在服务器启动后阻塞，直到服务器停止

        // 在程序结束时会自动调用析构函数停止服务器和关闭应用程序管理器
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error occurred during program execution: " << e.what() << std::endl;
        Logger::get_instance().fatal(std::string("Fatal program error: ") + e.what());
        return -1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred during program execution" << std::endl;
        Logger::get_instance().fatal("Unknown fatal program error");
        return -1;
    }
}
