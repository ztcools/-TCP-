cmake_minimum_required(VERSION 3.10)
project(tcp_server_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/../include
)

# 添加日志测试可执行文件
add_executable(test_log test_log.cpp ../src/util/log.cpp)
target_link_libraries(test_log pthread)

# 添加线程池测试可执行文件
add_executable(test_thread_pool test_thread_pool.cpp ../src/util/thread_pool.cpp)
target_link_libraries(test_thread_pool pthread)

# 添加内存池测试可执行文件
add_executable(test_memory_pool test_memory_pool.cpp ../src/util/memory_pool.cpp)
target_link_libraries(test_memory_pool pthread)

# 添加Socket测试可执行文件
add_executable(test_socket test_socket.cpp ../src/net/socket.cpp ../src/util/log.cpp)
target_link_libraries(test_socket pthread)

# 添加测试
add_test(NAME LoggerTest COMMAND test_log)
add_test(NAME ThreadPoolTest COMMAND test_thread_pool)
add_test(NAME MemoryPoolTest COMMAND test_memory_pool)
add_test(NAME SocketTest COMMAND test_socket)
