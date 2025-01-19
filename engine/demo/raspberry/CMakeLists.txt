cmake_minimum_required(VERSION 3.22)
project(rbpi-demo)

set(CMAKE_CXX_STANDARD 23)

add_executable(rbpi_producer producer.cpp)
target_link_libraries(rbpi_producer db_static)
target_include_directories(rbpi_producer PRIVATE include)

add_executable(rbpi_consumer consumer.cpp)
target_link_libraries(rbpi_consumer db_static)
target_include_directories(rbpi_consumer PRIVATE include)

add_executable(rbpi_cleanup cleanup.cpp)
target_link_libraries(rbpi_cleanup db_static)
target_include_directories(rbpi_cleanup PRIVATE include)