cmake_minimum_required(VERSION 3.20)
project(ascee)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

add_subdirectory(tests)

add_library(app1 SHARED app1.c)
add_library(app2 SHARED app2.c)

add_library(argcrt SHARED src/argc/runtime/string.cpp src/argc/runtime/invoke.cpp src/heap/Heap.cpp src/heap/Heap.h src/heap/HeapModifier.cpp src/heap/HeapModifier.h src/argc/runtime/heapAccess.cpp src/argc/runtime/session.h)
target_link_libraries(argcrt pthread rt stdc++)

add_executable(ascee src/main.cpp src/loader/AppLoader.cpp src/loader/AppLoader.h include/argc/types.h src/ThreadCpuTimer.cpp src/ThreadCpuTimer.h)
target_link_libraries(ascee dl stdc++ argcrt app1 app2)
