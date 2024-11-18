cmake_minimum_required(VERSION 3.22)
project(ase)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(core)
add_subdirectory(db)
add_subdirectory(server)