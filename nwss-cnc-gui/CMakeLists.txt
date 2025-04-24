cmake_minimum_required(VERSION 3.16)
project(nwss-cnc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets Svg SvgWidgets SerialPort REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/gcodeeditor.cpp
    src/gcodeviewer3d.cpp
    src/svgviewer.cpp
    src/gcodeoptionspanel.cpp
    src/svgtogcode.cpp
)

# Header files
set(HEADERS
    src/mainwindow.h
    src/gcodeeditor.h
    src/gcodeviewer3d.h
    src/svgviewer.h
    src/gcodeoptionspanel.h
    src/svgtogcode.h
)

# UI files
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::SerialPort
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Install directives
install(TARGETS ${PROJECT_NAME} DESTINATION bin)