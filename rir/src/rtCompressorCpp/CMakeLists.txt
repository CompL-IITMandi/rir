project (rtCompressor)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(source_directory "${PROJECT_SOURCE_DIR}/src")

file(GLOB project_files "${source_directory}/*.cpp")

add_executable(rtCompressor ${project_files})