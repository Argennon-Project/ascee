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

file(COPY param DESTINATION .)
file(COPY param DESTINATION tests)

file(COPY apps DESTINATION .)
file(COPY include/arg DESTINATION include)
file(COPY src/ascee/argc/types.h DESTINATION include/argc)
file(COPY src/ascee/argc/functions.h DESTINATION include/argc)
file(COPY src/ascee/argc/StringBuffer.h DESTINATION include)
file(COPY src/util/StaticArray.hpp DESTINATION include/util)
file(COPY src/util/PrefixTrie.hpp DESTINATION include/util)
file(COPY src/util/encoding.h DESTINATION include/util)
file(COPY src/util/crypto/Keys.h DESTINATION include/util/crypto)

file(COPY tests/testdata DESTINATION tests)

add_subdirectory(src/ascee)
add_subdirectory(src/util)
add_subdirectory(src/validator)
add_subdirectory(src/storage)
add_subdirectory(tests)

add_executable(ascee_run src/main.cpp apps/app0x100000000000000.cpp)
target_link_libraries(ascee_run ascee asa ave argutil stdc++ pthread rt pbc gmp crypto dl)

add_executable(signer src/signer.cpp)
target_link_libraries(signer argutil stdc++ rt pbc gmp crypto dl)