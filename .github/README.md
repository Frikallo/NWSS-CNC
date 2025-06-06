# GitHub Actions Build Workflows

This directory contains GitHub Actions workflows for building and packaging the NWSS-CNC application for Windows and macOS.

## Workflows

### 1. `build-all-platforms.yml` (Recommended)
- **Purpose**: Builds both Windows and macOS versions in parallel
- **Triggers**: Push to main/develop, pull requests, releases, and tags starting with 'v'
- **Outputs**: 
  - Windows: ZIP file with executable and dependencies
  - macOS: DMG file with app bundle

### 2. `build-windows.yml`
- **Purpose**: Builds Windows version only
- **Output**: ZIP file and/or executable with Qt dependencies

### 3. `build-macos.yml`
- **Purpose**: Builds macOS version only  
- **Output**: DMG file with signed app bundle (if certificates are configured)

## Setup Instructions

### Basic Setup (No Code Signing)
1. Push these workflow files to your repository
2. The workflows will automatically run on pushes to main/develop branches
3. Download artifacts from the Actions tab

### Advanced Setup (With Code Signing)

#### For macOS Code Signing (Optional)
Add these secrets to your repository (Settings → Secrets and variables → Actions):

- `APPLE_CERTIFICATE_P12`: Base64-encoded .p12 certificate file
- `APPLE_CERTIFICATE_PASSWORD`: Password for the .p12 file  
- `APPLE_SIGNING_IDENTITY`: Code signing identity (e.g., "Developer ID Application: Your Name")

To create the base64 certificate:
```bash
base64 -i your_certificate.p12 | pbcopy
```

#### For Windows Code Signing (Optional)
You can extend the Windows workflow to include code signing by adding:
- Certificate file as a secret
- Signing tools like `signtool.exe`

## How It Works

### Dependencies
- **Qt 6.5.0**: Installed via `jurplel/install-qt-action`
- **CMake**: Uses your existing CMakeLists.txt configuration
- **Platform Tools**: 
  - Windows: Visual Studio 2022, windeployqt
  - macOS: Xcode tools, macdeployqt, create-dmg

### Build Process
1. **Checkout**: Downloads your source code
2. **Install Qt**: Sets up Qt 6.5.0 with required modules (qtserialport, qtsvg)
3. **Configure**: Runs CMake configuration
4. **Build**: Compiles the application
5. **Deploy**: Bundles Qt dependencies with the application
6. **Package**: Creates distributable packages (ZIP/DMG)
7. **Upload**: Stores artifacts and uploads to releases

### Artifacts
- **Development builds**: Available in GitHub Actions artifacts for 30 days
- **Release builds**: Automatically attached to GitHub releases

## Customization

### Qt Version
To change Qt version, modify the `version` field in the workflows:
```yaml
- name: Install Qt
  uses: jurplel/install-qt-action@v3
  with:
    version: '6.6.0'  # Change this
```

### Build Configuration
The workflows use your existing CMakeLists.txt configuration. To modify build settings, you can:
1. Update CMakeLists.txt 
2. Add CMake flags in the workflow files
3. Modify the deployment steps

### Triggers
To change when builds run, modify the `on:` section:
```yaml
on:
  push:
    branches: [ main ]  # Only build main branch
  release:
    types: [ published ]
```

## Troubleshooting

### Common Issues

1. **Qt modules not found**: Ensure all required Qt modules are listed in the workflow
2. **Build failures**: Check that your CMakeLists.txt works locally first
3. **Missing dependencies**: Make sure all third-party dependencies are properly configured
4. **Packaging issues**: Review the deployment steps for your specific platform

### Debugging
- Check the workflow logs in the Actions tab
- Use the "List build artifacts" step output to see what files were created
- Test builds locally using similar commands to the workflow

### Local Testing
Before pushing, you can test the build locally:

```bash
# Install Qt 6.5.0 locally
# Configure and build
cmake -B build -S nwss-cnc -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
``` 