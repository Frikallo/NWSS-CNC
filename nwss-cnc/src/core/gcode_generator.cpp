#include "core/gcode_generator.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "core/tool_offset.h"

namespace nwss {
namespace cnc {

GCodeGenerator::GCodeGenerator() = default;
GCodeGenerator::~GCodeGenerator() = default;

void GCodeGenerator::setConfig(const CNConfig &config) { m_config = config; }

void GCodeGenerator::setOptions(const GCodeOptions &options) {
  m_options = options;
}

void GCodeGenerator::setToolRegistry(const ToolRegistry &registry) {
  m_toolRegistry = registry;
}

bool GCodeGenerator::generateGCode(const std::vector<Path> &paths,
                                   const std::string &outputFile) const {
  std::ofstream file(outputFile);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file for writing: " << outputFile
              << std::endl;
    return false;
  }

  // Validate paths if enabled
  if (m_options.validateFeatureSizes) {
    std::cout << "DEBUG: Feature size validation ENABLED" << std::endl;
    std::vector<std::string> warnings;
    if (!validatePaths(paths, warnings)) {
      std::cout << "DEBUG: Feature validation found issues:" << std::endl;
      std::cerr
          << "Warning: Some features may be too small for the selected tool:"
          << std::endl;
      for (const auto &warning : warnings) {
        std::cerr << "  " << warning << std::endl;
      }
    } else {
      std::cout << "DEBUG: All features validated successfully" << std::endl;
    }
  } else {
    std::cout << "DEBUG: Feature size validation DISABLED" << std::endl;
  }

  // Apply tool offsets if enabled
  std::cout << "DEBUG: GCode generation - Tool offsets "
            << (m_options.enableToolOffsets ? "ENABLED" : "DISABLED")
            << std::endl;
  if (m_options.enableToolOffsets) {
    std::cout << "DEBUG: ========================================" << std::endl;
    std::cout << "DEBUG: NEW HIGH-PRECISION OFFSET ALGORITHM" << std::endl;
    std::cout << "DEBUG: ========================================" << std::endl;
    std::cout << "DEBUG: Features:" << std::endl;
    std::cout << "  - Sub-millimeter precision for detailed cuts" << std::endl;
    std::cout << "  - Robust curve handling (not just line segments)"
              << std::endl;
    std::cout << "  - Topology preservation for shape integrity" << std::endl;
    std::cout << "  - Adaptive strategies based on feature size" << std::endl;
    std::cout << "  - Extensive quality validation" << std::endl;
    std::cout << "DEBUG: ========================================" << std::endl;
  }
  std::vector<Path> processedPaths =
      m_options.enableToolOffsets ? applyToolOffsets(paths) : paths;

  if (m_options.enableToolOffsets) {
    std::cout << "DEBUG: Tool offsets applied - processed "
              << processedPaths.size() << " paths" << std::endl;
  } else {
    std::cout << "DEBUG: Using original paths without offsets - "
              << paths.size() << " paths" << std::endl;
  }

  // Check if we need area cutting
  std::vector<Path> finalPaths;
  if (m_options.cutoutMode != CutoutMode::PERIMETER) {
    std::cout << "DEBUG: Using area cutting mode: "
              << static_cast<int>(m_options.cutoutMode) << std::endl;

    // Convert paths to polygons
    std::vector<Polygon> polygons = pathsToPolygons(processedPaths);
    std::cout << "DEBUG: Converted " << processedPaths.size() << " paths to "
              << polygons.size() << " polygons" << std::endl;

    // Generate area cutting paths
    finalPaths = generateAreaCuttingPaths(polygons);
    std::cout << "DEBUG: Generated " << finalPaths.size()
              << " area cutting paths" << std::endl;
  } else {
    std::cout << "DEBUG: Using perimeter cutting mode" << std::endl;
    finalPaths = processedPaths;
  }

  // Set precision for output
  file << std::fixed << std::setprecision(4);

  // Write header
  if (m_options.includeHeader) {
    writeHeader(file);
  }

  // Process each path
  for (size_t pathIndex = 0; pathIndex < finalPaths.size(); pathIndex++) {
    const auto &path = finalPaths[pathIndex];
    if (path.empty()) continue;

    writePath(file, path, pathIndex);
  }

  // Write footer
  writeFooter(file);

  file.close();
  return true;
}

std::string GCodeGenerator::generateGCodeString(
    const std::vector<Path> &paths) const {
  // Apply tool offsets if enabled
  std::cout << "DEBUG: GCode string generation - Tool offsets "
            << (m_options.enableToolOffsets ? "ENABLED" : "DISABLED")
            << std::endl;
  std::vector<Path> processedPaths =
      m_options.enableToolOffsets ? applyToolOffsets(paths) : paths;

  if (m_options.enableToolOffsets) {
    std::cout
        << "DEBUG: Tool offsets applied for string generation - processed "
        << processedPaths.size() << " paths" << std::endl;
  } else {
    std::cout << "DEBUG: Using original paths for string generation - "
              << paths.size() << " paths" << std::endl;
  }

  // Check if we need area cutting
  std::vector<Path> finalPaths;
  if (m_options.cutoutMode != CutoutMode::PERIMETER) {
    std::cout << "DEBUG: Using area cutting mode for string generation: "
              << static_cast<int>(m_options.cutoutMode) << std::endl;

    // Convert paths to polygons
    std::vector<Polygon> polygons = pathsToPolygons(processedPaths);
    std::cout << "DEBUG: Converted " << processedPaths.size() << " paths to "
              << polygons.size() << " polygons" << std::endl;

    // Generate area cutting paths
    finalPaths = generateAreaCuttingPaths(polygons);
    std::cout << "DEBUG: Generated " << finalPaths.size()
              << " area cutting paths" << std::endl;
  } else {
    std::cout << "DEBUG: Using perimeter cutting mode for string generation"
              << std::endl;
    finalPaths = processedPaths;
  }

  std::stringstream ss;
  ss << std::fixed << std::setprecision(4);

  // Write header
  if (m_options.includeHeader) {
    writeHeader(ss);
  }

  // Process each path
  for (size_t pathIndex = 0; pathIndex < finalPaths.size(); pathIndex++) {
    const auto &path = finalPaths[pathIndex];
    if (path.empty()) continue;

    writePath(ss, path, pathIndex);
  }

  // Write footer
  writeFooter(ss);

  return ss.str();
}

void GCodeGenerator::writeHeader(std::ostream &out) const {
  // Get configuration values
  double feedRate = m_config.getFeedRate();
  int spindleSpeed = m_config.getSpindleSpeed();
  double cutDepth = m_config.getCutDepth();
  int passCount = m_config.getPassCount();
  double safeHeight = m_config.getSafeHeight();
  std::string units = m_options.useInches ? "in" : m_config.getUnitsString();

  // Write header comments only if enabled
  if (m_options.includeComments) {
    // out << "( Generated by NWSS-CNC Software )" << std::endl;
    // out << "( Units: " << units << " )" << std::endl;
    // out << "( Material size: " << m_config.getMaterialWidth() << " x "
    //     << m_config.getMaterialHeight() << " x "
    //     << m_config.getMaterialThickness() << " " << units << " )" <<
    //     std::endl;
    // out << "( Cut depth: " << cutDepth << " " << units
    //     << " x " << passCount << " passes )" << std::endl;
    // out << "( Feed rate: " << feedRate << " " << units << "/min )" <<
    // std::endl;

    // Include any additional comments
    if (!m_options.comments.empty()) {
      out << "( " << m_options.comments << " )" << std::endl;
    }

    out << std::endl;
  }

  // Initial setup
  if (m_options.useInches) {
    out << "G20" << std::endl;
  } else {
    out << "G21" << std::endl;
  }

  out << "G90" << std::endl;

  // Tool selection and offset compensation
  const Tool *tool = m_toolRegistry.getTool(m_options.selectedToolId);
  if (tool && m_options.enableToolOffsets) {
    // Tool selection
    out << "T" << tool->id << " M06" << std::endl;

    // Enable tool length compensation
    out << "G43 H" << tool->id << std::endl;

    if (m_options.includeComments) {
      out << "( Tool: " << tool->name << ", Diameter: " << tool->diameter
          << "mm )" << std::endl;
      out << "( Tool offset compensation enabled )" << std::endl;
    }
  } else if (tool) {
    // Tool selection without offset compensation
    out << "T" << tool->id << " M06" << std::endl;

    if (m_options.includeComments) {
      out << "( Tool: " << tool->name << ", Diameter: " << tool->diameter
          << "mm )" << std::endl;
      out << "( Tool offset compensation disabled )" << std::endl;
    }
  }

  out << "M03 S" << spindleSpeed << std::endl;
  out << "G00 Z" << safeHeight << std::endl << std::endl;
}

void GCodeGenerator::writeFooter(std::ostream &out) const {
  if (m_options.returnToOrigin) {
    out << "G00 Z" << m_config.getSafeHeight() << std::endl;
    out << "G00 X0 Y0" << std::endl;
  }

  // Cancel tool offset compensation if it was enabled
  if (m_options.enableToolOffsets) {
    out << "G49" << std::endl;
    if (m_options.includeComments) {
      out << "( Tool offset compensation canceled )" << std::endl;
    }
  }

  out << "M05" << std::endl;
  out << "END" << std::endl;
}

void GCodeGenerator::linearizePath(std::ostream &out,
                                   const std::vector<Point2D> &points,
                                   double feedRate) const {
  if (points.size() < 2) return;

  // Start with the first point
  size_t lineStart = 0;

  while (lineStart < points.size() - 1) {
    size_t lineEnd = lineStart + 1;

    // Try to extend the current line as far as possible
    while (
        lineEnd + 1 < points.size() &&
        isCollinear(points[lineStart], points[lineEnd], points[lineEnd + 1])) {
      lineEnd++;
    }

    // Output the line from start to end
    out << "G01 X" << points[lineEnd].x << " Y" << points[lineEnd].y << " F"
        << feedRate;

    // Add a comment if this is a linearized segment and comments are enabled
    if (m_options.includeComments && lineEnd > lineStart + 1) {
      out << "  ; Linearized segment (" << (lineEnd - lineStart + 1)
          << " points)";
    }
    out << std::endl;

    // Move to the next line
    lineStart = lineEnd;
  }
}

bool GCodeGenerator::isCollinear(const Point2D &p1, const Point2D &p2,
                                 const Point2D &p3) const {
  // Calculate the area of the triangle formed by the three points
  // If the area is close to zero, the points are collinear
  double area = 0.5 * std::abs((p2.x - p1.x) * (p3.y - p1.y) -
                               (p3.x - p1.x) * (p2.y - p1.y));

  // Check if the area is less than the tolerance
  return area < m_options.linearizeTolerance;
}

void GCodeGenerator::writePath(std::ostream &out, const Path &path,
                               size_t pathIndex) const {
  const auto &points = path.getPoints();
  if (points.empty()) return;

  // Get configuration values
  double feedRate = m_config.getFeedRate();
  double plungeRate = m_config.getPlungeRate();
  double cutDepth = m_config.getCutDepth();
  int passCount = m_config.getPassCount();
  double safeHeight = m_config.getSafeHeight();
  double materialThickness = m_config.getMaterialThickness();

  // Add path comment only if comments are enabled
  if (m_options.includeComments) {
    out << "( Path " << pathIndex << " )" << std::endl;
  }

  // Ensure we're at safe height before rapid move to start point
  out << "G00 Z" << safeHeight;
  if (m_options.includeComments) {
    out << "  ; Retract to safe height before rapid move";
  }
  out << std::endl;

  // Move to the start point of the path
  out << "G00 X" << points[0].x << " Y" << points[0].y;
  if (m_options.includeComments) {
    out << "  ; Rapid to start point";
  }
  out << std::endl;

  // Make multiple passes if needed
  for (int pass = 0; pass < passCount; pass++) {
    double depth = -cutDepth * (pass + 1);

    // Ensure we don't cut deeper than the material thickness
    if (std::abs(depth) > materialThickness) {
      depth = -materialThickness;
      if (m_options.includeComments) {
        out << "( Note: Depth limited to material thickness )" << std::endl;
      }
    }

    // Plunge to depth
    out << "G01 Z" << depth << " F" << plungeRate;
    if (m_options.includeComments) {
      out << "  ; Plunge to depth (pass " << (pass + 1) << ")";
    }
    out << std::endl;

    if (m_options.linearizePaths && points.size() > 2) {
      // Linearized path generation
      linearizePath(out, points, feedRate);
    } else {
      // Standard path generation (point by point)
      for (size_t i = 1; i < points.size(); i++) {
        out << "G01 X" << points[i].x << " Y" << points[i].y << " F" << feedRate
            << std::endl;
      }
    }

    // Close the loop if requested and not already closed
    if (m_options.closeLoops && !points.empty() && points.size() > 2) {
      const auto &first = points.front();
      const auto &last = points.back();

      // Check if start and end points are different
      double dx = first.x - last.x;
      double dy = first.y - last.y;
      double distance = std::sqrt(dx * dx + dy * dy);

      // If the distance is significant, close the loop
      if (distance > 0.001) {
        out << "G01 X" << first.x << " Y" << first.y << " F" << feedRate;
        if (m_options.includeComments) {
          out << "  ; Close loop";
        }
        out << std::endl;
      }
    }

    // Retract to safe height between passes and after the last pass
    out << "G00 Z" << safeHeight;
    if (m_options.includeComments) {
      out << "  ; Retract to safe height";
    }
    out << std::endl;
  }

  // No need for separate retract option since we always retract now
  // Keep an empty line between paths for readability
  out << std::endl;
}

GCodeGenerator::TimeEstimate GCodeGenerator::calculateTimeEstimate(
    const std::vector<Path> &paths) const {
  TimeEstimate estimate;
  estimate.rapidTime = 0.0;
  estimate.cuttingTime = 0.0;
  estimate.totalTime = 0.0;
  estimate.totalDistance = 0.0;
  estimate.rapidDistance = 0.0;
  estimate.cuttingDistance = 0.0;

  // Get feed rates from config
  double feedRate = m_config.getFeedRate();      // Units per minute
  double plungeRate = m_config.getPlungeRate();  // Units per minute
  double rapidRate = 3000.0;  // Default rapid rate (units per minute)

  // Convert to units per second for calculations
  double feedRatePerSec = feedRate / 60.0;
  double plungeRatePerSec = plungeRate / 60.0;
  double rapidRatePerSec = rapidRate / 60.0;

  // Number of passes needed
  int passCount = m_config.getPassCount();

  // Process each path
  for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
    const auto &path = paths[pathIndex];
    if (path.empty()) continue;

    const auto &points = path.getPoints();

    // Calculate for each pass
    for (int pass = 0; pass < passCount; pass++) {
      // Initial plunge time - from safe height to cutting depth
      double passDepth = m_config.getCutDepth() * (pass + 1);
      double plungeDistance = passDepth;
      double plungeTime = plungeDistance / plungeRatePerSec;
      estimate.cuttingTime += plungeTime;

      // Initial rapid move to the start point (first pass only)
      if (pass == 0 && pathIndex > 0) {
        // Calculate distance from previous path's end point to current path's
        // start point
        const auto &prevPath = paths[pathIndex - 1];
        if (!prevPath.empty()) {
          const auto &prevEndPoint = prevPath.getPoints().back();
          const auto &currentStartPoint = points.front();

          double dx = currentStartPoint.x - prevEndPoint.x;
          double dy = currentStartPoint.y - prevEndPoint.y;
          double moveDistance = std::sqrt(dx * dx + dy * dy);

          estimate.rapidDistance += moveDistance;
          estimate.rapidTime += moveDistance / rapidRatePerSec;
        }
      }

      // Add up all segment lengths
      for (size_t i = 1; i < points.size(); i++) {
        const auto &p1 = points[i - 1];
        const auto &p2 = points[i];

        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double distance = std::sqrt(dx * dx + dy * dy);

        estimate.cuttingDistance += distance;
        estimate.cuttingTime += distance / feedRatePerSec;
      }

      // Retract time - from cutting depth to safe height
      double retractDistance = m_config.getSafeHeight() + passDepth;
      double retractTime = retractDistance / rapidRatePerSec;
      estimate.rapidTime += retractTime;
      estimate.rapidDistance += retractDistance;
    }
  }

  // Calculate totals
  estimate.totalDistance = estimate.rapidDistance + estimate.cuttingDistance;
  estimate.totalTime = estimate.rapidTime + estimate.cuttingTime;

  return estimate;
}

bool GCodeGenerator::validatePaths(const std::vector<Path> &paths,
                                   std::vector<std::string> &warnings) const {
  warnings.clear();

  // Get the selected tool
  const Tool *tool = m_toolRegistry.getTool(m_options.selectedToolId);
  if (!tool) {
    warnings.push_back("No tool selected or tool not found in registry");
    return false;
  }

  // Validate each path
  return ToolOffset::validateToolForPaths(paths, tool->diameter, warnings);
}

std::vector<Path> GCodeGenerator::applyToolOffsets(
    const std::vector<Path> &paths) const {
  std::cout << "DEBUG: GCodeGenerator::applyToolOffsets() called with "
            << paths.size() << " paths" << std::endl;

  std::vector<Path> offsetPaths;
  offsetPaths.reserve(paths.size());

  // Get the selected tool
  const Tool *tool = m_toolRegistry.getTool(m_options.selectedToolId);
  if (!tool || tool->diameter <= 0) {
    std::cout << "DEBUG: No valid tool selected or invalid tool diameter"
              << std::endl;
    if (!tool) {
      std::cout << "  - Tool ID: " << m_options.selectedToolId
                << " not found in registry" << std::endl;
    } else {
      std::cout << "  - Tool diameter: " << tool->diameter << " (invalid)"
                << std::endl;
    }
    std::cout << "DEBUG: Returning original paths without offset" << std::endl;
    // No valid tool selected, return original paths
    return paths;
  }

  std::cout << "DEBUG: Using tool for offset calculation:" << std::endl;
  std::cout << "  - Tool ID: " << m_options.selectedToolId << std::endl;
  std::cout << "  - Tool diameter: " << tool->diameter << std::endl;
  std::cout << "  - Tool name: " << tool->name << std::endl;
  std::cout << "  - Offset direction: "
            << static_cast<int>(m_options.offsetDirection) << std::endl;

  // Apply offset to each path
  for (size_t pathIndex = 0; pathIndex < paths.size(); ++pathIndex) {
    const auto &path = paths[pathIndex];

    std::cout << "DEBUG: Processing path " << pathIndex << " of "
              << paths.size() << std::endl;

    if (path.empty()) {
      std::cout << "  - Path is empty, keeping original" << std::endl;
      offsetPaths.push_back(path);
      continue;
    }

    const auto &originalPoints = path.getPoints();
    std::cout << "  - Original path has " << originalPoints.size() << " points"
              << std::endl;

    // Print first few points of original path for reference
    std::cout << "  - First few original points:" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(3, originalPoints.size()); ++i) {
      std::cout << "    [" << i << "] (" << originalPoints[i].x << ", "
                << originalPoints[i].y << ")" << std::endl;
    }

    // Calculate offset path using new robust algorithm
    std::cout << "  - Calling new ToolOffset::calculateToolOffset for path "
              << pathIndex << std::endl;

    ToolOffset::OffsetOptions options;
    options.minFeatureSize = 0.01;  // 0.01mm minimum feature size
    options.validateResults = true;
    options.precision = 0.001;  // High precision

    auto offsetResult = ToolOffset::calculateToolOffset(
        path, tool->diameter, m_options.offsetDirection, options);

    Path offsetPath;
    if (offsetResult.success && !offsetResult.paths.empty()) {
      offsetPath = offsetResult.paths[0];  // Use first result path

      // Log detailed results
      std::cout << "  - Offset result: SUCCESS" << std::endl;
      std::cout << "    - Result paths: " << offsetResult.paths.size()
                << std::endl;
      std::cout << "    - Warnings: " << offsetResult.warnings.size()
                << std::endl;
      std::cout << "    - Errors: " << offsetResult.errors.size() << std::endl;
      std::cout << "    - Actual offset: " << offsetResult.actualOffsetDistance
                << "mm" << std::endl;

      for (const auto &warning : offsetResult.warnings) {
        std::cout << "    WARNING: " << warning << std::endl;
      }
    } else {
      std::cout << "  - Offset result: FAILED" << std::endl;
      for (const auto &error : offsetResult.errors) {
        std::cout << "    ERROR: " << error << std::endl;
      }
    }

    // If offset failed, use original path
    if (offsetPath.empty()) {
      std::cout << "  - Offset calculation failed for path " << pathIndex
                << ", using original path" << std::endl;
      offsetPaths.push_back(path);
    } else {
      const auto &offsetPoints = offsetPath.getPoints();
      std::cout << "  - Offset calculation successful for path " << pathIndex
                << std::endl;
      std::cout << "    - Offset path has " << offsetPoints.size() << " points"
                << std::endl;

      // Print first few points of offset path for comparison
      std::cout << "    - First few offset points:" << std::endl;
      for (size_t i = 0; i < std::min<size_t>(3, offsetPoints.size()); ++i) {
        std::cout << "      [" << i << "] (" << offsetPoints[i].x << ", "
                  << offsetPoints[i].y << ")" << std::endl;
      }

      // Use the actual offset from the ToolOffset result instead of manual
      // calculation
      double actualOffset = offsetResult.actualOffsetDistance;
      double expectedOffset = tool->diameter / 2.0;
      std::cout << "    - Actual offset distance: " << actualOffset
                << std::endl;
      std::cout << "    - Expected offset distance: " << expectedOffset
                << std::endl;

      // Safety check: if offset is way off, use original path instead
      double accuracyRatio = std::abs(actualOffset) / expectedOffset;
      if (accuracyRatio > 2.0 || accuracyRatio < 0.5) {
        std::cout << "    - Offset accuracy: FAILED (ratio: " << accuracyRatio
                  << ") - USING ORIGINAL PATH" << std::endl;
        offsetPaths.push_back(path);  // Use original path instead
      } else {
        if (std::abs(actualOffset - expectedOffset) < 0.01) {
          std::cout << "    - Offset accuracy: EXCELLENT" << std::endl;
        } else if (std::abs(actualOffset - expectedOffset) < 0.1) {
          std::cout << "    - Offset accuracy: GOOD" << std::endl;
        } else {
          std::cout << "    - Offset accuracy: ACCEPTABLE" << std::endl;
        }
        offsetPaths.push_back(offsetPath);  // Use offset path
      }
    }

    std::cout << "  - Path " << pathIndex << " processing complete"
              << std::endl;
  }

  std::cout << "DEBUG: GCodeGenerator::applyToolOffsets() completed"
            << std::endl;
  std::cout << "  - Input paths: " << paths.size() << std::endl;
  std::cout << "  - Output paths: " << offsetPaths.size() << std::endl;

  return offsetPaths;
}

std::vector<Polygon> GCodeGenerator::pathsToPolygons(
    const std::vector<Path> &paths) const {
  std::vector<Polygon> polygons;

  for (const auto &path : paths) {
    if (path.size() < 3) {
      continue;  // Need at least 3 points for a polygon
    }

    Polygon polygon;
    for (const auto &point : path.getPoints()) {
      polygon.addPoint(point);
    }

    // Ensure the polygon is closed
    if (!polygon.empty()) {
      const auto &points = polygon.getPoints();
      if (points.front().distanceTo(points.back()) > 1e-6) {
        polygon.addPoint(points.front());
      }
    }

    if (polygon.size() >= 3) {
      polygons.push_back(polygon);
    }
  }

  return polygons;
}

std::vector<Path> GCodeGenerator::generateAreaCuttingPaths(
    const std::vector<Polygon> &polygons) const {
  std::vector<Path> areaPaths;

  // Create cutout parameters from options
  CutoutParams cutoutParams;
  cutoutParams.mode = m_options.cutoutMode;
  cutoutParams.stepover = m_options.stepover;
  cutoutParams.overlap = m_options.overlap;
  cutoutParams.spiralIn = m_options.spiralIn;
  cutoutParams.maxStepover = m_options.maxStepover;

  // Get the selected tool
  const Tool *tool = m_toolRegistry.getTool(m_options.selectedToolId);
  if (!tool) {
    return areaPaths;  // Return empty if no tool selected
  }

  // Set up area cutter (const_cast needed for configuration)
  AreaCutter &areaCutter = const_cast<AreaCutter &>(m_areaCutter);
  areaCutter.setConfig(m_config);
  areaCutter.setToolRegistry(m_toolRegistry);

  // Convert polygons to paths
  std::vector<Path> inputPaths;
  for (const auto &polygon : polygons) {
    Path path(polygon.getPoints());
    inputPaths.push_back(path);
  }

  // Use the new area cutter API
  auto result = areaCutter.generateAreaCuts(inputPaths, cutoutParams,
                                            m_options.selectedToolId);

  // TODO: Handle warnings and errors from result
  if (result.success) {
    areaPaths = result.toolpaths;
  }

  return areaPaths;
}

}  // namespace cnc
}  // namespace nwss