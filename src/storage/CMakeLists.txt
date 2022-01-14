cmake_minimum_required(VERSION 3.20)
project(argennon_storage_adaptor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

add_library(asa STATIC
        PageLoader.cpp
        ChunkIndex.cpp
        PageCache.cpp
        Page.cpp)
