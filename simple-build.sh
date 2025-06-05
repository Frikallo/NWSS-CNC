#!/bin/bash

# Simple build script for macOS
set -e

BUILD_DIR="build-simple"
APP_NAME="NWSS-CNC"

echo "🚀 Building $APP_NAME for macOS..."

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir "$BUILD_DIR"

# Enter project directory and build
cd nwss-cnc
cmake -B "../$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"

echo "🔨 Building..."
cd "../$BUILD_DIR"
make -j$(sysctl -n hw.ncpu)

echo "🎉 Build completed successfully!"
echo "📁 App bundle created at: $BUILD_DIR/$APP_NAME.app"

# Test the app
if [ -f "$APP_NAME.app/Contents/MacOS/$APP_NAME" ]; then
    echo "✅ Binary created successfully"
    echo "📊 Binary size: $(du -h "$APP_NAME.app/Contents/MacOS/$APP_NAME" | cut -f1)"
else
    echo "❌ Binary not found"
    exit 1
fi

# Deploy Qt dependencies
echo "🔗 Deploying Qt dependencies..."
QT_BIN_PATH="$(brew --prefix qt)/bin"
if [ -f "$QT_BIN_PATH/macdeployqt" ]; then
    "$QT_BIN_PATH/macdeployqt" "$APP_NAME.app" -verbose=2
    echo "✅ Qt deployment completed"
else
    echo "❌ macdeployqt not found"
    exit 1
fi

# Code sign the application for local development (ad-hoc signing)
echo "🔐 Code signing application..."
codesign --force --deep --sign - "$APP_NAME.app"
if [ $? -eq 0 ]; then
    echo "✅ Application signed successfully (ad-hoc)"
    echo "🔓 Code signature: $(codesign -dvv "$APP_NAME.app" 2>&1 | head -3)"
else
    echo "❌ Code signing failed"
    exit 1
fi

echo "🎯 Ready for distribution: $BUILD_DIR/$APP_NAME.app" 