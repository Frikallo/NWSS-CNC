# NWSS-CNC Production Build Instructions

This document explains how to build NWSS-CNC for production distribution on macOS and Windows.

## Prerequisites

### macOS
- **Xcode Command Line Tools**: `xcode-select --install`
- **Homebrew**: Install from [brew.sh](https://brew.sh/)
- **Qt6**: `brew install qt6`
- **CMake**: `brew install cmake`

### Windows
- **Visual Studio 2022** (Community Edition or higher) with C++ development tools
- **Qt6**: Download and install from [qt.io](https://www.qt.io/download-qt-installer)
  - Install to default location (`C:\Qt\`)
  - Select MSVC 2022 64-bit compiler
  - Include Qt SerialPort, Qt SVG, and Qt OpenGL modules
- **CMake**: Download from [cmake.org](https://cmake.org/download/) or install via Visual Studio Installer

## Building for macOS

### Quick Start
```bash
# From the project root directory
./build-macos.sh
```

### What the script does:
1. Cleans any previous build artifacts
2. Configures CMake with Release settings and Universal Binary support (x86_64 + ARM64)
3. Builds the application using all available CPU cores
4. Deploys Qt dependencies using `macdeployqt`
5. Creates a `.dmg` package using CPack
6. Verifies the final app bundle

### Output:
- **App Bundle**: `build-macos/NWSS-CNC.app`
- **DMG Package**: `NWSS-CNC-*.dmg` (in project root)
- **Fallback ZIP**: `NWSS-CNC-macOS.zip` (if DMG creation fails)

### Troubleshooting macOS:
- **Qt6 not found**: Ensure Qt6 is installed via Homebrew: `brew install qt6`
- **macdeployqt fails**: Check that `$(brew --prefix qt6)/bin/macdeployqt` exists
- **Build fails**: Verify Xcode Command Line Tools are installed

## Building for Windows

### Quick Start
```batch
REM From the project root directory (Command Prompt)
build-windows.bat
```

### What the script does:
1. Auto-detects Qt6 installation in `C:\Qt\`
2. Cleans any previous build artifacts
3. Configures CMake with Visual Studio 2022 generator
4. Builds the application in Release mode
5. Deploys Qt dependencies using `windeployqt`
6. Copies additional resources
7. Creates a ZIP package for distribution

### Output:
- **Executable**: `build-windows/Release/nwss-cnc.exe`
- **Distribution Folder**: `NWSS-CNC-Windows/` (with all dependencies)
- **ZIP Package**: `NWSS-CNC-Windows.zip`

### Troubleshooting Windows:
- **Qt6 not found**: Ensure Qt6 is installed in `C:\Qt\6.x.x\msvcXXXX_64\`
- **Visual Studio not found**: Install Visual Studio 2022 with C++ development tools
- **Build fails**: Open "Developer Command Prompt for VS 2022" and run the script from there
- **windeployqt errors**: Usually safe to ignore warnings, but check that Qt DLLs are copied

## Manual Building (Alternative)

If the automated scripts don't work for your environment:

### macOS Manual Build:
```bash
cd nwss-cnc
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt6)"
make -j$(sysctl -n hw.ncpu)
/usr/local/opt/qt6/bin/macdeployqt NWSS-CNC.app
```

### Windows Manual Build:
```batch
cd nwss-cnc
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvcXXXX_64"
cmake --build . --config Release
C:\Qt\6.x.x\msvcXXXX_64\bin\windeployqt.exe Release\nwss-cnc.exe
```

## Customization

You can modify the build scripts to customize:

### Build Configuration:
- Change `BUILD_TYPE` from "Release" to "Debug" for debugging builds
- Modify architecture settings (macOS: `CMAKE_OSX_ARCHITECTURES`)
- Adjust Qt module requirements in CMakeLists.txt

### Packaging Options:
- **macOS**: Modify CPack DMG settings in CMakeLists.txt
- **Windows**: Change ZIP compression or add installer creation

### Resource Deployment:
- Additional resources are automatically copied from `nwss-cnc/resources/`
- Fonts and icons are handled by the CMakeLists.txt configuration

## Verification

After building, verify your distribution:

### macOS:
```bash
# Test the app bundle
open build-macos/NWSS-CNC.app

# Check dependencies
otool -L build-macos/NWSS-CNC.app/Contents/MacOS/NWSS-CNC
```

### Windows:
```batch
# Test the executable
NWSS-CNC-Windows\nwss-cnc.exe

# Check dependencies are included
dir NWSS-CNC-Windows\
```

## Distribution

### macOS:
- Distribute the `.dmg` file for easy installation
- Users drag the app to Applications folder
- The app is code-signed if you have a Developer Certificate

### Windows:
- Distribute the `NWSS-CNC-Windows.zip` file
- Users extract and run `nwss-cnc.exe`
- All Qt dependencies are included in the folder

## CI/CD Integration

These scripts are designed to work in automated environments:

```yaml
# GitHub Actions example
- name: Build macOS
  run: ./build-macos.sh
  
- name: Build Windows
  run: build-windows.bat
```

The scripts provide exit codes for success/failure detection in automated builds. 