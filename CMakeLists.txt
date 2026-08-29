cmake_minimum_required(VERSION 3.13)

project(hello_webgpu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

find_package(Dawn REQUIRED)
add_executable(hello_webgpu main.cpp)

# Declare dependency on the dawn::webgpu_dawn library
target_link_libraries(hello_webgpu dawn::webgpu_dawn)