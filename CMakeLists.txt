cmake_minimum_required(VERSION 3.16)
project(simdisk)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "-m32")
#set(CMAKE_C_FLAGS "-m32")

find_package(fmt CONFIG REQUIRED)
find_package(Threads)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(simdisk src/main.cpp)
#set_target_properties(simdisk PROPERTIES COMPILE_OPTIONS "-m32")
add_executable(test src/test.cpp)
#
add_executable(server src/server.cpp)
#
add_executable(shell src/shell.cpp)

add_executable(init src/init.cpp)
#set_target_properties(test PROPERTIES COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32")

target_link_libraries(simdisk fmt::fmt Threads::Threads)

#target_link_libraries(test fmt::fmt Threads::Threads)

target_link_libraries(init fmt::fmt)


