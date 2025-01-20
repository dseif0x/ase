cmake_minimum_required(VERSION 3.22)
project(redis-bench)

set(CMAKE_CXX_STANDARD 23)

CPMAddPackage("gh:redis/hiredis#v1.2.0")

add_executable(bench_dma main_dma.cpp)
target_link_libraries(bench_dma db_static)
target_include_directories(bench_dma PRIVATE include)

add_executable(bench_dma_parallel main_dma_parallel.cpp)
target_link_libraries(bench_dma_parallel db_static)
target_include_directories(bench_dma_parallel PRIVATE include)

add_executable(bench_redis main_redis.cpp)
target_link_libraries(bench_redis hiredis db_static)
target_include_directories(bench_redis PRIVATE include)

add_executable(bench_redis_parallel main_redis_parallel.cpp)
target_link_libraries(bench_redis_parallel hiredis db_static)
target_include_directories(bench_redis_parallel PRIVATE include)

add_executable(bench_tcp2 main_tcp2.cpp)
target_link_libraries(bench_tcp2 client_static)
target_include_directories(bench_tcp2 PRIVATE include)

add_executable(bench_tcp2_parallel main_tcp2_parallel.cpp)
target_link_libraries(bench_tcp2_parallel client_static)
target_include_directories(bench_tcp2_parallel PRIVATE include)

add_executable(bench_tcp2_buff main_tcp2_buffered.cpp)
target_link_libraries(bench_tcp2_buff client_static)
target_include_directories(bench_tcp2_buff PRIVATE include)

add_executable(bench_tcp2_bat main_tcp2_batching.cpp)
target_link_libraries(bench_tcp2_bat client_static)
target_include_directories(bench_tcp2_bat PRIVATE include)

add_executable(bench_tcp2_buff_parallel main_tcp2_buffered_parallel.cpp)
target_link_libraries(bench_tcp2_buff_parallel client_static)
target_include_directories(bench_tcp2_buff_parallel PRIVATE include)


if(NOT TARGET bench)
    add_custom_target(bench)
    add_dependencies(bench bench_dma bench_dma_parallel bench_redis bench_redis_parallel bench_tcp2 bench_tcp2_parallel bench_tcp2_buff bench_tcp2_buff_parallel)
endif()