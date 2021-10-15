project(tests)
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

add_library(app10 SHARED appfiles/single-thread/timeout/app10.c)
add_library(app12 SHARED appfiles/single-thread/timeout/app12.c)

add_library(app11 SHARED appfiles/single-thread/call/app11.c)
add_library(app15 SHARED appfiles/single-thread/call/app15.c)
add_library(app16 SHARED appfiles/single-thread/call/app16.c)

add_library(app13 SHARED appfiles/single-thread/stack-overflow/app13.c)
add_library(app14 SHARED appfiles/single-thread/stack-overflow/app14.c)

add_library(app17 SHARED appfiles/single-thread/circular/app17.c)
add_library(app18 SHARED appfiles/single-thread/circular/app18.c)

add_library(app19 SHARED appfiles/single-thread/failed-call/app19.c)
add_library(app20 SHARED appfiles/single-thread/failed-call/app20.c)
add_library(app21 SHARED appfiles/single-thread/failed-call/app21.c)

add_library(app22 SHARED appfiles/single-thread/reentrancy/app22.c)
add_library(app23 SHARED appfiles/single-thread/reentrancy/app23.c)


set_target_properties(app10 app11 app12 app13 app14 app15 app16 app17 app18 app19 app20 app21 app22 app23 PROPERTIES LIBRARY_OUTPUT_DIRECTORY appfiles/compiled)


# adding the Google_Tests_run target
add_executable(Google_Tests_run ../src/argc/runtime/string.cpp ../src/Executor.cpp ../src/ThreadCpuTimer.cpp ../src/loader/AppLoader.cpp ArgcStringDeathTest.cpp AsceeExecutorTest.cpp HeapModifierDeathTest.cpp)

# linking Google_Tests_run with DateConverter_lib which will be tested
# target_link_libraries(Google_Tests_run DateConverter_lib)

target_link_libraries(Google_Tests_run gtest gtest_main stdc++ m argcrt dl argcrt app10 app11 app12 app13 app14 app15 app16 app17 app18 app19 app20 app21 app22 app23)
