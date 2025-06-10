cmake_minimum_required(VERSION 3.12)
project(STLToGCode VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Threads REQUIRED)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/stl_loader.cpp
    src/geometry_analyzer.cpp
    src/gcode_generator.cpp
    src/validation_engine.cpp
    src/toolpath_generator.cpp
)

# Header files
set(HEADERS
    include/stl_loader.h
    include/geometry_analyzer.h
    include/gcode_generator.h
    include/validation_engine.h
    include/toolpath_generator.h
    include/types.h
)

# Create executable
add_executable(stl_to_gcode ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(stl_to_gcode Threads::Threads)

# Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(stl_to_gcode PRIVATE -Wall -Wextra -O2)
endif()

# Install target
install(TARGETS stl_to_gcode DESTINATION bin)

# Create include directory structure
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src)