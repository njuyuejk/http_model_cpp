//
// Created by YJK on 2025/5/22.
//

#ifndef STATUS_HANDLER_H
#define STATUS_HANDLER_H

#include "httplib.h"

namespace Handlers {
    void handle_system_status(const httplib::Request& req, httplib::Response& res);
    void handle_model_pools_status(const httplib::Request& req, httplib::Response& res);
    void handle_concurrency_stats(const httplib::Request& req, httplib::Response& res);
}

#endif // STATUS_HANDLER_H
