include(BundleUtilities)

message(STATUS "Running macOS fixup_bundle on the .app bundle")

set(BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app")
fixup_bundle("${BUNDLE_PATH}" "" "")