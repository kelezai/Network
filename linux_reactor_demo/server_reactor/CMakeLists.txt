cmake_minimum_required(VERSION 3.5)
project(x-server)

include_directories(${RX_ROOT}/inc)

file(GLOB SRC_LIST ${PROJECT_SOURCE_DIR}/*.cc)

set(EXECUTABLE_OUTPUT_PATH ${RX_ROOT}/bin)

# list(REMOVE_ITEM SRC_LIST 
#     ${PROJECT_SOURCE_DIR}/echoserver.cc
#     ${PROJECT_SOURCE_DIR}/echo_server.cc)

list(REMOVE_ITEM SRC_LIST 
    ${PROJECT_SOURCE_DIR}/bankserver.cc
    ${PROJECT_SOURCE_DIR}/bank_server.cc)

add_executable(x-server ${SRC_LIST})