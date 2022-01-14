cmake_minimum_required(VERSION 3.20)
project(ascee)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

include_directories(include src ~/pbc-0.5.14/include)

add_subdirectory(tests)

add_library(app1 SHARED app1.cpp)
add_library(app2 SHARED app2.cpp)

set_target_properties(app1 app2
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY compiled)

# Should be compiled as shared lib so apps can find argc functions.
add_library(ascee SHARED
        src/BlockValidator.cpp
        src/executor/ThreadCpuTimer.cpp
        src/executor/Executor.cpp
        src/executor/RequestScheduler.cpp
        src/executor/FailureManager.cpp
        src/loader/AppLoader.cpp
        src/loader/BlockLoader.cpp
        src/loader/PageLoader.cpp
        src/argc/runtime/string.cpp
        src/argc/runtime/invoke.cpp
        src/argc/runtime/auth.cpp
        src/argc/runtime/float.cpp
        src/argc/runtime/heapAccess.cpp
        src/heap/Chunk.cpp
        src/heap/Modifier.cpp
        src/heap/PageCache.cpp
        src/heap/Page.cpp
        src/crypto/CryptoSystem.cpp
        src/crypto/DigestCalculator.cpp
        src/util/StringBuffer.cpp
        src/util/VirtualSigManager.cpp
        src/heap/ChunkIndex.cpp)

add_executable(ascee_run
        src/main.cpp
        )

target_link_libraries(ascee stdc++ pthread rt pbc gmp crypto dl)

target_link_libraries(ascee_run ascee)

