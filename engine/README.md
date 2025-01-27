## CMake options
- `CMAKE_BUILD_TYPE` - Set to `Debug` for debug build, `Release` for release build.
- `ASE_BUILD_DEMO=ON` - Additionally add the demo executable to the build.

The following will build the default config of your CMake version. If you want to build Debug or Release please set the `CMAKE_BUILD_TYPE` variable by adding `-DCMAKE_BUILD_TYPE=XYZ` to the `cmake ..` command.

## Build for Local Processor 
```bash
mkdir -p cmake-build
cd cmake-build
cmake ..
cmake --build ./ -j 8 --target db_test
```

## Build for Windows From Linux using Mingw
This should automatically download the mingw toolchain and build the project for windows x64.
If you intend to build for Windows Arm64, you will need to modify the toolchain file and set CMAKE_SYSTEM_PROCESSOR to `aarch64`.
```bash
mkdir -p cmake-build-mingw
cd cmake-build-mingw
cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw-toolchain.cmake ..
cmake --build ./ -j 8 --target db_test
```

# Docker Demo
This will run a simple demo setup with:
- A server listening on port 42069
- A producer that writes the current time in milliseconds into producer.time every second
- A consumer that reads the time from producer.time and prints it to the console ever second
```bash
docker build -t ase_demo . -f docker/Dockerfile.demo
docker run --rm -p 42069:42069 ase_demo
```