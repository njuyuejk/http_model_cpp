
#include "exception/GlobalExceptionHandler.h"
#include <sstream>

void ExceptionHandler::handleRequest(
        const httplib::Request& req,
        httplib::Response& res,
        std::function<void(const httplib::Request&, httplib::Response&)> func) {
    try {
        func(req, res);
    } catch (const AppException& e) {
        setErrorResponse(res, e, &req);
    } catch (const json::exception& e) {
        JSONParseException je(std::string("JSON错误: ") + e.what());
        setErrorResponse(res, je, &req);
    } catch (const std::exception& e) {
        setErrorResponse(res, e, &req);
    } catch (...) {
        std::runtime_error unknown("未知错误");
        setErrorResponse(res, unknown, &req);
    }
}

void ExceptionHandler::setErrorResponse(
        httplib::Response& res,
        const std::exception& e,
        const httplib::Request* req) {

    int status_code = 500;
    std::string error_type = "Server Error";

    // 根据异常类型设置不同的状态码和错误类型
    if (auto app_ex = dynamic_cast<const AppException*>(&e)) {
        status_code = app_ex->getErrorCode();
        error_type = app_ex->getErrorType();
    } else if (dynamic_cast<const json::exception*>(&e)) {
        status_code = 400;
        error_type = "JSON Parsing Error";
    }

    // 构建错误响应JSON
    json error_json = {
            {"status", "error"},
            {"error_type", error_type},
            {"message", e.what()}
    };

    // 如果有请求信息，添加路径
    if (req) {
        error_json["path"] = req->path;

        // 记录错误日志
        std::stringstream log_msg;
        log_msg << error_type << " (" << status_code << "): "
                << e.what() << " path: " << req->path;
        LOGGER_ERROR(log_msg.str());
    } else {
        // 记录错误日志（无请求信息）
        std::stringstream log_msg;
        log_msg << error_type << " (" << status_code << "): " << e.what();
        LOGGER_ERROR(log_msg.str());
    }

    res.status = status_code;
    res.set_content(error_json.dump(), "application/json");
}

bool ExceptionHandler::execute(const std::string& operation, std::function<void()> func) {
    try {
        func();
        return true;
    } catch (const std::exception& e) {
        std::stringstream log_msg;
        log_msg << "Operation execution failed: " << operation << " - " << e.what();
        LOGGER_ERROR(log_msg.str());
        return false;
    } catch (...) {
        std::stringstream log_msg;
        log_msg << "Operation execution failed (unknown error): " << operation;
        LOGGER_ERROR(log_msg.str());
        return false;
    }
}