# Minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(Kache CXX)

# Use the C++17 standard for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mac specific setting
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Find the pthreads library for std::thread
find_package(Threads REQUIRED)

# Define the executable and its source files
add_executable(kache_server src/main.cpp src/KacheStore.cpp src/KacheServer.cpp)

# Tell the compiler where to find our header files
target_include_directories(kache_server PUBLIC include)

# Link necessary libraries (like pthreads for threading)
target_link_libraries(kache_server PRIVATE Threads::Threads)

# A nice-to-have for easier debugging in VS Code or GDB
set(CMAKE_BUILD_TYPE Debug)