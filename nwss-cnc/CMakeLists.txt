cmake_minimum_required(VERSION 3.16)
project(nwss-cnc VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt specific configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add math definitions for M_PI on Windows
if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

# Find required packages
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets Svg SvgWidgets SerialPort REQUIRED)

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Define source files for the core library
set(CORE_SOURCES
    src/core/svg_parser.cpp
    src/core/discretizer.cpp
    src/core/geometry.cpp
    src/core/utils.cpp
    src/core/nanosvg_impl.cpp
    src/core/config.cpp
    src/core/transform.cpp
    src/core/gcode_generator.cpp
)

# Create the core static library
add_library(nwss-cnc-core STATIC ${CORE_SOURCES})
target_include_directories(nwss-cnc-core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Define source files for the GUI application
set(GUI_SOURCES
    src/gui/mainwindow.cpp
    src/gui/gcodeeditor.cpp
    src/gui/gcodeviewer3d.cpp
    src/gui/svgviewer.cpp
    src/gui/gcodeoptionspanel.cpp
    src/gui/svgtogcode.cpp
)

# Define header files for the GUI application
set(GUI_HEADERS
    include/gui/mainwindow.h
    include/gui/gcodeeditor.h
    include/gui/gcodeviewer3d.h
    include/gui/svgviewer.h
    include/gui/gcodeoptionspanel.h
    include/gui/svgtogcode.h
)

# Define resources
set(RESOURCES
    resources/resources.qrc
)

# Create the main executable
add_executable(nwss-cnc src/main.cpp ${GUI_SOURCES} ${GUI_HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(nwss-cnc PRIVATE
    nwss-cnc-core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::SerialPort
)

# Include directories for GUI
target_include_directories(nwss-cnc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/gui)

# Copy example SVG files to build directory
file(COPY ${PROJECT_SOURCE_DIR}/example_files/ DESTINATION ${CMAKE_BINARY_DIR}/example_files/)

# Install targets
install(TARGETS nwss-cnc nwss-cnc-core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install header files (optional)
install(DIRECTORY include/ DESTINATION include)