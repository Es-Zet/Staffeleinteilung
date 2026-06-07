cmake_minimum_required(VERSION 3.12)
project(StaffelEinteilung)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common source files (shared between executables)
set(COMMON_SOURCES
    src/file_io.cpp
    src/league_splitter.cpp
    src/distance_calculator.cpp
    src/optimizer.cpp
    src/evaluation_results.cpp
    src/html_visualizer.cpp
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Main executable: League Assignment Optimizer
set(MAIN_SOURCES ${COMMON_SOURCES} src/main.cpp)
add_executable(StaffelEinteilung ${MAIN_SOURCES})

# Utility executable: Metric Calculator
set(EVAL_SOURCES ${COMMON_SOURCES} src/evaluation.cpp)
add_executable(StaffelBewertung ${EVAL_SOURCES})

# Platform-specific configurations for main executable
if(MSVC)
    target_compile_options(StaffelEinteilung PRIVATE /W4)
    target_compile_options(StaffelBewertung PRIVATE /W4)
else()
    target_compile_options(StaffelEinteilung PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(StaffelBewertung PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy config.json and Teamliste.csv to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/config.json
    ${CMAKE_BINARY_DIR}/config.json
    COPYONLY
)
configure_file(
    ${CMAKE_SOURCE_DIR}/Teamliste.csv
    ${CMAKE_BINARY_DIR}/Teamliste.csv
    COPYONLY
)
