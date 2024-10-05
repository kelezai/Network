cmake_minimum_required(VERSION 3.5)
project(demo)

set(CMAKE_CXX_STANDARD 20)
set(RX_ROOT ${PROJECT_SOURCE_DIR})
# set(CMAKE_CXX_FLAGS "-Wall -Wreorder -g -O0 -pthread -fsanitize=address")
set(CMAKE_CXX_FLAGS "-Wall -Wreorder -g -O0 -pthread -fsanitize=undefined")

add_subdirectory(server_reactor)
add_subdirectory(client)

include_directories(${PROJECT_SOURCE_DIR}/inc)

file(GLOB SRC_LIST ${PROJECT_SOURCE_DIR}/src/*.cc ${PROJECT_SOURCE_DIR}/server_18/thread_pool.cc)

set(EXECUTABLE_OUTPUT_PATH ${RX_ROOT}/bin)

add_executable(demo ${SRC_LIST})