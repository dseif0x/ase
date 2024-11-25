cmake_minimum_required(VERSION 3.22)
project(ase)

set(CMAKE_CXX_STANDARD 23)

# gTest
include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)


add_subdirectory(core)
add_subdirectory(db)
add_subdirectory(server)