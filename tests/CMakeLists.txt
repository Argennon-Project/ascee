# 'test' is the subproject name
project(tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)


# adding the Google_Tests_run target
add_executable(Google_Tests_run
        ascee/ArgcStringTest.cpp
        ascee/HeapModifierDeathTest.cpp
        ascee/AsceeVSigManagerTest.cpp
        ascee/HeapChunkTest.cpp
        ascee/ArgcFloatTest.cpp
        util/PrefixTrieTest.cpp
        util/UtilCryptoTest.cpp
        util/UtilBase64urlTest.cpp
        validator/RequestSchedulerTest.cpp
        apps/ArgAppTest.cpp storage/AsaPageTest.cpp)


# linking Google_Tests_run with libraries
target_link_libraries(Google_Tests_run gmock gtest gtest_main m ascee asa ave argutil stdc++ pthread rt pbc gmp crypto dl)

add_executable(Google_Mock_run
        ascee/AsceeExecutorTest.cpp)
target_link_libraries(Google_Mock_run gmock gtest gtest_main m ascee_mock argutil stdc++ pthread rt pbc gmp crypto dl)
target_compile_definitions(Google_Mock_run PUBLIC ASCEE_MOCK_BUILD)
