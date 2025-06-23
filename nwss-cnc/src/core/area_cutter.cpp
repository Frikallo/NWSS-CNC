#include "core/area_cutter.h"

#include "core/cam_processor.h"

namespace nwss {
namespace cnc {

AreaCutter::AreaCutter() : m_camProcessor(std::make_unique<CAMProcessor>()) {}

AreaCutter::~AreaCutter() = default;

void AreaCutter::setConfig(const CNConfig &config) {
  m_config = config;
  m_camProcessor->setConfig(config);
}

void AreaCutter::setToolRegistry(const ToolRegistry &registry) {
  m_toolRegistry = registry;
  m_camProcessor->setToolRegistry(registry);
}

AreaCutterResult AreaCutter::generateAreaCuts(const std::vector<Path> &paths,
                                              const CutoutParams &params,
                                              int selectedToolId) {
  AreaCutterResult result;

  // Use the professional CAM processor
  auto camResult = m_camProcessor->processForCAM(paths, params, selectedToolId);

  // Convert CAM result to AreaCutter result
  result.success = camResult.success;
  result.toolpaths = camResult.toolpaths;
  result.warnings = camResult.warnings;
  result.errors = camResult.errors;
  result.estimatedTime = camResult.estimatedMachiningTime;
  result.totalDistance = camResult.totalCuttingDistance;

  return result;
}

std::vector<Path> AreaCutter::generateContourPaths(const Polygon &polygon,
                                                   double toolDiameter,
                                                   double stepover) {
  // Use CAM processor for professional contour generation
  return m_camProcessor->generateContourToolpath(polygon, toolDiameter,
                                                 stepover);
}

std::vector<Path> AreaCutter::generateSpiralPaths(const Polygon &polygon,
                                                  double toolDiameter,
                                                  double stepover,
                                                  bool inward) {
  // Use CAM processor for professional spiral generation
  return m_camProcessor->generateSpiralToolpath(polygon, toolDiameter, stepover,
                                                inward);
}

std::vector<Path> AreaCutter::generateRasterPaths(const Polygon &polygon,
                                                  double toolDiameter,
                                                  double stepover,
                                                  double angle) {
  // Use CAM processor for professional raster generation
  return m_camProcessor->generateRasterToolpath(polygon, toolDiameter, stepover,
                                                angle);
}

bool AreaCutter::validateCutParameters(const Polygon &polygon,
                                       double toolDiameter, CutoutMode mode) {
  // Use CAM processor for professional validation
  auto validationResult =
      m_camProcessor->validateToolpathFeasibility(polygon, toolDiameter, mode);
  return validationResult.success;
}

}  // namespace cnc
}  // namespace nwss