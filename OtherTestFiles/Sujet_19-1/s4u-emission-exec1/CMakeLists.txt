cmake_minimum_required(VERSION 3.0)
project(s4u-emission-exec1)

# Set the path to the custom CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Find SimGrid package
find_package(SimGrid REQUIRED)

# Add the executable
add_executable(s4u-emission-exec1 src/s4u-emission-exec1.cpp)

# Link against SimGrid library and emission plugin
target_link_libraries(s4u-emission-exec1 ${SimGrid_LIBRARIES})
include_directories(${SimGrid_INCLUDE_DIRS})