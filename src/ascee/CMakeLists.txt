cmake_minimum_required(VERSION 3.20)
project(ascee)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

# Should be compiled as shared lib so apps can find argc functions.
add_library(ascee SHARED
        executor/ThreadCpuTimer.cpp
        executor/Executor.cpp
        executor/FailureManager.cpp
        executor/VirtualSigManager.cpp
        loader/AppLoader.cpp
        argc/functions/string.cpp
        argc/functions/invoke.cpp
        argc/functions/auth.cpp
        argc/functions/float.cpp
        argc/functions/heapAccess.cpp
        argc/StringBuffer.cpp
        heap/Chunk.cpp
        heap/RestrictedModifier.cpp
        executor/AppTable.cpp)
