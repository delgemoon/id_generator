cmake_minimum_required(VERSION 3.21)
project(ticker)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(ASIO_ENABLE_HANDLER_TRACKING)
include_directories(../../workspace/asio/asio/include)
add_executable(ticker main.cpp)
