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

set(exception
        include/exception/GlobalExceptionHandler.h
        src/exception/GlobalExceptionHandler.cpp

)

# 所有库源文件
set(LIB_SOURCES
        ${handlers}
        ${common}
        ${app}
        ${route}
        ${exception}
)

# 创建静态库 (可根据需要改为 SHARED 库)
add_library(58ai_processor SHARED ${LIB_SOURCES})

# 为库设置属性
set_target_properties(58ai_processor PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
)

# 链接库依赖
target_link_libraries(58ai_processor PRIVATE
        ws2_32
        pthread
)


add_executable(http_model main.cpp
)

#add_library(DynLibName STATIC src/handlers/api_handler.cpp) # Output dynamic library.

target_link_libraries(http_model PRIVATE 58ai_processor ws2_32 pthread)
#target_link_libraries(http_model PRIVATE ws2_32 pthread)