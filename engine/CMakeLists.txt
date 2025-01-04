cmake_minimum_required(VERSION 3.22)
project(ase)

set(CMAKE_CXX_STANDARD 23)
option(ASE_BUILD_DEMO "Build demos" OFF)

# gTest
include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_MakeAvailable(googletest)


add_subdirectory(core)
add_subdirectory(db)
add_subdirectory(server)


if (ASE_BUILD_DEMO)
    add_subdirectory(demo)
endif()