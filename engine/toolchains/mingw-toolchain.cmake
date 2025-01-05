include(FetchContent)

set(CMAKE_SYSTEM_NAME Windows) # Cross-compiling to Windows
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Detect the host platform
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(MINGW_FILE_NAME "llvm-mingw-20241217-ucrt-x86_64.zip")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(MINGW_FILE_NAME "llvm-mingw-20241217-ucrt-macos-universal.tar.xz")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(MINGW_FILE_NAME "llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64.tar.xz")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(MINGW_FILE_NAME "llvm-mingw-20241217-ucrt-ubuntu-20.04-aarch64.tar.xz")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

# Base URL for downloading
set(MINGW_BASE_URL "https://github.com/mstorsjo/llvm-mingw/releases/download/20241217/")

# Download and extract the toolchain
set(MINGW_TOOLCHAIN_DIR "${CMAKE_BINARY_DIR}/mingw-w64")
set(MINGW_TOOLCHAIN_URL "${MINGW_BASE_URL}${MINGW_FILE_NAME}")

FetchContent_Declare(
        mingw_toolchain
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL ${MINGW_TOOLCHAIN_URL}
        SOURCE_DIR ${MINGW_TOOLCHAIN_DIR}
)

message(STATUS "Downloading MinGW toolchain from ${MINGW_TOOLCHAIN_URL}")

FetchContent_MakeAvailable(mingw_toolchain)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(TOOLCHAIN_PREFIX aarch64-w64-mingw32)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()


set(CMAKE_C_COMPILER "${MINGW_TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${MINGW_TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-g++")
set(CMAKE_RC_COMPILER "${MINGW_TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-windres")

# Set other tools
set(CMAKE_AR "${MINGW_TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-ar")
set(CMAKE_RANLIB "${MINGW_TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-ranlib")
set(CMAKE_LINKER "${MINGW_TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-ld")

# Set the sysroot and compiler paths
set(CMAKE_SYSROOT "${MINGW_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
