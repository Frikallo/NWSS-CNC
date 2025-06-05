#!/bin/bash

# NWSS-CNC macOS Production Build Script
set -e

# Configuration
BUILD_TYPE="Release"
BUILD_DIR="build-macos"
APP_NAME="NWSS-CNC"
PROJECT_DIR="nwss-cnc"

echo "🚀 Starting macOS production build for ${APP_NAME}..."

# Check if we're in the right directory
if [ ! -d "$PROJECT_DIR" ]; then
    echo "❌ Error: $PROJECT_DIR directory not found. Please run this script from the project root."
    exit 1
fi

# Clean previous build
echo "🧹 Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Enter project directory
cd "$PROJECT_DIR"

# Configure CMake
echo "⚙️ Configuring CMake..."
# Try to find Qt installation
QT_PREFIX=""
if [ -d "$(brew --prefix qt)" ]; then
    QT_PREFIX="$(brew --prefix qt)"
    echo "📦 Using Qt from: $QT_PREFIX"
elif [ -d "$(brew --prefix qt6)" ]; then
    QT_PREFIX="$(brew --prefix qt6)"
    echo "📦 Using Qt6 from: $QT_PREFIX"
else
    echo "❌ Error: Qt not found via Homebrew. Please install Qt6: brew install qt"
    exit 1
fi

cmake -B "../$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" \
    -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
    -DCMAKE_INSTALL_PREFIX="../$BUILD_DIR/install"

# Build the application
echo "🔨 Building application..."
cmake --build "../$BUILD_DIR" --config "$BUILD_TYPE" -j$(sysctl -n hw.ncpu)

# Go back to root
cd ..

# Find the built app bundle
APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"

if [ ! -d "$APP_BUNDLE" ]; then
    echo "❌ Error: App bundle not found at $APP_BUNDLE"
    exit 1
fi

echo "📦 App bundle created at: $APP_BUNDLE"

# Deploy Qt dependencies using macdeployqt
echo "🔗 Deploying Qt dependencies..."
QT_BIN_PATH="$QT_PREFIX/bin"
if [ -f "$QT_BIN_PATH/macdeployqt" ]; then
    "$QT_BIN_PATH/macdeployqt" "$APP_BUNDLE" -verbose=2
else
    echo "❌ Error: macdeployqt not found at $QT_BIN_PATH/macdeployqt"
    exit 1
fi

# Code sign the application for distribution
echo "🔐 Code signing application..."
codesign --force --deep --sign - "$APP_BUNDLE"
if [ $? -eq 0 ]; then
    echo "✅ Application signed successfully (ad-hoc)"
else
    echo "❌ Code signing failed"
    exit 1
fi

# Create DMG using CPack
echo "💿 Creating DMG package..."
cd "$BUILD_DIR"
cpack -G DragNDrop

# Find the created DMG
DMG_FILE=$(find . -name "*.dmg" | head -1)
if [ -n "$DMG_FILE" ]; then
    DMG_NAME=$(basename "$DMG_FILE")
    echo "✅ DMG created successfully: $DMG_NAME"
    
    # Move DMG to project root
    mv "$DMG_FILE" "../$DMG_NAME"
    echo "📁 Final package location: ./$DMG_NAME"
else
    echo "⚠️ DMG not created by CPack, creating manual distribution folder..."
    # Create a distribution folder as fallback
    DIST_DIR="../${APP_NAME}-macOS"
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR"
    cp -R "$APP_BUNDLE" "$DIST_DIR/"
    
    # Create a ZIP as alternative
    cd ..
    zip -r "${APP_NAME}-macOS.zip" "${APP_NAME}-macOS"
    echo "📁 Alternative package created: ${APP_NAME}-macOS.zip"
fi

cd ..

# Verify the app bundle
echo "🔍 Verifying app bundle..."
if [ -d "$APP_BUNDLE" ]; then
    echo "   Bundle structure:"
    ls -la "$APP_BUNDLE/Contents/"
    
    # Check if the executable is properly linked
    echo "   Checking executable dependencies:"
    otool -L "$APP_BUNDLE/Contents/MacOS/$APP_NAME" | head -10
fi

echo "✅ macOS build completed successfully!"
echo ""
echo "📋 Build Summary:"
echo "   • Build type: $BUILD_TYPE"
echo "   • Architecture: Universal (x86_64 + arm64)"
echo "   • App bundle: $APP_BUNDLE"
if [ -n "$DMG_FILE" ]; then
    echo "   • Package: $DMG_NAME"
else
    echo "   • Package: ${APP_NAME}-macOS.zip"
fi
echo ""
echo "🎉 Ready for distribution!" 