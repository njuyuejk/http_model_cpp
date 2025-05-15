//
// Created by YJK on 2025/1/2.
//

#include "../../include/handlers/root_handler.h"

void Handlers::handle_root(const httplib::Request& req, httplib::Response& res) {
    res.set_content("Welcome to the C++ HTTP Server!", "text/plain");
}