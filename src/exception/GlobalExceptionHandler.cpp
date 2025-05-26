
#include "exception/GlobalExceptionHandler.h"
#include <sstream>


bool ExceptionHandler::execute(const std::string& operation, std::function<void()> func) {
    try {
        func();
        return true;
    } catch (const std::exception& e) {
        std::stringstream log_msg;
        log_msg << "Operation execution failed: " << operation << " - " << e.what();
        Logger::get_instance().error(log_msg.str());
        return false;
    } catch (...) {
        std::stringstream log_msg;
        log_msg << "Operation execution failed (unknown error): " << operation;
        Logger::get_instance().error(log_msg.str());
        return false;
    }
}