cmake_minimum_required(VERSION 3.22)
project(redis-bench)

set(CMAKE_CXX_STANDARD 23)

CPMAddPackage("gh:redis/hiredis#v1.2.0")

add_executable(ase_bench main_ase.cpp)
target_link_libraries(ase_bench db_static)
target_include_directories(ase_bench PRIVATE include)

add_executable(redis_bench main_redis.cpp)
target_link_libraries(redis_bench hiredis db_static)
target_include_directories(redis_bench PRIVATE include)