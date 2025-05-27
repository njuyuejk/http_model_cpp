cmake_minimum_required(VERSION 3.28)
project(http_model)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories("D:/project/C++/my/ffmpeg_push_pull/protobuf/include")
include_directories("D:/project/C++/grpc-1.70.2/include")

# 设置 opencv 的根目录
set(OPENCV_ROOT "D:/AI/opencv/build/install")

# 添加包含目录
include_directories("${OPENCV_ROOT}/include")

# 设置库目录
link_directories("${OPENCV_ROOT}/x64/mingw/lib")

# 要链接的 FFmpeg 库
set(OPENCV_LIBS
        opencv_world470
        opencv_img_hash470
)


# 查找 FFmpeg 库
foreach(lib ${OPENCV_LIBS})
    find_library(${lib}_LIBRARY NAMES ${lib} PATHS "${OPENCV_ROOT}/x64/mingw/lib")
    if(NOT ${lib}_LIBRARY)
        message(FATAL_ERROR "FFmpeg library ${lib} not found in ${OPENCV_ROOT}/x64/mingw/lib")
    endif()
    list(APPEND FOUND_OPENCV_LIBS ${${lib}_LIBRARY})
endforeach()


# rknn api
#set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/RKNN/RK3588/${CMAKE_SYSTEM_NAME}/librknn_api)
set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/RKNN/RK3588/Linux/librknn_api)
set(LIB_ARCH aarch64)
set(RKNN_RT_LIB "${CMAKE_SOURCE_DIR}/RKNN/RK3588/Linux/librknn_api/aarch64/librknnrt.so")
include_directories("${RKNN_API_PATH}/include")
include_directories("${CMAKE_SOURCE_DIR}/include/3rdparty")

#rga
set(RGA_PATH ${CMAKE_SOURCE_DIR}/include/3rdparty/rga/RK3588)
set(LIB_ARCH aarch64)
set(RGA_LIB ${RGA_PATH}/lib/Linux/${LIB_ARCH}/librga.so)
include_directories( ${RGA_PATH}/include)
set(CMAKE_INSTALL_RPATH "lib")


set(common
        include/common/StreamConfig.h
        include/common/Logger.h
        src/common/Logger.cpp
        src/common/StreamConfig.cpp
        include/common/base64.h
        src/common/base64.cpp
        include/common/utils.h
        src/common/utils.cpp
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
        include/routeManager/base/BasicRoutes.h
        include/routeManager/route/ModelConfigRoutes.h
        include/routeManager/route/ApiRoutes.h
        include/routeManager/base/RouteInitializer.h
        include/routeManager/route/ModelRoutes.h
        include/routeManager/route/StatusRoutes.h
)

set(handlers
        src/handlers/root_handler.cpp
        include/handlers/root_handler.h
        src/handlers/modelConfig_handler.cpp
        include/handlers/modelConfig_handler.h
        src/handlers/api_handler.cpp
        include/handlers/api_handler.h
        include/handlers/status_handler.h
        src/handlers/status_handler.cpp
)

set(exception
        include/exception/GlobalExceptionHandler.h
        src/exception/GlobalExceptionHandler.cpp

)

set(ai_service
        include/AIService/postprocess/postprocess.h
        include/AIService/rknn/rknnPool.h
        src/AIService/postprocess/postprocess.cpp
        src/AIService/rknn/rknnPool.cpp
        include/AIService/preprocess/preprocess.h
        src/AIService/preprocess/preprocess.cpp
        include/AIService/postprocess/lprnetPostprocess.h
        src/AIService/postprocess/lprnetPostprocess.cpp
        src/AIService/rknn/lprnetPool.cpp
        include/AIService/rknn/lprnetPool.h
        include/AIService/rknn/lprnetPool.h
        src/AIService/postprocess/postprocess_p2.cpp
        include/AIService/postprocess/postprocess_p2.h
        include/AIService/postprocess/postprocess_pose.h
        src/AIService/postprocess/postprocess_pose.cpp
        include/AIService/rknn/rknnPool_Pose.h
        src/AIService/rknn/rknnPool_Pose.cpp
        include/AIService/postprocess/postprocess_seg.h
        src/AIService/postprocess/postprocess_seg.cpp
        include/AIService/rknn/rknnPool_Seg.h
        src/AIService/rknn/rknnPool_Seg.cpp
        src/AIService/ModelPool.cpp
        include/AIService/ModelPool.h
)


# 所有库源文件
set(LIB_SOURCES
        ${handlers}
        ${common}
        ${app}
        ${route}
        ${exception}
        ${ai_service}
)

# 创建静态库 (可根据需要改为 SHARED 库)
add_library(58ai_http_processor SHARED ${LIB_SOURCES})

# 为库设置属性
set_target_properties(58ai_http_processor PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
)

# 链接库依赖
target_link_libraries(58ai_http_processor PRIVATE
        ${OpenCV_LIBS}
        ${RKNN_RT_LIB}
        ${RGA_LIB}
        ws2_32
        pthread
)


add_executable(http_model main.cpp
)

#add_library(DynLibName STATIC src/handlers/api_handler.cpp) # Output dynamic library.

target_link_libraries(http_model PRIVATE 58ai_http_processor ${OpenCV_LIBS} ${RKNN_RT_LIB} ${RGA_LIB} ws2_32 pthread)
#target_link_libraries(http_model PRIVATE ws2_32 pthread)