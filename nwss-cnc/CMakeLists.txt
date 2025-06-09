cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0175)
    cmake_policy(SET CMP0175 NEW)
endif()

project(nwss-cnc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(APP_NAME "NWSS-CNC")
set(APP_BUNDLE_IDENTIFIER "org.nwss.cnc")
set(APP_VERSION ${PROJECT_VERSION})
set(APP_COMPANY "NWSS")
set(APP_COPYRIGHT "Copyright (C) 2025 NWSS. All rights reserved.")

set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.icns")
set(APP_ICON_LINUX "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.png")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets Svg SvgWidgets DBus REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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

add_library(nwss-cnc-core STATIC ${CORE_SOURCES})
target_include_directories(nwss-cnc-core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

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

set(GUI_HEADERS
    include/gui/mainwindow.h
    include/gui/gcodeeditor.h
    include/gui/gcodeviewer3d.h
    include/gui/svgdesigner.h
    include/gui/gcodeoptionspanel.h
    include/gui/svgtogcode.h
    include/gui/toolmanager.h
)

set(RESOURCES resources/resources.qrc)

# -------------------- Platform-specific setup --------------------

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "nwss-cnc-icon.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    
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
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path/../Frameworks"
    )

    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${QT_WINDEPLOYQT_EXECUTABLE})
    
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET nwss-cnc POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app"
            "-always-overwrite"
            "-executable=${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/MacOS/${APP_NAME}"
            COMMENT "Deploying Qt dependencies and fixing paths..."
        )
        
        find_program(CODESIGN_EXECUTABLE codesign)
        if(CODESIGN_EXECUTABLE)
            add_custom_command(TARGET nwss-cnc POST_BUILD
                COMMAND "${CODESIGN_EXECUTABLE}" --deep --force --verify --verbose --sign "-" "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app"
                COMMENT "Code signing application..."
            )
        else()
            message(WARNING "codesign not found! The app will not be code signed.")
        endif()
    else()
        message(WARNING "macdeployqt not found! Skipping Qt dependency deployment.")
        message(STATUS "Searched in: ${QT_WINDEPLOYQT_EXECUTABLE}")
    endif()
    
    install(TARGETS nwss-cnc BUNDLE DESTINATION .)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${APP_NAME}")
    
    set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    
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
    
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
    # set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    
    install(TARGETS nwss-cnc RUNTIME DESTINATION ".")
    
    if(EXISTS "${CMAKE_BINARY_DIR}/redist/vc_redist.x64.exe")
        install(FILES "${CMAKE_BINARY_DIR}/redist/vc_redist.x64.exe" DESTINATION "redist/")
    endif()
    
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${Qt6Core_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET nwss-cnc POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:nwss-cnc>"
            "--release"
            "--no-translations"
            COMMENT "Deploying Qt dependencies (Release mode)..."
        )
    else()
        message(WARNING "windeployqt not found! Skipping Qt dependency deployment.")
    endif()
    
    add_custom_command(TARGET nwss-cnc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/install"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:nwss-cnc>" "${CMAKE_BINARY_DIR}/install/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:nwss-cnc>" "${CMAKE_BINARY_DIR}/install"
        COMMENT "Creating installation directory..."
    )
    
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "NWSS-CNC")
    set(CPACK_NSIS_DISPLAY_NAME "NWSS-CNC")
    set(CPACK_PACKAGE_EXECUTABLES "nwss-cnc.exe" "NWSS-CNC")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "NWSS-CNC-${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "NWSS-CNC-${PROJECT_VERSION}-win64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "NWSS-CNC")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/nwss-cnc-icon.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH "ON")
    set(CPACK_CREATE_DESKTOP_LINKS "nwss-cnc.exe")

    set(CPACK_NSIS_MENU_LINKS
        "nwss-cnc.exe" "${APP_NAME}"
    )
endif()

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

target_include_directories(nwss-cnc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
)

include(CPack)
set(CPACK_PACKAGE_NAME "${APP_NAME}")
set(CPACK_PACKAGE_VENDOR "${APP_COMPANY}")
set(CPACK_PACKAGE_CONTACT "${APP_COMPANY} <info@nwss.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NWSS CNC - SVG to G-Code Converter")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")

if(WIN32)
    add_custom_target(package_for_github
        COMMAND ${CMAKE_COMMAND} --build . --config Release --target package
        COMMENT "Creating packages for GitHub release (Release configuration only)"
        DEPENDS nwss-cnc
    )
else()
    add_custom_target(package_for_github
        COMMAND ${CMAKE_COMMAND} --build . --target package
        COMMENT "Creating packages for GitHub release"
    )
endif()