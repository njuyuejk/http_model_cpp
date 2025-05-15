cmake_minimum_required(VERSION 3.28)
project(http_model)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

set(common
        include/common/StreamConfig.h
        include/common/Logger.h
        src/common/Logger.cpp
        src/common/StreamConfig.cpp
)

set(app
        include/app/ApplicationManager.h
        src/app/ApplicationManager.cpp

)

set(route
        include/routeManager/HttpServer.h
        src/routeManager/HttpServer.cpp
        include/routeManager/RouteManager.h
        src/routeManager/RouteManager.cpp
        include/routeManager/BasicRoutes.h
        include/routeManager/UserRoutes.h
        include/routeManager/ApiRoutes.h
        include/routeManager/RouteInitializer.h
)

set(handlers
        src/handlers/root_handler.cpp
        include/handlers/root_handler.h
        src/handlers/user_handler.cpp
        include/handlers/user_handler.h
        src/handlers/api_handler.cpp
        include/handlers/api_handler.h
)

add_executable(http_model main.cpp
        ${handlers}
        ${common}
        ${app}
        ${route}
)

find_package(Threads REQUIRED)

#add_library(DynLibName STATIC src/handlers/api_handler.cpp) # Output dynamic library.

target_link_libraries(http_model PRIVATE ${handlers} ${common} ${app} ${route} ws2_32 pthread)
