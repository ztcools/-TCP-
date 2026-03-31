cmake_minimum_required(VERSION 3.10)
project(tcp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# 源文件
file(GLOB_RECURSE SOURCES
    "src/util/*.cpp"
    "src/net/*.cpp"
    "src/conn/*.cpp"
    "src/service/*.cpp"
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    src/main.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    pthread
)

# 测试
enable_testing()
add_subdirectory(test)
