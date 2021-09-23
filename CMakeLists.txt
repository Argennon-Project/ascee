cmake_minimum_required(VERSION 3.20)
project(ascee)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 99)

add_library(argcrt SHARED invoker.cpp Heap.cpp Heap.h HeapModifier.cpp HeapModifier.h)
target_link_libraries(argcrt pthread rt stdc++ )

add_executable(ascee main.cpp AppLoader.cpp AppLoader.h argctypes.h)
target_link_libraries(ascee dl stdc++ argcrt)