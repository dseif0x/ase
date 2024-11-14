cmake_minimum_required(VERSION 3.22)
project(db)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(db_static ${SOURCES} ${SOURCES_PLATFORM})
target_link_libraries(db_static PUBLIC core_static)
target_include_directories(db_static PUBLIC include)
add_executable(db_test main.cpp)
target_link_libraries(db_test db_static)
