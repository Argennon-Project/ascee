cmake_minimum_required(VERSION 3.20)
project(ArgennonCore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929 # release-1.11.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# include directories
include_directories(include src src/ascee)
include_directories(tests ${gtest_SOURCE_DIR}/include ${gmock_SOURCE_DIR}/include)

file(COPY resources DESTINATION .)
file(COPY resources DESTINATION tests)

file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/apps)
file(COPY apps DESTINATION .)

file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/tests/testdata)
file(COPY tests/testdata DESTINATION tests)

configure_file(include/core/primitives.h include/core/primitives.h COPYONLY)
configure_file(src/ascee/argc/types.h include/argc/types.h COPYONLY)
configure_file(src/ascee/argc/functions.h include/argc COPYONLY)
configure_file(src/ascee/argc/StringBuffer.h include COPYONLY)
configure_file(src/util/StaticArray.hpp include/util/StaticArray.hpp COPYONLY)
configure_file(src/util/encoding.h include/util COPYONLY)
configure_file(src/util/crypto/Keys.h include/util/crypto/Keys.h COPYONLY)

add_subdirectory(src/ascee)
add_subdirectory(src/util)
add_subdirectory(src/validator)
add_subdirectory(src/storage)
add_subdirectory(tests)

#add_compile_options(-Wall -Og)

add_executable(ascee_run src/main.cpp)
target_link_libraries(ascee_run ave asa ascee argutil stdc++ pthread rt pbc gmp crypto dl)

add_executable(signer src/signer.cpp)
target_link_libraries(signer argutil stdc++ rt pbc gmp crypto dl)