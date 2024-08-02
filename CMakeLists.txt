cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(Tnet
        VERSION 2024.8
        DESCRIPTION "Reactor NetWork"
        LANGUAGES C CXX)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-reorder")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# Output directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Includes.
set(TNET_SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)

include_directories(${TNET_SRC_INCLUDE_DIR})

# Include the subdirectories
add_subdirectory(src)
