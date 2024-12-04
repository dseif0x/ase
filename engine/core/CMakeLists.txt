cmake_minimum_required(VERSION 3.22)
project(core)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCES "src/*.cpp")
if (WIN32)
    file(GLOB SOURCES_PLATFORM "src/win/*.cpp")
elseif(UNIX)
    file(GLOB SOURCES_PLATFORM "src/posix/*.cpp")
else ()
    message(FATAL_ERROR "Unsupported platform")
endif()


add_library(core_static STATIC ${SOURCES} ${SOURCES_PLATFORM})
target_include_directories(core_static PUBLIC include)
add_executable(core_test main.cpp)
target_link_libraries(core_test core_static)

#gTest
enable_testing()
file(GLOB TEST_SOURCES "tests/*.cpp")
add_executable(gtest_core ${TEST_SOURCES})
target_link_libraries(gtest_core PRIVATE core_static GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(gtest_core)