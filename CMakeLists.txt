cmake_minimum_required(VERSION 3.0)
project(ThreadPool)
set(CMAKE_CXX_STANDARD 20) # 2a

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(./threadpool)
aux_source_directory(${PROJECT_SOURCE_DIR}/threadpool SRC_POOL)
add_subdirectory(test)
