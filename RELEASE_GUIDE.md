# GitHub Actions Release Guide

This guide explains how to use the automated CI/CD pipeline to build and release NWSS-CNC binaries.

## 🚀 Quick Release Process

### Automatic Release (Recommended)
1. **Tag a Release:**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. **GitHub Actions will automatically:**
   - Build macOS and Windows binaries
   - Package them with all Qt dependencies
   - Create a GitHub Release with download links
   - Generate release notes

### Manual Release
1. Go to GitHub → Actions → "Build and Release"
2. Click "Run workflow"
3. Check "Create a new release"
4. Click "Run workflow"

## 📋 What Gets Built

### macOS Package
- **Universal Binary** (Intel + Apple Silicon)
- **DMG Installer** - Drag-and-drop installation
- **ZIP Archive** - Portable version
- **All Qt dependencies** included via `macdeployqt`
- **Compatible with** macOS 10.15+

### Windows Package  
- **64-bit Executable** (x64)
- **ZIP Archive** with all dependencies
- **All Qt DLLs** included via `windeployqt`
- **Compatible with** Windows 10+
- **No installation required** - extract and run

## 🔄 CI/CD Workflows

### 1. Continuous Integration (`ci.yml`)
**Triggers:** Push to main/develop, Pull Requests

**What it does:**
- Builds on macOS and Windows
- Verifies app bundle/executable creation
- Runs code quality checks
- Validates project structure

### 2. Build and Release (`build-and-release.yml`)
**Triggers:** Version tags (`v*`), Manual dispatch

**What it does:**
- Production builds for both platforms
- Qt dependency deployment
- Automatic GitHub Release creation
- Release notes generation

## 📦 Release Assets

Each release includes these files:

```
NWSS-CNC-1.0.0.dmg          # macOS installer
NWSS-CNC-macOS.zip           # macOS portable
NWSS-CNC-Windows.zip         # Windows portable
```

## 🏷️ Version Tagging

### Semantic Versioning
- `v1.0.0` - Major release
- `v1.1.0` - Minor release  
- `v1.0.1` - Patch release

### Tag Creation
```bash
# Local tagging
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# Or create via GitHub UI
# Releases → Create a new release → Choose a tag
```

## 🛠️ Customizing Builds

### Build Configuration
Edit the workflow files in `.github/workflows/`:

**Qt Version:**
```yaml
- name: Install Qt6
  with:
    version: '6.6.0'  # Change this
```

**Additional Modules:**
```yaml
modules: 'qtserialport qtsvg qtopengl qtnetwork'
```

**Build Type:**
In build scripts, change:
```bash
BUILD_TYPE="Release"  # or "Debug"
```

### Release Notes Customization
Edit the release notes template in `build-and-release.yml`:
```yaml
- name: Generate release notes
  run: |
    cat << 'EOF' > release_notes.md
    # Your custom release notes here
    EOF
```

## 🐛 Troubleshooting

### Common Issues

**Qt Installation Fails:**
- GitHub runners sometimes have Qt installation issues
- The workflow will retry with caching enabled

**Build Script Permission Denied (macOS):**
- Fixed by `chmod +x build-macos.sh` step in workflow

**Windows Build Fails:**
- Usually related to Visual Studio or CMake setup
- Check the "Setup MSVC" and "Setup CMake" steps

**Missing Artifacts:**
- Build likely failed, check the build logs
- Debug artifacts are uploaded on failure

### Debugging Failed Builds

1. **Check Action Logs:**
   - Go to Actions tab
   - Click on failed workflow
   - Expand failed step logs

2. **Download Debug Artifacts:**
   - Available for 7 days after failed builds
   - Contains build directories and logs

3. **Test Locally:**
   ```bash
   # Test macOS build
   ./build-macos.sh
   
   # Test Windows build  
   build-windows.bat
   ```

## 🔐 Security & Signing

### Code Signing (Optional)

**macOS:**
Add to your build script:
```bash
# In build-macos.sh, after macdeployqt
codesign --force --deep --sign "Developer ID Application: Your Name" "$APP_BUNDLE"
```

**Windows:**
Add signing step to workflow:
```yaml
- name: Sign Windows executable
  run: |
    # Use signtool.exe with your certificate
```

### Release Security
- Releases are created with `GITHUB_TOKEN`
- No additional secrets required for basic setup
- For code signing, add certificates to GitHub Secrets

## 📊 Build Statistics

Typical build times:
- **macOS:** 5-8 minutes
- **Windows:** 6-10 minutes
- **Total pipeline:** 15-20 minutes

Artifact sizes:
- **macOS DMG:** ~50-80MB
- **Windows ZIP:** ~60-90MB

## 🎯 Best Practices

### Version Management
1. Use semantic versioning
2. Tag releases from main branch
3. Keep develop branch for ongoing work

### Release Process
1. Merge features to develop
2. Test thoroughly
3. Merge develop to main
4. Tag release on main
5. GitHub Actions handles the rest

### Quality Assurance
- CI runs on every PR
- Both platforms tested automatically
- Manual testing before major releases

## 📱 Distribution

### For End Users
**macOS:**
1. Download `.dmg` file
2. Open and drag to Applications
3. May need to allow in Security & Privacy

**Windows:** 
1. Download `.zip` file
2. Extract anywhere
3. Run `nwss-cnc.exe`

### For Developers
- Source code available on GitHub
- Build scripts work locally
- Same Qt version as CI/CD

## 🔄 Updating the Pipeline

### Adding New Platforms
To add Linux builds:
```yaml
# Add to matrix in workflows
- os: ubuntu-latest
  qt_arch: gcc_64
  build_script: ./build-linux.sh
```

### Changing Qt Version
1. Update workflow files
2. Test locally first
3. Update documentation

### Adding Tests
Add test steps to `ci.yml`:
```yaml
- name: Run tests
  run: |
    cd build-*/
    ctest --output-on-failure
```

This automated setup ensures your users get professionally packaged, dependency-free binaries without needing any development tools! 