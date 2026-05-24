cmake_minimum_required(VERSION 3.12)
project(StaffelEinteilung)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/file_io.cpp
    src/league_splitter.cpp
    src/distance_calculator.cpp
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create executable
add_executable(StaffelEinteilung ${SOURCES})

# Platform-specific configurations
if(MSVC)
    # For MSVC compiler
    target_compile_options(StaffelEinteilung PRIVATE /W4)
else()
    # For GCC/Clang
    target_compile_options(StaffelEinteilung PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy config.json and adressen.txt to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/config.json
    ${CMAKE_BINARY_DIR}/config.json
    COPYONLY
)
configure_file(
    ${CMAKE_SOURCE_DIR}/adressen.txt
    ${CMAKE_BINARY_DIR}/adressen.txt
    COPYONLY
)
