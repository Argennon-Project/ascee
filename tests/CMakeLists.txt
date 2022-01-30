# 'test' is the subproject name
project(tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

# adding the Google_Tests_run target
add_executable(Google_Tests_run
        ArgcStringTest.cpp
        HeapModifierDeathTest.cpp
        ArgcFloatTest.cpp
        #AsceeHeapDeathTest.cpp
        AsceeIdentifiersTest.cpp
        AsceeCryptoTest.cpp
        AsceeVSigManagerTest.cpp
        RequestSchedulerTest.cpp
        HeapChunkTest.cpp
        UtilBase64urlTest.cpp)

# linking Google_Tests_run with libraries

target_link_libraries(Google_Tests_run gmock gtest gtest_main m ascee asa ave argutil stdc++ pthread rt pbc gmp crypto dl)

add_executable(Google_Mock_run AsceeExecutorTest.cpp)
target_link_libraries(Google_Mock_run gmock gtest gtest_main m ascee_mock argutil stdc++ pthread rt pbc gmp crypto dl)
target_compile_definitions(Google_Mock_run PUBLIC ASCEE_MOCK_BUILD)
