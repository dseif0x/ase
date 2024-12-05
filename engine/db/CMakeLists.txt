cmake_minimum_required(VERSION 3.22)
project(db)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(db_static STATIC ${SOURCES} ${SOURCES_PLATFORM}
        src/custom_map.cpp)
target_link_libraries(db_static PUBLIC core_static)
target_include_directories(db_static PUBLIC include)
add_executable(db_test main.cpp)
target_link_libraries(db_test db_static)

#gTest
enable_testing()
file(GLOB TEST_SOURCES "tests/*.cpp")
add_executable(gtest_db ${TEST_SOURCES})
target_link_libraries(gtest_db PRIVATE db_static GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(gtest_db)