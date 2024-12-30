cmake_minimum_required(VERSION 3.22)
project(server)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCES "src/*.cpp")
if (WIN32)
    file(GLOB SOURCES_PLATFORM "src/win/*.cpp")
elseif(UNIX)
    file(GLOB SOURCES_PLATFORM "src/posix/*.cpp")
else ()
    message(FATAL_ERROR "Unsupported platform")
endif()


add_library(server_static ${SOURCES} ${SOURCES_PLATFORM})

if (WIN32)
    target_link_libraries(server_static PRIVATE ws2_32)
    target_compile_definitions(server_static PRIVATE WIN)
elseif(UNIX)
    target_compile_definitions(server_static PRIVATE UNIX)
else ()
    message(FATAL_ERROR "Unsupported platform")
endif()

target_link_libraries(server_static PUBLIC core_static)
target_link_libraries(server_static PUBLIC db_static)
target_include_directories(server_static PUBLIC include)

add_executable(tcp_test main.cpp)
target_link_libraries(tcp_test server_static)

#gTest
enable_testing()
file(GLOB TEST_SOURCES "tests/*.cpp")
add_executable(gtest_server ${TEST_SOURCES})
target_include_directories(gtest_server PUBLIC tests/include)
target_link_libraries(gtest_server PRIVATE server_static GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(gtest_server)