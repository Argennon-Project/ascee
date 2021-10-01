project(tests)
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

# adding the Google_Tests_run target
add_executable(Google_Tests_run ../src/argc/runtime/string.cpp ArgcStringDeathTest.cpp)

# linking Google_Tests_run with DateConverter_lib which will be tested
# target_link_libraries(Google_Tests_run DateConverter_lib)

target_link_libraries(Google_Tests_run gtest gtest_main stdc++ m)
