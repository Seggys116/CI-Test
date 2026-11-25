cmake_minimum_required(VERSION 3.21)

project(ciutil_consumer LANGUAGES CXX)

find_package(ciutil 1.0 CONFIG REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE ciutil::ciutil)
