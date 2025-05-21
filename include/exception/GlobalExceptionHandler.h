//
// Created by YJK on 2025/5/15.
//

#ifndef EXCEPTION_HANDLER_H
#define EXCEPTION_HANDLER_H

#include "httplib.h"
#include "common/Logger.h"
#include "nlohmann/json.hpp"
#include <stdexcept>
#include <string>
#include <functional>

using json = nlohmann::json;

/**
 * @brief 应用程序异常基类
 * 所有应用程序特定异常的基类
 */
class AppException : public std::runtime_error {
protected:
    int error_code;
    std::string error_type;

public:
    AppException(const std::string& message, int code, const std::string& type)
            : std::runtime_error(message), error_code(code), error_type(type) {}

    int getErrorCode() const { return error_code; }
    const std::string& getErrorType() const { return error_type; }
};

/**
 * @brief 配置异常
 * 当配置加载或访问出错时抛出
 */
class ConfigException : public AppException {
public:
    ConfigException(const std::string& message)
            : AppException(message, 500, "Configuration Error") {}
};

/**
 * @brief API异常
 * 当API请求处理出错时抛出
 */
class APIException : public AppException {
public:
    APIException(const std::string& message, int code = 400)
            : AppException(message, code, "API Error") {}
};

/**
 * @brief JSON解析异常
 * 当JSON解析出错时抛出
 */
class JSONParseException : public AppException {
public:
    JSONParseException(const std::string& message)
            : AppException(message, 400, "JSON Parse Error") {}
};

/**
 * @brief 模型异常
 * 当模型初始化或使用过程中出错时抛出
 */
class ModelException : public AppException {
public:
    ModelException(const std::string& message, const std::string& model_name = "")
            : AppException(createMessage(message, model_name), 500, "Model Error") {}

private:
    static std::string createMessage(const std::string& message, const std::string& model_name) {
        if (!model_name.empty()) {
            return "Model '" + model_name + "' Error: " + message;
        }
        return "Model Error: " + message;
    }
};

/**
 * @brief 全局异常处理工具类
 * 提供全局异常处理功能
 */
class ExceptionHandler {
public:
    /**
     * @brief 处理HTTP请求过程中的异常
     * @param req HTTP请求
     * @param res HTTP响应
     * @param func 要执行的处理函数
     */
    static void handleRequest(
            const httplib::Request& req,
            httplib::Response& res,
            std::function<void(const httplib::Request&, httplib::Response&)> func);

    /**
     * @brief 将异常转换为标准化的错误响应
     * @param res HTTP响应
     * @param e 异常
     * @param req HTTP请求（可选）
     */
    static void setErrorResponse(httplib::Response& res, const std::exception& e, const httplib::Request* req = nullptr);

    /**
     * @brief 处理执行操作过程中的异常
     * @param operation 操作描述
     * @param func 要执行的函数
     * @return 是否执行成功
     */
    static bool execute(const std::string& operation, std::function<void()> func);
};

#endif // EXCEPTION_HANDLER_H