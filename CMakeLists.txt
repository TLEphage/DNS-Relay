cmake_minimum_required(VERSION 3.10)
project(dnsrelay C)

# 指定 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/src)

# 收集所有 .c 文件作为源文件
file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.c)

# 添加可执行文件目标
add_executable(dnsrelay ${SRC_FILES})

# 平台相关库链接（Windows 需要 ws2_32）
if(WIN32)
    target_link_libraries(dnsrelay PRIVATE ws2_32)
endif()

# 构建后将 dnsrelay.txt 复制到构建目录
configure_file(${CMAKE_SOURCE_DIR}/dnsrelay.txt
               ${CMAKE_BINARY_DIR}/dnsrelay.txt COPYONLY)
