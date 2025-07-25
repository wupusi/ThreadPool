cmake_minimum_required(VERSION 3.0)
project(ThreadPool)

add_executable(testsync testsync.cpp ${SRC_POOL})
target_link_libraries(testsync pthread)