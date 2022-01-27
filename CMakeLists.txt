cmake_minimum_required(VERSION 3.20)
project(argennon_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

include_directories(include src src/ascee)

add_subdirectory(src/ascee)
add_subdirectory(src/util)
add_subdirectory(src/validator)
add_subdirectory(src/storage)
add_subdirectory(tests)

add_library(app1 SHARED arg.cpp)
add_library(app2 SHARED app2.cpp)

set_target_properties(app1 app2
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY compiled)

add_executable(ascee_run src/main.cpp)

target_link_libraries(ascee_run ascee asa ave argutil stdc++ pthread rt pbc gmp crypto dl)
