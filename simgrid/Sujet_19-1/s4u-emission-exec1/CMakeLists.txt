cmake_minimum_required(VERSION 3.0)
project(s4u-emission-exec1)

set(CMAKE_MODULE_PATH "//home/yann/newSimGrid/simgrid")

# Find SimGrid package
find_package(SimGrid REQUIRED)
include_directories(${SimGrid_INCLUDE_DIR})

# Add the executable
add_executable(s4u-emission-exec1 src/s4u-emission-exec1.cpp)

# Link against SimGrid library and emission plugin
target_link_libraries(s4u-emission-exec1 ${SimGrid_LIBRARY})