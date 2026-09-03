# CamEngine 引擎构建脚本
# 编译为静态库 libcamengine.a，由各平台桥接层链接。
cmake_minimum_required(VERSION 3.16)
project(camengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENGINE_SOURCES
    src/engine.cpp
)

add_library(camengine STATIC ${ENGINE_SOURCES})

target_include_directories(camengine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ANDROID)
    # Android 下禁用 RTTI/异常以减小体积（如需可调整）
    target_compile_options(camengine PRIVATE -fno-rtti -fno-exceptions)
endif()
