# NWSS-CNC ![](https://tokei.rs/b1/github/frikallo/nwss-cnc?category=code)

<p align="center">
  <img src="./docs/1.png" width="350">
  <img src="./docs/2.png" width="350">
  <img src="./docs/3.png" width="350">
  <img src="./docs/4.png" width="350">
</p>


## Table of Contents

1. [Introduction](#introduction)
2. [System Overview](#system-overview)
3. [Installation and Setup](#installation-and-setup)
4. [User Interface Guide](#user-interface-guide)
5. [Core Features](#core-features)
6. [Technical Architecture](#technical-architecture)
7. [Advanced Features](#advanced-features)
8. [Tool Management](#tool-management)
9. [Configuration and Settings](#configuration-and-settings)
10. [Troubleshooting](#troubleshooting)
11. [Development and Extension](#development-and-extension)
12. [Appendices](#appendices)

---

## 1. Introduction

### What is NWSS-CNC?

NWSS-CNC is a professional 2D Computer-Aided Manufacturing (CAM) software designed to convert vector graphics (SVG files) into G-code for CNC machines. The application provides a complete workflow from design visualization to machine-ready code generation, featuring advanced path optimization, tool management, and real-time 3D visualization.

### Key Features

- **SVG to G-Code Conversion**: Import SVG files and convert them to optimized G-code
- **3D Visualization**: Real-time 3D preview of toolpaths with advanced navigation controls
- **Professional CAM Operations**: Support for perimeter cutting, pocketing, punching, and engraving
- **Tool Management**: Comprehensive tool database with automatic parameter optimization
- **Advanced Path Processing**: Clipper2-based polygon operations for professional results
- **Material Management**: Precise material and bed size configuration
- **Smooth UX**: Modern, professional interface with full customizability and user-experience prioritized

### Target Users

- First-time machinists
- CNC machine operators
- CAM professionals
- Hobbyists and makers
- Educational institutions
- Small to medium manufacturing shops

---

## 2. System Overview

### Software Architecture

NWSS-CNC is built using Qt6 with C++17, providing cross-platform compatibility for Windows, macOS, and Linux. The application follows a modular architecture with clear separation between [core processing logic](./nwss-cnc/src/core/) and [user interface components](./nwss-cnc/src/gui/).

### Core Technologies

- **Qt6 Framework**: Cross-platform GUI framework
- **OpenGL**: 3D graphics rendering for toolpath visualization
- **NanoSVG**: Lightweight SVG parsing library
- **Clipper2**: Advanced 2D polygon clipping library
- **Modern C++17**: High-performance core algorithms

### Supported File Formats

**Input Formats:**
- SVG (Scalable Vector Graphics) - Primary input format
- Existing G-code files for editing, visualization, and automatic conversion
- DXF (Drawing Exchange Format) - Future support planned
- Other vector formats (future support planned)
- **STL** - Future support planned for 3D models

**Output Formats:**
- G-code (ISO 6983 standard)

---

## 3. Installation and Setup

### Installation Process

#### Windows
1. Download the installer from the [release page](https://github.com/Frikallo/NWSS-CNC/releases)
2. Run the installer as administrator
3. Follow the installation wizard
4. Launch NWSS-CNC from the desktop shortcut, start menu, or Program Files directory

#### macOS
1. Download the .dmg file
2. Open the disk image
3. Drag NWSS-CNC to the Applications folder
4. Launch from Applications or Launchpad
5. If MacOS blocks the application, go to System Preferences > Security & Privacy and allow it under the "General" tab

#### Linux - Very soon supported
1. Download the AppImage or use the package manager
2. Make the file executable: `chmod +x NWSS-CNC.AppImage`
3. Run the application: `./NWSS-CNC.AppImage`

### First Launch Setup

On first launch, NWSS-CNC presents a welcome dialog with options to:
- Create a new empty project
- Open an existing G-code file
- Import an SVG file for conversion

The application will automatically create default configuration files and tool databases.

---

## 4. User Interface Guide

### Main Window Layout

The NWSS-CNC interface consists of several key areas:

#### 4.1 Menu Bar
- **File Menu**: New, Open, Save, Import SVG, Exit
- **Edit Menu**: Cut, Copy, Paste operations
- **View Menu**: Toggle docks, zoom controls
- **Tools Menu**: Tool management, settings
- **Help Menu**: About dialog, documentation

#### 4.2 Toolbar
Quick access buttons for common operations:
- New file
- Open file
- Save file
- Import SVG
- Tool management

#### 4.3 Main Content Area (Tabbed Interface)

**SVG Designer Tab:**
- Vector graphics viewer and editor
- Material and bed boundary visualization
- Grid and snap controls
- Measurement tools
- Design transformation controls

**G-Code Editor Tab:**
- Syntax-highlighted G-code editor
- Line numbering

**3D Viewer Tab:**
- Interactive 3D toolpath visualization
- Navigation cube for view control
- CAD-style camera controls
- Real-time rendering of toolpaths
- Animation controls
- Level-of-detail (LOD) optimization

#### 4.4 Dock Panels

**G-Code Options Panel:**
- Material settings (width, height, thickness)
- Machine settings (bed size, units)
- Cutting parameters (feed rate, spindle speed, depth, pass count)
- Path optimization options
- Tool selection and parameters
- Cutting mode selection (perimeter or punchout)

**Tool Management Panel:**
- Tool library browser
- Tool parameter editor
- Material-specific recommendations
- Tool validation and warnings

#### 4.5 Status Bar
- Current operation status
- Time estimates
- File information

### 4.6 Theme and Appearance

NWSS-CNC features a professional black and orange theme optimized for CNC work environments:
- High contrast for readability
- Reduced eye strain during long sessions
- Professional appearance
- Customizable via Qt stylesheets

---

## 5. Core Features

### 5.1 SVG Import and Processing

#### Supported SVG Elements
- Paths (lines, curves, bezier curves)
- Basic shapes (rectangles, circles, ellipses)
- Text (converted to paths)
- Groups and layers
- Transformations (scale, rotate, translate)

#### SVG Processing Pipeline
1. **Parsing**: NanoSVG library parses SVG structure
2. **Shape Extraction**: Individual shapes are identified and cataloged
3. **Path Discretization**: Curves are converted to linear segments
4. **Optimization**: Redundant points are removed
5. **Validation**: Geometry is checked for manufacturing feasibility

### 5.2 Path Discretization

The discretization process converts smooth curves into discrete points suitable for CNC machining:

#### Adaptive Sampling
- Automatically adjusts point density based on curve complexity
- Maintains accuracy while minimizing file size
- Configurable tolerance settings

#### Fixed Sampling
- User-defined number of points per curve segment
- Predictable output for specific requirements
- Compatible with older CNC controllers

### 5.3 CAM Operations

#### 5.3.1 Perimeter Cutting
- Cuts along the outline of shapes
- Supports inside, outside, and on-path offsets
- Automatic tool compensation
- Lead-in/lead-out options

#### 5.3.2 Punching/Cutout
- Complete material removal through thickness
- Optimized for parts production
- Automatic tab generation (future feature)
- Nest optimization support

### 5.4 G-Code Generation

#### Output Features
- ISO 6983 compliant G-code
- Customizable header and footer
- Tool change sequences
- Spindle control commands
- Safety height management

#### Optimization Features
- Path ordering for minimum travel time
- Rapid move optimization
- Redundant move elimination
- Linear interpolation for straight segments

---

## 6. Technical Architecture

### 6.1 Core Components

#### SVGParser Class
```cpp
class SVGParser {
    // Parses SVG files using NanoSVG
    // Extracts shape information and dimensions
    // Provides access to raw SVG data
}
```

#### Discretizer Class
```cpp
class Discretizer {
    // Converts bezier curves to linear segments
    // Implements adaptive and fixed sampling
    // Handles path simplification
}
```

#### GCodeGenerator Class
```cpp
class GCodeGenerator {
    // Generates ISO-compliant G-code
    // Handles tool changes and parameters
    // Implements path optimization
}
```

#### CAMProcessor Class
```cpp
class CAMProcessor {
    // Professional CAM operations using Clipper2
    // Polygon hierarchy analysis
    // Advanced toolpath generation
}
```

### 6.2 Data Structures

#### Point2D Structure
```cpp
struct Point2D {
    double x, y;
    // Basic 2D point with utility methods
}
```

#### Path Class
```cpp
class Path {
    std::vector<Point2D> m_points;
    // Represents a sequence of connected points
    // Includes length calculation and simplification
}
```

#### Polygon Class
```cpp
class Polygon {
    std::vector<Point2D> m_points;
    // Closed polygon for area operations
    // Point-in-polygon testing
    // Area calculation
}
```

### 6.3 Processing Pipeline

1. **SVG Loading**: File is parsed and validated
2. **Shape Extraction**: Individual elements are identified
3. **Discretization**: Curves are converted to point sequences
4. **Transformation**: Scaling and positioning applied
5. **CAM Processing**: Toolpaths are generated based on operation type
6. **Optimization**: Paths are optimized for efficiency
7. **G-Code Generation**: Final machine code is produced
8. **Validation**: Output is checked for errors

### 6.4 Thread Safety and Performance

- Core processing runs on background threads
- UI remains responsive during long operations
- Memory-efficient processing of large files
- Optimized data structures for speed

---

## 7. Advanced Features

### 7.1 Professional CAM Operations

#### Polygon Hierarchy Analysis
The CAMProcessor implements sophisticated polygon analysis to handle complex nested shapes:
- Automatic hole detection
- Parent-child relationships
- Optimal machining sequence
- Island machining support

#### Clipper2 Integration
Advanced 2D polygon operations powered by Clipper2:
- Union operations for combining shapes
- Difference operations for hole creation
- Intersection for feature detection
- Offset operations for tool compensation

### 7.2 Tool Offset Compensation

#### Offset Types
- **Inside Offset**: Tool center runs inside the path (for cutting out parts)
- **Outside Offset**: Tool center runs outside the path (for pockets)
- **On Path**: Tool center follows the exact path (for engraving)
- **Auto**: Automatically determined based on shape analysis

#### Smart Offset Calculation
- Automatic corner rounding for inside corners
- Sharp corner handling for outside corners
- Collision detection and avoidance
- Minimum feature size validation

### 7.3 Advanced Path Optimization

#### Travel Optimization
- Minimum spanning tree algorithms
- Nearest neighbor heuristics
- Custom optimization for specific machine characteristics

#### Path Smoothing
- Bezier curve fitting for smoother motion
- Acceleration-aware path planning
- Jerk minimization for better surface finish

### 7.4 3D Visualization Features

#### Real-time Rendering
- OpenGL-based 3D graphics
- Smooth camera controls
- Multiple view modes (isometric, top, side)
- Interactive navigation cube

#### Performance Optimization
- Level-of-detail (LOD) rendering
- Frustum culling
- Batch rendering for large toolpaths
- Frame rate limiting

#### Visual Feedback
- Color-coded rapid vs. cutting moves
- Tool representation
- Material boundaries
- Grid and measurement overlays

---

## 8. Tool Management

### 8.1 Tool Database Structure

#### Tool Properties
```cpp
struct Tool {
    int id;                    // Unique identifier
    std::string name;          // Tool description
    ToolType type;             // End mill, ball nose, V-bit, etc.
    double diameter;           // Tool diameter (mm)
    double length;             // Overall length
    double fluteLength;        // Cutting length
    int fluteCount;            // Number of flutes
    ToolMaterial material;     // HSS, Carbide, etc.
    ToolCoating coating;       // TiN, TiAlN, etc.
    double maxDepthOfCut;      // Maximum depth per pass
    double maxFeedRate;        // Maximum feed rate
    double maxSpindleSpeed;    // Maximum RPM
    double minSpindleSpeed;    // Minimum RPM
    std::string notes;         // Additional information
    bool isActive;             // Tool availability
};
```

#### Tool Types Supported
- **End Mills**: General purpose cutting tools
- **Ball Nose**: 3D contouring and finishing
- **V-Bits**: Engraving and chamfering
- **Drills**: Hole making
- **Router Bits**: Wood and plastic cutting
- **Engraving Bits**: Fine detail work
- **Custom**: User-defined tools

### 8.2 Automatic Parameter Calculation

#### Feed Rate Calculation
The system automatically calculates optimal feed rates based on:
- Tool material and coating
- Workpiece material
- Tool diameter and geometry
- Spindle speed
- Depth of cut

#### Spindle Speed Optimization
Automatically determines optimal RPM considering:
- Tool material and diameter
- Workpiece material properties
- Surface finish requirements
- Tool manufacturer recommendations

### 8.3 Tool Validation

#### Feature Size Validation
- Checks if tool can physically machine the geometry
- Warns about features smaller than tool diameter
- Suggests alternative tools when appropriate

#### Parameter Validation
- Ensures feed rates are within tool limits
- Validates spindle speeds against tool specifications
- Checks depth of cut against tool capabilities

---

## 9. Configuration and Settings (Can be changed in the GUI or in the config file)

### 9.1 Machine Configuration

#### CNConfig Class
```cpp
class CNConfig {
    // Machine physical properties
    double m_bedWidth;         // CNC bed width
    double m_bedHeight;        // CNC bed height
    MeasurementUnit m_units;   // mm or inches
    
    // Material properties
    double m_materialWidth;    // Workpiece width
    double m_materialHeight;   // Workpiece height
    double m_materialThickness; // Material thickness
    
    // Cutting parameters
    double m_feedRate;         // XY feed rate
    double m_plungeRate;       // Z feed rate
    int m_spindleSpeed;        // Spindle RPM
    double m_cutDepth;         // Depth per pass
    int m_passCount;           // Number of passes
    double m_safeHeight;       // Safe travel height
};
```

### 9.2 Discretization Settings

#### DiscretizerConfig Structure
```cpp
struct DiscretizerConfig {
    int bezierSamples;         // Points per curve segment
    double simplifyTolerance;  // Path simplification threshold
    double adaptiveSampling;   // Adaptive sampling tolerance
    double maxPointDistance;   // Maximum point spacing
};
```

### 9.3 G-Code Generation Options

#### GCodeOptions Structure
```cpp
struct GCodeOptions {
    bool includeComments;      // Add descriptive comments
    bool useInches;            // Output in inches vs. mm
    bool includeHeader;        // Generate file header
    bool optimizePaths;        // Optimize path order
    bool linearizePaths;       // Combine straight segments
    bool enableToolOffsets;    // Apply tool compensation
    CutoutMode cutoutMode;     // Cutting operation type
    double stepover;           // Area cutting stepover
    bool spiralIn;             // Spiral cutting direction
};
```

#### File Format
Configuration files use INI format with sections:
```ini
[Machine]
BedWidth=300.0
BedHeight=200.0
Units=mm

[Material]
Width=100.0
Height=100.0
Thickness=3.0

[Cutting]
FeedRate=1000.0
PlungeRate=300.0
SpindleSpeed=12000
```

---

## 10. Troubleshooting

### 10.1 Common Issues

#### SVG Import Problems
**Problem**: SVG file won't load or appears corrupted
**Solutions**:
1. Verify SVG file is valid XML
2. Check for unsupported SVG features
3. Try re-saving from original design software
4. Use SVG optimization tools to clean up the file

**Problem**: Missing shapes or incorrect scaling
**Solutions**:
1. Check SVG viewBox and dimensions
2. Verify units are consistent
3. Use "Fit to View" in the designer
4. Check for invisible layers or groups

#### G-Code Generation Issues
**Problem**: Empty or invalid G-code output
**Solutions**:
1. Verify paths are properly closed
2. Check material and bed size settings
3. Ensure tool is selected and valid
4. Review cutting parameters for sanity

**Problem**: Toolpaths appear incorrect in 3D view
**Solutions**:
1. Check tool offset settings
2. Verify cutting mode selection
3. Review stepover and overlap settings
4. Check for self-intersecting geometry

#### Performance Issues
**Problem**: Slow processing or UI lag
**Solutions**:
1. Reduce SVG complexity
2. Lower discretization quality temporarily
3. Close unnecessary applications
4. Increase system RAM if possible

**Problem**: 3D viewer is slow or unresponsive
**Solutions**:
1. Update graphics drivers
2. Reduce toolpath complexity
3. Enable performance optimizations
4. Use lower level of detail

### 10.2 Error Messages

#### "Tool diameter too large for feature"
This warning indicates that the selected tool cannot machine all features in the design. Consider:
- Using a smaller tool
- Modifying the design to accommodate the tool
- Using multiple tools for different features

#### "Invalid cutting parameters"
Check that:
- Feed rates are within reasonable limits
- Spindle speed is appropriate for the tool
- Depth of cut doesn't exceed tool capabilities
- Material properties are correctly specified

#### "Polygon self-intersection detected"
The input geometry has overlapping or self-intersecting paths:
- Simplify the design
- Fix overlapping shapes in the original software
- Use the built-in geometry repair tools

---

## 11. Development and Extension

### 11.1 Build System

#### Requirements
- CMake 3.16 or later
- Qt6 (Core, Widgets, OpenGL, SVG components)
- C++17 compatible compiler
- Git for source code management

#### Build Process
```bash
# Clone repository
git clone https://github.com/nwss/nwss-cnc.git
cd nwss-cnc

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j4  # Linux/macOS
# or
cmake --build . --config Release  # Windows
```

### 11.2 Project Structure

```
nwss-cnc/
├── include/
│   ├── core/           # Core processing classes
│   └── gui/            # User interface classes
├── src/
│   ├── core/           # Core implementation
│   ├── gui/            # GUI implementation
│   └── main.cpp        # Application entry point
├── resources/          # Icons, fonts, themes
├── third_party/        # External libraries
├── example_files/      # Sample SVG files
└── docs/               # Documentation and images
```

### 11.3 Adding New Features

#### Core Processing
To add new CAM operations:
1. Extend the `CutoutMode` enum in `geometry.h`
2. Implement the algorithm in `CAMProcessor`
3. Add UI controls in `GCodeOptionsPanel`
4. Update the G-code generator

#### User Interface
To add new GUI features:
1. Create new Qt widgets in `include/gui/`
2. Implement in `src/gui/`
3. Integrate with `MainWindow`
4. Add to menu system and toolbars

#### Tool Support
To add new tool types:
1. Extend the `ToolType` enum in `tool.h`
2. Update parameter calculation methods
3. Add UI support in tool manager
4. Update validation logic

---

## 12. Appendices

### Appendix A: G-Code Commands Reference

#### Common G-Codes Generated
- `G0`: Rapid positioning (non-cutting moves)
- `G1`: Linear interpolation (cutting moves)
- `G2/G3`: Circular interpolation (arcs)
- `G17`: XY plane selection
- `G20/G21`: Inch/Metric units
- `G90/G91`: Absolute/Incremental positioning
- `G94`: Feed rate per minute mode

#### M-Codes Used
- `M3/M4`: Spindle start (clockwise/counterclockwise)
- `M5`: Spindle stop
- `M8/M9`: Coolant on/off
- `M30`: Program end and return

### Appendix B: Supported SVG Features

#### Fully Supported
- Paths with line, curve, and arc segments
- Basic shapes (rect, circle, ellipse, polygon)
- Groups and transformations
- Stroke properties
- Units and viewBox

#### Partially Supported
- Text (converted to paths)
- Complex gradients (ignored)
- Filters and effects (ignored)
- Animations (ignored)

#### Not Supported
- Raster images
- Complex CSS styling
- JavaScript interactions
- External references

#### Tool Database Format (.json) (Editable in the GUI)
JSON format for tool library storage:
```json
{
  "tools": [
    {
      "id": 1,
      "name": "1/8\" End Mill",
      "type": "END_MILL",
      "diameter": 3.175,
      "material": "CARBIDE",
      "maxFeedRate": 2000.0,
      "maxSpindleSpeed": 18000
    }
  ]
}
```

### Appendix D: Keyboard Shortcuts

#### Global Shortcuts
- `Ctrl+N`: New file
- `Ctrl+O`: Open file
- `Ctrl+S`: Save file
- `Ctrl+Shift+S`: Save as
- `Ctrl+Q`: Quit application

#### Designer Shortcuts
- `Ctrl+A`: Select all
- `Delete`: Delete selected
- `Ctrl+Z`: Undo
- `Ctrl+Y`: Redo
- `Ctrl++`: Zoom in
- `Ctrl+-`: Zoom out
- `Ctrl+0`: Reset zoom

#### 3D Viewer Shortcuts
- Mouse drag: Rotate view
- `Shift+drag`: Pan view
- Mouse wheel: Zoom
- `R`: Reset view
- `F`: Fit to view

### Appendix F: Safety Considerations

#### Machine Safety
- Always verify G-code before running on actual machine
- Check tool paths don't exceed material boundaries
- Verify safe height settings
- Use appropriate feeds and speeds for material

#### Software Safety
- Regularly backup tool databases and configurations
- Keep software updated
- Verify calculations with independent tools
- Test with scrap material first

---

## Conclusion

NWSS-CNC represents a comprehensive solution for 2D CAM operations, combining professional-grade algorithms with an intuitive user interface. The software's modular architecture and extensive feature set make it suitable for both hobby and professional applications.

The application's commitment to open standards, cross-platform compatibility, and extensible design ensures it can adapt to evolving manufacturing needs while maintaining compatibility with existing workflows.

For additional support, contact me personally at noahkay13@gmail.com, or open an issue [here](https://github.com/Frikallo/NWSS-CNC/issues) and I will get back to you as soon as possible.

---

*This document serves as both a user guide and technical reference for NWSS-CNC version 1.0.0. For the most current information, please refer to the online documentation.*