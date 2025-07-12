cmake_minimum_required(VERSION 3.16)
project(TSGL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SOURCES "*.c")
add_library(TSGL ${SOURCES})
target_include_directories(TSGL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
