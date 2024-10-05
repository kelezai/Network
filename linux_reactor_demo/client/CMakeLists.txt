cmake_minimum_required(VERSION 3.5)
project(x-client)

include_directories(${RX_ROOT}/inc)

# file(GLOB SRC_LIST ${PROJECT_SOURCE_DIR}/bankclient.cc)
file(GLOB SRC_LIST ${PROJECT_SOURCE_DIR}/echoclient.cc)

set(EXECUTABLE_OUTPUT_PATH ${RX_ROOT}/bin)

add_executable(x-client ${SRC_LIST})