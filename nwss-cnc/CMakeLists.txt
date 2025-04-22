cmake_minimum_required(VERSION 3.14)
project(nwss-cnc VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)  # Increased from 14 to 17 for std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to build tests
option(NWSS_CNC_BUILD_TESTS "Build test programs" OFF)

# Add math definitions for M_PI on Windows
if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Library sources
set(NWSS_CNC_SOURCES
    src/svg_parser.cpp
    src/discretizer.cpp
    src/geometry.cpp
    src/utils.cpp
    src/nanosvg_impl.cpp
    src/config.cpp
    src/transform.cpp
)

# Create static library
add_library(nwss-cnc STATIC ${NWSS_CNC_SOURCES})
target_include_directories(nwss-cnc PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Main application executable
add_executable(svg-converter apps/svg_converter.cpp)
target_link_libraries(svg-converter PRIVATE nwss-cnc)

# Config wizard application
add_executable(config-wizard apps/config_wizard.cpp)
target_link_libraries(config-wizard PRIVATE nwss-cnc)

# Copy example SVG files to build directory
file(COPY ${PROJECT_SOURCE_DIR}/example_files/ DESTINATION ${CMAKE_BINARY_DIR}/example_files/)

# Install targets
install(TARGETS nwss-cnc svg-converter config-wizard
    EXPORT nwss-cnc-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# Export targets
install(EXPORT nwss-cnc-targets
    FILE nwss-cnc-targets.cmake
    NAMESPACE NWSS::
    DESTINATION lib/cmake/nwss-cnc
)

# Create and install config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/nwss-cnc-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Instead of using an external config template, we'll generate it here
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/nwss-cnc-config.cmake"
"@PACKAGE_INIT@

include(\"\${CMAKE_CURRENT_LIST_DIR}/nwss-cnc-targets.cmake\")

check_required_components(nwss-cnc)
")

configure_package_config_file(
    "${CMAKE_CURRENT_BINARY_DIR}/nwss-cnc-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/nwss-cnc-config.cmake"
    INSTALL_DESTINATION lib/cmake/nwss-cnc
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/nwss-cnc-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/nwss-cnc-config-version.cmake"
    DESTINATION lib/cmake/nwss-cnc
)