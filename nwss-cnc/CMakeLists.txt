cmake_minimum_required(VERSION 3.16)
project(nwss-cnc VERSION 1.0.0 LANGUAGES CXX)

# Basic project settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Application metadata
set(APP_NAME "NWSS-CNC")
set(APP_BUNDLE_IDENTIFIER "org.nwss.cnc")
set(APP_VERSION ${PROJECT_VERSION})
set(APP_COMPANY "NWSS")
set(APP_COPYRIGHT "Copyright (C) 2025 NWSS. All rights reserved.")

# Platform-specific icons
set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.icns")
set(APP_ICON_LINUX "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.png")

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Find Qt packages
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets Svg SvgWidgets DBus REQUIRED)

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}
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
    src/core/tool.cpp
    src/core/tool_offset.cpp
)

# Create the core static library
add_library(nwss-cnc-core STATIC ${CORE_SOURCES})
target_include_directories(nwss-cnc-core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Define source files for the GUI application
set(GUI_SOURCES
    src/gui/mainwindow.cpp
    src/gui/gcodeeditor.cpp
    src/gui/gcodeviewer3d.cpp
    src/gui/svgdesigner.cpp
    src/gui/gcodeoptionspanel.cpp
    src/gui/svgtogcode.cpp
    src/gui/toolmanager.cpp
    src/main.cpp
)

# Define header files for the GUI application
set(GUI_HEADERS
    include/gui/mainwindow.h
    include/gui/gcodeeditor.h
    include/gui/gcodeviewer3d.h
    include/gui/svgdesigner.h
    include/gui/gcodeoptionspanel.h
    include/gui/svgtogcode.h
    include/gui/toolmanager.h
)

# Define resources
set(RESOURCES resources/resources.qrc)

# -------------------- Platform-specific setup --------------------

# macOS configuration
if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "nwss-cnc-icon.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    
    # Add fonts for macOS
    set(RESOURCE_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/JetBrainsMono-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/SF-Pro-Display-Regular.otf"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/SF-Pro-Display-Bold.otf"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/SF-Pro-Display-Medium.otf"
    )
    set_source_files_properties(${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/fonts")
    
    add_executable(nwss-cnc MACOSX_BUNDLE 
        ${GUI_SOURCES} 
        ${GUI_HEADERS}
        ${RESOURCES} 
        ${APP_ICON_MACOS}
        ${RESOURCE_FILES}
    )
    
    set_target_properties(nwss-cnc PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "${APP_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_COPYRIGHT "${APP_COPYRIGHT}"
    )

    find_program(MACDEPLOYQT_EXECUTABLE NAMES macdeployqt HINTS "${Qt6Core_DIR}/../../../bin")
    if(MACDEPLOYQT_EXECUTABLE)
        # Run windeployqt after build
        add_custom_command(TARGET nwss-cnc POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app"
            "-hardened-runtime"
            COMMENT "Deploying Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found! Skipping Qt dependency deployment.")
    endif()
    
    
    # macOS deployment
    install(TARGETS nwss-cnc BUNDLE DESTINATION .)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${APP_NAME}")
    
# Windows configuration
elseif(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/app.rc")
        set(PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/app.rc")
    endif()
    
    add_executable(nwss-cnc WIN32 
        ${GUI_SOURCES} 
        ${GUI_HEADERS}
        ${RESOURCES} 
        ${PLATFORM_SOURCES}
    )
    
    # Windows deployment
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
    
    install(TARGETS nwss-cnc RUNTIME DESTINATION ".")
    
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${Qt6Core_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        # Run windeployqt after build
        add_custom_command(TARGET nwss-cnc POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:nwss-cnc>"
            "--no-translations"
            COMMENT "Deploying Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found! Skipping Qt dependency deployment.")
    endif()
    
    # Create a post-build step to copy files to an 'install' directory for packaging
    add_custom_command(TARGET nwss-cnc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/install"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:nwss-cnc>" "${CMAKE_BINARY_DIR}/install/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:nwss-cnc>" "${CMAKE_BINARY_DIR}/install"
        COMMENT "Creating installation directory..."
    )
    
    # Package with CPack
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_FILE_NAME "NWSS-CNC-${PROJECT_VERSION}-win64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "NWSS-CNC")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    
    # NSIS Desktop Shortcut Configuration
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\${APP_NAME}.lnk' '$INSTDIR\\\\nwss-cnc.exe' '' '$INSTDIR\\\\nwss-cnc.exe' 0"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\${APP_NAME}.lnk'"
    )
    
    # Alternative method using MENU_LINKS (creates Start Menu + Desktop shortcuts)
    # set(CPACK_NSIS_MENU_LINKS
    #     "nwss-cnc.exe" "${APP_NAME}"
    # )
    # set(CPACK_NSIS_CREATE_ICONS ON)
    
    # Optional: Add to Start Menu as well
    set(CPACK_NSIS_MENU_LINKS
        "nwss-cnc.exe" "${APP_NAME}"
    )
    
    # Optional: Modify PATH (if needed)
    # set(CPACK_NSIS_MODIFY_PATH ON)
    
endif()

# Link libraries for the main executable
target_link_libraries(nwss-cnc PRIVATE
    nwss-cnc-core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::DBus
)

# Add additional include directories for the main executable
target_include_directories(nwss-cnc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
)

# Setup packaging
include(CPack)
set(CPACK_PACKAGE_NAME "${APP_NAME}")
set(CPACK_PACKAGE_VENDOR "${APP_COMPANY}")
set(CPACK_PACKAGE_CONTACT "${APP_COMPANY} <info@nwss.org>")  # General contact for all packages
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NWSS CNC - SVG to G-Code Converter")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")

# Special handling for package_for_github target on Windows
if(WIN32)
    add_custom_target(package_for_github
        COMMAND ${CMAKE_COMMAND} --build . --target package
        COMMENT "Creating packages for GitHub release"
        DEPENDS nwss-cnc
    )
else()
    # Standard version for macOS and Linux
    add_custom_target(package_for_github
        COMMAND ${CMAKE_COMMAND} --build . --target package
        COMMENT "Creating packages for GitHub release"
    )
endif()