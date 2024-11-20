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
target_link_libraries(server_static PUBLIC core_static)
# todo, add db_static to library
#target_link_libraries(server_static PUBLIC db_static)
target_include_directories(server_static PUBLIC include)

add_executable(tcp_test tcp_test.cpp
        src/decoder.cpp
        include/decoder.h)
target_link_libraries(tcp_test server_static)
