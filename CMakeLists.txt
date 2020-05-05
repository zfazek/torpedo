project(torpedo)

cmake_minimum_required(VERSION 2.8)

find_package(ZLIB REQUIRED)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -g -O2")

include_directories(${CMAKE_SOURCE_DIR}/../seasocks/src/main/c)

add_subdirectory(src)
