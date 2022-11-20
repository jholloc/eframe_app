cmake_minimum_required(VERSION 3.23)
project(convolution)

set(CMAKE_CXX_STANDARD 20)

add_executable(convolution main.cpp)
target_compile_definitions(convolution PRIVATE THREADS)
