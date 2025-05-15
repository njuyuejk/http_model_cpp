//
// Created by YJK on 2025/1/2.
//

#ifndef HTTP_MODEL_API_HANDLER_H
#define HTTP_MODEL_API_HANDLER_H

#include "httplib.h"

namespace Handlers {
    void handle_api_data(const httplib::Request& req, httplib::Response& res);
}

#endif //HTTP_MODEL_API_HANDLER_H
