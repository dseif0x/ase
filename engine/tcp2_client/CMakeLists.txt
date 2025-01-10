cmake_minimum_required(VERSION 3.22)
project(client)

set(CMAKE_CXX_STANDARD 23)
file(GLOB SOURCES "src/*.cpp")
add_library(client_static ${SOURCES} ${SOURCES_PLATFORM})
if (WIN32)
    target_compile_definitions(client_static PRIVATE WIN)
    file(GLOB SOURCES_PLATFORM "src/win/*.cpp")
elseif(UNIX)
    target_compile_definitions(client_static PRIVATE UNIX)
    file(GLOB SOURCES_PLATFORM "src/posix/*.cpp")
else ()
    message(FATAL_ERROR "Unsupported platform")
endif()


target_link_libraries(client_static PUBLIC core_static)
target_link_libraries(client_static PUBLIC db_static)
target_link_libraries(client_static PUBLIC server_static)
target_include_directories(client_static PUBLIC include)

add_executable(tcp2_dumb_client main.cpp)
target_link_libraries(tcp2_dumb_client client_static)