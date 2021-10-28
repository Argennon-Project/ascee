cmake_minimum_required(VERSION 3.20)
project(ascee)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

include_directories(include src)

add_subdirectory(tests)

add_library(app1 SHARED app1.c)
add_library(app2 SHARED app2.c)

add_library(argcrt SHARED src/argc/runtime/string.cpp src/argc/runtime/invoke.cpp src/heap/Pointer.cpp src/heap/Heap.h src/heap/Modifier.cpp src/argc/runtime/heapAccess.cpp src/heap/Heap.cpp src/argc/runtime/float.cpp src/loader/FailureManager.cpp src/loader/FailureManager.h)
target_link_libraries(argcrt pthread rt stdc++)

add_executable(ascee src/main.cpp src/loader/AppLoader.cpp src/loader/AppLoader.h include/argc/types.h src/ThreadCpuTimer.cpp src/ThreadCpuTimer.h src/Executor.cpp src/Executor.h)
target_link_libraries(ascee dl stdc++ argcrt app1 app2)
