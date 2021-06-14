cmake_minimum_required(VERSION 3.16)

project(MINISQL)

include_directories(ext)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g")
aux_source_directory(src SRC_LIST)
add_executable(minisql ${SRC_LIST})