cmake_minimum_required(VERSION 3.20)
project(argennon_utilities)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

add_library(argutil STATIC crypto/CryptoSystem.cpp)
