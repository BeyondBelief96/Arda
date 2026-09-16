cmake_minimum_required(VERSION 3.23)
project(Arda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All executables land in <binaryDir>/bin. Multi-config generators (VS, Ninja
# Multi-Config) append /Debug or /Release, so run.ps1 always knows the path.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_subdirectory(core)
add_subdirectory(renderer)
add_subdirectory(scene)
