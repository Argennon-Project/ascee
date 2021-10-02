project(tests)
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

add_library(app10 SHARED appfiles/1/app10.c)
set_target_properties(app10 PROPERTIES LIBRARY_OUTPUT_DIRECTORY appfiles/1)


# adding the Google_Tests_run target
add_executable(Google_Tests_run ../src/argc/runtime/string.cpp ../src/Executor.cpp ../src/ThreadCpuTimer.cpp ../src/loader/AppLoader.cpp ArgcStringDeathTest.cpp AsceeExecutorTest.cpp)

# linking Google_Tests_run with DateConverter_lib which will be tested
# target_link_libraries(Google_Tests_run DateConverter_lib)

target_link_libraries(Google_Tests_run gtest gtest_main stdc++ m argcrt dl argcrt app10)
