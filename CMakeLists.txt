cmake_minimum_required(VERSION 3.20)
project(ascee)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

include_directories(include src ~/pbc-0.5.14/include)

add_subdirectory(tests)

add_library(app1 SHARED app1.cpp)
add_library(app2 SHARED app2.cpp)

add_library(argcrt SHARED src/argc/runtime/string.cpp src/argc/runtime/invoke.cpp src/heap/PageCache.h src/heap/Modifier.cpp src/argc/runtime/heapAccess.cpp src/heap/PageCache.cpp src/argc/runtime/float.cpp src/crypto/CryptoSystem.cpp src/crypto/CryptoSystem.h src/executor/FailureManager.cpp src/executor/FailureManager.h src/heap/Chunk.cpp src/heap/Chunk.h src/heap/Page.cpp src/heap/Page.h src/util/IdentifierTrie.h src/util/StringBuffer.h src/util/StringBuffer.cpp src/util/StaticArray.h src/util/VirtualSigManager.cpp src/util/VirtualSigManager.h src/argc/runtime/auth.cpp src/crypto/Keys.h src/util/BlockingQueue.cpp src/util/BlockingQueue.h src/heap/Modifier.h src/crypto/DigestCalculator.cpp src/crypto/DigestCalculator.h src/util/FixedOrderedMap.hpp)
target_link_libraries(argcrt pthread rt stdc++ crypto gmp pbc)

add_executable(ascee src/main.cpp src/loader/AppLoader.cpp src/loader/AppLoader.h include/argc/types.h src/executor/ThreadCpuTimer.cpp src/executor/ThreadCpuTimer.h src/executor/Executor.cpp src/executor/Executor.h src/executor/RequestScheduler.cpp src/executor/RequestScheduler.h src/loader/BlockLoader.cpp src/loader/BlockLoader.h src/BlockValidator.cpp src/BlockValidator.h src/loader/PageLoader.cpp src/loader/PageLoader.h)
target_link_libraries(ascee pbc gmp crypto dl stdc++ argcrt app1 app2)

