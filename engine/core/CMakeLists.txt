cmake_minimum_required(VERSION 3.22)
project(core)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCES "src/*.cpp")
if (WIN32)
    file(GLOB SOURCES_PLATFORM "src/win/*.cpp")
elseif(UNIX)
    file(GLOB SOURCES_PLATFORM "src/posix/*.cpp")
else ()
    message(FATAL_ERROR "Unsupported platform")
endif()


add_library(core_static ${SOURCES} ${SOURCES_PLATFORM})
target_include_directories(core_static PUBLIC include)
add_executable(core_test main.cpp)
target_link_libraries(core_test core_static)
#add_executable(tcp_test tcp_test.cpp)
#target_link_libraries(tcp_test core_static)
