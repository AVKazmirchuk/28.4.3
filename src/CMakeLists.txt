cmake_minimum_required(VERSION 3.17)

add_executable(Project main.cpp ${OPERATION_INCLUDE_DIR}/header.h source.cpp)

target_include_directories(Project PUBLIC ${OPERATION_INCLUDE_DIR})