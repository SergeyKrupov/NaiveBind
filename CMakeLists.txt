cmake_minimum_required(VERSION 3.6)
project(NaiveBind)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp)
add_executable(NaiveBind ${SOURCE_FILES})