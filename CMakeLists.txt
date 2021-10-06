cmake_minimum_required(VERSION 3.16)
project(simdisk)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(simdisk src/main.cpp)
add_executable(test src/test.cpp)
