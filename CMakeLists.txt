cmake_minimum_required(VERSION 3.22.1)
project(tcp_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG_BUILD")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB_RECURSE SERVER_SOURCES
    ${PROJECT_SOURCE_DIR}/src/util/*.cpp
    ${PROJECT_SOURCE_DIR}/src/net/*.cpp
    ${PROJECT_SOURCE_DIR}/src/conn/*.cpp
    ${PROJECT_SOURCE_DIR}/src/service/*.cpp
)

list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*test.*")

list(APPEND SERVER_SOURCES ${PROJECT_SOURCE_DIR}/src/main.cpp)

add_executable(tcp_server ${SERVER_SOURCES})

target_link_libraries(tcp_server
    pthread
    rt
)

target_compile_options(tcp_server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(tcp_server PRIVATE DEBUG_BUILD)
endif()

message(STATUS "==================================================")
message(STATUS "TCP Server Build Configuration")
message(STATUS "==================================================")
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source Files: ${SERVER_SOURCES}")
message(STATUS "==================================================")

# Only add test directory if it exists
if(EXISTS ${PROJECT_SOURCE_DIR}/test AND IS_DIRECTORY ${PROJECT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif()
