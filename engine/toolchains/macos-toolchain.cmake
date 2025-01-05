include(FetchContent)

set(CMAKE_SYSTEM_NAME Darwin) # Cross-compiling to Macos
set(CMAKE_SYSTEM_VERSION 15.2) # The version of the SDK that will be downloaded & used
set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0) # The minimum versions to build for
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Keep in mind that the SDK version you selected will propably not work if you installed clang is older than the one used by the SDK version. You can check this here: https://gist.github.com/yamaya/2924292

# Set the compiler to clang
find_program(CLANG_C_COMPILER NAMES clang HINTS /usr/bin /usr/local/bin /opt/homebrew/bin)
find_program(CLANG_CXX_COMPILER NAMES clang++ HINTS /usr/bin /usr/local/bin /opt/homebrew/bin)

# Check if Clang was found
if (NOT CLANG_C_COMPILER OR NOT CLANG_CXX_COMPILER)
    message(FATAL_ERROR "Clang or Clang++ not found on this system!")
endif()

# Set the compilers
set(CMAKE_C_COMPILER "${CLANG_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${CLANG_CXX_COMPILER}")

# Set the linker to `ld64.lld`
find_program(LLD_LINKER NAMES ld64.lld HINTS /usr/bin /usr/local/bin /opt/llvm/bin)
if (NOT LLD_LINKER)
    message(FATAL_ERROR "ld64.lld not found! Please install LLVM's ld64.lld linker.")
endif()
set(CMAKE_LINKER "${LLD_LINKER}")

# Set ar and ranlib
find_program(LLVM_AR   NAMES llvm-ar   HINTS /usr/bin /usr/local/bin /opt/llvm/bin)
find_program(LLVM_RANLIB NAMES llvm-ranlib HINTS /usr/bin /usr/local/bin /opt/llvm/bin)

if(NOT LLVM_AR OR NOT LLVM_RANLIB)
    message(FATAL_ERROR "Cannot find llvm-ar or llvm-ranlib. Install LLVM toolchain.")
endif()

set(CMAKE_AR      "${LLVM_AR}")
set(CMAKE_RANLIB  "${LLVM_RANLIB}")

# Base URL for downloading
set(MACOS_SDK_URL "https://github.com/joseluisq/macosx-sdks/releases/download/${CMAKE_SYSTEM_VERSION}/MacOSX${CMAKE_SYSTEM_VERSION}.sdk.tar.xz")

# Download and extract the toolchain
set(MACOS_SDK_DIR "${CMAKE_BINARY_DIR}/macos_sdk")

FetchContent_Declare(
        macos_sdk
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL ${MACOS_SDK_URL}
        SOURCE_DIR ${MACOS_SDK_DIR}
)

message(STATUS "Downloading MacOS SDK from ${MACOS_SDK_URL}")

FetchContent_MakeAvailable(macos_sdk)

set(CMAKE_OSX_SYSROOT "${MACOS_SDK_DIR}") # Path to the SDK
set(SDK_INCLUDES "${CMAKE_OSX_SYSROOT}/usr/include/c++/v1")

# Automatically set target flags based on CMAKE_OSX_ARCHITECTURES
set(ARCH_FLAGS "")
foreach(ARCH ${CMAKE_OSX_ARCHITECTURES})
    set(ARCH_FLAGS "${ARCH_FLAGS} -target ${ARCH}-apple-darwin")
endforeach()

set(PLATFORM_VERSION_FLAGS "-Wl,-platform_version,macos,${CMAKE_OSX_DEPLOYMENT_TARGET},${CMAKE_SYSTEM_VERSION}")

# Set compiler and linker flags
set(CMAKE_C_FLAGS_INIT "${ARCH_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT} -nostdinc++")
set(CMAKE_CXX_FLAGS_INIT "${ARCH_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT} -nostdinc++ -stdlib=libc++ -isystem ${SDK_INCLUDES}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=${LLD_LINKER} -isysroot ${CMAKE_OSX_SYSROOT} ${PLATFORM_VERSION_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=${LLD_LINKER} -isysroot ${CMAKE_OSX_SYSROOT} ${PLATFORM_VERSION_FLAGS}")
