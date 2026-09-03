cmake_minimum_required(VERSION 3.16)
project(camengine_jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引用根目录 engine 静态库
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../engine
                 ${CMAKE_BINARY_DIR}/engine)

# JNI 桥接代码
add_library(camengine_jni SHARED
    jni_interface.cpp
)

target_include_directories(camengine_jni PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../engine/include
)

target_link_libraries(camengine_jni PRIVATE
    camengine
    log
    android
)