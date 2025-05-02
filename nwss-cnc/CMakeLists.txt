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

# Application metadata
set(APP_NAME "NWSS CNC")
set(APP_BUNDLE_IDENTIFIER "org.nwss.cnc")
set(APP_VERSION ${PROJECT_VERSION})
set(APP_COMPANY "NWSS")
set(APP_COPYRIGHT "Copyright (C) 2025 NWSS. All rights reserved.")

# Platform-specific icons
set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.icns")

# Add math definitions for M_PI on Windows
if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Performance optimization flags for Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring release build with optimizations")
    
    if(MSVC)
        # MSVC compiler flags for optimized release
        add_compile_options(/O2 /Ob2 /GL /Gy /Gw)
        add_link_options(/LTCG)
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    else()
        # GCC/Clang compiler flags for optimized release
        add_compile_options(-O3 -march=native -flto)
        
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_link_options(-flto)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_link_options(-flto)
        endif()
    endif()
endif()

# Find required packages
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets Svg SvgWidgets SerialPort REQUIRED)

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}  # Include root directory
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

# -------------------- Platform-specific setup --------------------

# Windows specific configuration
if(WIN32)
    # Windows icon and resources
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/app.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/app.rc
        @ONLY
    )
    set(PLATFORM_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
endif()

# macOS specific configuration
if(APPLE)
    # Set macOS bundle icon file
    set(MACOSX_BUNDLE_ICON_FILE "nwss-cnc-icon.icns")
    set(APPLICATION_ICON "${APP_ICON_MACOS}")
    set_source_files_properties(${APPLICATION_ICON}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )
    
    # Add other resources to the bundle
    file(GLOB_RECURSE EXAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/example_files/*")
    foreach(FILE ${EXAMPLE_FILES})
        file(RELATIVE_PATH REL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/example_files" ${FILE})
        get_filename_component(REL_DIR ${REL_FILE} DIRECTORY)
        set_source_files_properties(${FILE}
            PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/example_files/${REL_DIR}"
        )
    endforeach()
    
    # Add fonts or other resources
    set(RESOURCE_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/JetBrainsMono-Regular.ttf"
    )
    foreach(FILE ${RESOURCE_FILES})
        get_filename_component(FILENAME ${FILE} NAME)
        set_source_files_properties(${FILE}
            PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/fonts"
        )
    endforeach()
endif()

# -------------------- Create Executable --------------------

# Create the main executable with platform-specific setup
if(APPLE)
    add_executable(nwss-cnc MACOSX_BUNDLE
        src/main.cpp
        ${GUI_SOURCES}
        ${GUI_HEADERS}
        ${RESOURCES}
        ${APPLICATION_ICON}
        ${EXAMPLE_FILES}
        ${RESOURCE_FILES}
    )
else()
    add_executable(nwss-cnc WIN32
        src/main.cpp
        ${GUI_SOURCES}
        ${GUI_HEADERS}
        ${RESOURCES}
        ${PLATFORM_SOURCES}
    )
endif()

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

# -------------------- Set Properties --------------------

# Set application properties
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set_target_properties(nwss-cnc PROPERTIES
        OUTPUT_NAME "${APP_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_COPYRIGHT "${APP_COPYRIGHT}"
        RESOURCE "${RESOURCE_FILES}"
    )
else()
    set_target_properties(nwss-cnc PROPERTIES
        OUTPUT_NAME "${APP_NAME}"
    )
endif()

# -------------------- Packaging Setup --------------------

# Setup for CPack to create redistributable packages
include(CPack)
set(CPACK_PACKAGE_NAME "${APP_NAME}")
set(CPACK_PACKAGE_VENDOR "${APP_COMPANY}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NWSS CNC - SVG to G-Code Converter")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${APP_NAME}")

# Platform-specific packaging configuration
if(WIN32)
    # Windows installer settings
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "${APP_NAME}")
    set(CPACK_NSIS_DISPLAY_NAME "${APP_NAME}")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_MUI_ICON "${APP_ICON_WINDOWS}")
    set(CPACK_NSIS_MUI_UNIICON "${APP_ICON_WINDOWS}")
    
    # Add Windows example files to the package
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/example_files/"
        DESTINATION "example_files"
        COMPONENT Runtime
    )
    
    # Add Windows DLL dependencies
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:nwss-cnc>
            RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
            UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
        )
        foreach(DLL ${RESOLVED_DEPS})
            file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}" TYPE SHARED_LIBRARY FILES "${DLL}")
        endforeach()
    ]] COMPONENT Runtime)
elseif(APPLE)
    # macOS DMG settings
    set(CPACK_DMG_VOLUME_NAME "${APP_NAME}")
    set(CPACK_DMG_FORMAT "UDBZ") # bzip2 compressed disk image
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/DMGSetup.scpt")
    set(CPACK_APPLE_BUNDLE_ID "${APP_BUNDLE_IDENTIFIER}")
    
    set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/macos_fixup.cmake")
endif()

# Install the main application
install(TARGETS nwss-cnc
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION . COMPONENT Runtime
)

# Create a package target for GitHub Actions to use
add_custom_target(package_for_github
    COMMAND ${CMAKE_COMMAND} --build . --target package
    COMMENT "Creating packages for GitHub release"
)