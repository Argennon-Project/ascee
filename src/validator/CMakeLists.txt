cmake_minimum_required(VERSION 3.20)
project(argennon_validation_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

add_library(ave STATIC
        BlockLoader.cpp
        BlockValidator.cpp
        RequestScheduler.cpp)
