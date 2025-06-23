#ifndef NWSS_CNC_AREA_CUTTER_H
#define NWSS_CNC_AREA_CUTTER_H

#include <memory>
#include <string>
#include <vector>

#include "core/config.h"
#include "core/geometry.h"
#include "core/tool.h"

namespace nwss {
namespace cnc {

// Forward declaration
class CAMProcessor;

/**
 * Result structure for area cutting operations
 */
struct AreaCutterResult {
  std::vector<Path> toolpaths;
  bool success = false;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;
  double estimatedTime = 0.0;
  double totalDistance = 0.0;

  bool hasWarnings() const { return !warnings.empty(); }
  bool hasErrors() const { return !errors.empty(); }
  bool isValid() const { return success && !hasErrors(); }
};

/**
 * Area cutting functionality using professional CAM algorithms
 * This class serves as a wrapper around CAMProcessor for area-based operations
 */
class AreaCutter {
 public:
  AreaCutter();
  ~AreaCutter();

  /**
   * Set the CNC configuration
   */
  void setConfig(const CNConfig &config);

  /**
   * Set the tool registry
   */
  void setToolRegistry(const ToolRegistry &registry);

  /**
   * Generate area cutting toolpaths
   * @param paths Input paths from SVG
   * @param params Cutting parameters
   * @param selectedToolId ID of the selected tool
   * @return Area cutting result with toolpaths and validation info
   */
  AreaCutterResult generateAreaCuts(const std::vector<Path> &paths,
                                    const CutoutParams &params,
                                    int selectedToolId);

  /**
   * Generate contour toolpaths for a polygon
   * @param polygon Input polygon
   * @param toolDiameter Tool diameter
   * @param stepover Stepover distance
   * @return Vector of contour paths
   */
  std::vector<Path> generateContourPaths(const Polygon &polygon,
                                         double toolDiameter, double stepover);

  /**
   * Generate spiral toolpaths for a polygon
   * @param polygon Input polygon
   * @param toolDiameter Tool diameter
   * @param stepover Stepover distance
   * @param inward Whether to spiral inward
   * @return Vector of spiral paths
   */
  std::vector<Path> generateSpiralPaths(const Polygon &polygon,
                                        double toolDiameter, double stepover,
                                        bool inward = true);

  /**
   * Generate raster toolpaths for a polygon
   * @param polygon Input polygon
   * @param toolDiameter Tool diameter
   * @param stepover Stepover distance
   * @param angle Raster angle in degrees
   * @return Vector of raster paths
   */
  std::vector<Path> generateRasterPaths(const Polygon &polygon,
                                        double toolDiameter, double stepover,
                                        double angle = 0.0);

  /**
   * Validate cutting parameters for a polygon
   * @param polygon Input polygon
   * @param toolDiameter Tool diameter
   * @param mode Cutting mode
   * @return True if parameters are valid
   */
  bool validateCutParameters(const Polygon &polygon, double toolDiameter,
                             CutoutMode mode);

 private:
  CNConfig m_config;
  ToolRegistry m_toolRegistry;
  std::unique_ptr<CAMProcessor> m_camProcessor;
};

}  // namespace cnc
}  // namespace nwss

#endif  // NWSS_CNC_AREA_CUTTER_H