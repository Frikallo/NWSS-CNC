@echo off
setlocal enabledelayedexpansion

REM NWSS-CNC Windows Production Build Script
echo 🚀 Starting Windows production build for NWSS-CNC...

REM Configuration
set "BUILD_TYPE=Release"
set "BUILD_DIR=build-windows"
set "APP_NAME=NWSS-CNC"
set "PROJECT_DIR=nwss-cnc"
set "DIST_DIR=NWSS-CNC-Windows"

REM Check if we're in the right directory
if not exist "%PROJECT_DIR%" (
    echo ❌ Error: %PROJECT_DIR% directory not found. Please run this script from the project root.
    pause
    exit /b 1
)

REM Clean previous build
echo 🧹 Cleaning previous build...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%BUILD_DIR%"

REM Enter project directory
cd "%PROJECT_DIR%"

REM Try to find Qt6 installation
echo 🔍 Locating Qt6 installation...
set "QT_DIR="
for /f "tokens=*" %%i in ('where /r "C:\Qt" qtbase-config.cmake 2^>nul ^| findstr /i qt6') do (
    set "QT_DIR=%%~dpi"
    goto :qt_found
)

:qt_found
if "%QT_DIR%"=="" (
    echo ❌ Error: Qt6 not found. Please ensure Qt6 is installed.
    echo    Expected location: C:\Qt\6.x.x\msvcXXXX_64\lib\cmake\Qt6
    pause
    exit /b 1
)

REM Remove trailing backslash and go up to find the bin directory
set "QT_DIR=%QT_DIR:~0,-1%"
for %%i in ("%QT_DIR%") do set "QT_BASE_DIR=%%~dpi"
set "QT_BASE_DIR=%QT_BASE_DIR:~0,-1%"
for %%i in ("%QT_BASE_DIR%") do set "QT_BASE_DIR=%%~dpi"
set "QT_BASE_DIR=%QT_BASE_DIR:~0,-1%"
for %%i in ("%QT_BASE_DIR%") do set "QT_BASE_DIR=%%~dpi"
set "QT_BASE_DIR=%QT_BASE_DIR:~0,-1%"
set "QT_BIN_DIR=%QT_BASE_DIR%\bin"

echo ✅ Found Qt6 at: %QT_BASE_DIR%

REM Configure CMake
echo ⚙️ Configuring CMake...
cmake -B "..\%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE="%BUILD_TYPE%" ^
    -DCMAKE_PREFIX_PATH="%QT_BASE_DIR%" ^
    -DCMAKE_INSTALL_PREFIX="..\%BUILD_DIR%\install" ^
    -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo ❌ CMake configuration failed
    pause
    exit /b 1
)

REM Build the application
echo 🔨 Building application...
cmake --build "..\%BUILD_DIR%" --config "%BUILD_TYPE%" --parallel

if %ERRORLEVEL% neq 0 (
    echo ❌ Build failed
    pause
    exit /b 1
)

REM Go back to root directory
cd ..

REM Check if executable was built
set "EXE_PATH=%BUILD_DIR%\%BUILD_TYPE%\nwss-cnc.exe"
if not exist "%EXE_PATH%" (
    echo ❌ Error: Executable not found at %EXE_PATH%
    pause
    exit /b 1
)

echo 📦 Executable built at: %EXE_PATH%

REM Create distribution directory
echo 📁 Creating distribution directory...
mkdir "%DIST_DIR%"

REM Copy the executable
copy "%EXE_PATH%" "%DIST_DIR%\"

REM Deploy Qt dependencies using windeployqt
echo 🔗 Deploying Qt dependencies...
set "WINDEPLOYQT_PATH=%QT_BIN_DIR%\windeployqt.exe"
if not exist "%WINDEPLOYQT_PATH%" (
    echo ❌ Error: windeployqt.exe not found at %WINDEPLOYQT_PATH%
    pause
    exit /b 1
)

"%WINDEPLOYQT_PATH%" ^
    --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    --no-svg ^
    --dir "%DIST_DIR%" ^
    "%DIST_DIR%\nwss-cnc.exe"

if %ERRORLEVEL% neq 0 (
    echo ⚠️ Warning: windeployqt reported errors, but continuing...
)

REM Copy additional resources if they exist
if exist "%PROJECT_DIR%\resources" (
    echo 📋 Copying additional resources...
    xcopy /E /I /Y "%PROJECT_DIR%\resources" "%DIST_DIR%\resources\"
)

REM Create a simple ZIP package
echo 📦 Creating ZIP package...
powershell -command "Compress-Archive -Path '%DIST_DIR%' -DestinationPath 'NWSS-CNC-Windows.zip' -Force"

if %ERRORLEVEL% neq 0 (
    echo ⚠️ Warning: Could not create ZIP using PowerShell
    echo 📁 Distribution folder created: %DIST_DIR%
) else (
    echo ✅ ZIP package created: NWSS-CNC-Windows.zip
)

REM Verify the distribution
echo 🔍 Verifying distribution...
echo    Distribution contents:
dir /b "%DIST_DIR%"

echo.
echo    Checking executable dependencies:
"%WINDEPLOYQT_PATH%" --list target "%DIST_DIR%\nwss-cnc.exe" 2>nul || echo "   Dependencies verification skipped"

echo.
echo ✅ Windows build completed successfully!
echo.
echo 📋 Build Summary:
echo    • Build type: %BUILD_TYPE%
echo    • Architecture: x64
echo    • Executable: %EXE_PATH%
echo    • Distribution: %DIST_DIR%\
if exist "NWSS-CNC-Windows.zip" (
    echo    • Package: NWSS-CNC-Windows.zip
)
echo.
echo 🎉 Ready for distribution!
echo.
pause 