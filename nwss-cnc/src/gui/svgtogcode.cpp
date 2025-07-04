// svgtogcode.cpp
#include "svgtogcode.h"

#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>

// Include necessary library headers
#include "core/config.h"
#include "core/discretizer.h"
#include "core/gcode_generator.h"
#include "core/svg_parser.h"
#include "core/transform.h"

SvgToGCode::SvgToGCode(QObject *parent) : QObject(parent) {}

SvgToGCode::~SvgToGCode() {}

QString SvgToGCode::convertSvgToGCode(
    const QString &svgFilePath, int bezierSamples, double simplifyTolerance,
    double adaptiveSampling, double maxPointDistance, double bedWidth,
    double bedHeight, std::string units, double materialWidth,
    double materialHeight, double materialThickness, double feedRate,
    double plungeRate, double spindleSpeed, double passDepth, int passCount,
    double safetyHeight, bool preserveAspectRatio, bool centerDesign,
    bool flipY, bool optimizePaths, bool closeLoops, bool separateRetract,
    bool linearizePaths, double linearizeTolerance, double toolDiameter,
    int cutoutMode, double stepover, double maxStepover, bool spiralIn) {
  QElapsedTimer totalTimer;
  totalTimer.start();

  m_lastError.clear();
  QElapsedTimer stepTimer;

  // Step 1: Load SVG file
  stepTimer.start();
  nwss::cnc::SVGParser parser;
  if (!parser.loadFromFile(svgFilePath.toStdString(), "mm", 96.0f)) {
    m_lastError = "Failed to load SVG file: " + svgFilePath;
    return QString();
  }
  qDebug() << "Step 1: SVG loading took" << stepTimer.elapsed() << "ms";

  // Step 2: Get dimensions and detect content bounds
  stepTimer.restart();
  float width, height;
  if (parser.getDimensions(width, height)) {
    qDebug() << "Original SVG Dimensions: " << width << " x " << height
             << " mm";
  } else {
    m_lastError = "Failed to get SVG dimensions.";
    return QString();
  }

  // Get content bounds to automatically remove margins
  nwss::cnc::SVGContentBounds contentBounds = parser.getContentBounds();
  if (!contentBounds.isEmpty) {
    qDebug() << "Content Bounds (margins removed): " << contentBounds.width
             << " x " << contentBounds.height << " mm";
    qDebug() << "Margin removal: Original(" << width << "x" << height
             << ") -> Content(" << contentBounds.width << "x"
             << contentBounds.height << ")";
  } else {
    qDebug() << "No content bounds detected, using original dimensions";
  }
  qDebug() << "Step 2: Get dimensions took" << stepTimer.elapsed() << "ms";

  // Step 3: Configure discretizer
  stepTimer.restart();
  nwss::cnc::Discretizer discretizer;
  nwss::cnc::DiscretizerConfig discretizerConfig;
  discretizerConfig.bezierSamples = bezierSamples;
  discretizerConfig.simplifyTolerance = simplifyTolerance;
  discretizerConfig.adaptiveSampling = adaptiveSampling;
  discretizerConfig.maxPointDistance = maxPointDistance;
  discretizer.setConfig(discretizerConfig);

  nwss::cnc::CNConfig CNconfig;
  CNconfig.setBedWidth(bedWidth);
  CNconfig.setBedHeight(bedHeight);
  CNconfig.setUnitsFromString(units);
  CNconfig.setMaterialWidth(materialWidth);
  CNconfig.setMaterialHeight(materialHeight);
  CNconfig.setMaterialThickness(materialThickness);
  CNconfig.setFeedRate(feedRate);
  CNconfig.setPlungeRate(plungeRate);
  CNconfig.setSpindleSpeed(spindleSpeed);
  CNconfig.setCutDepth(passDepth);
  CNconfig.setPassCount(passCount);
  CNconfig.setSafeHeight(safetyHeight);
  qDebug() << "Step 3: Configuration took" << stepTimer.elapsed() << "ms";

  // Step 4: Discretize SVG paths
  stepTimer.restart();
  std::vector<nwss::cnc::Path> allPaths =
      discretizer.discretizeImage(parser.getRawImage());
  qDebug() << "Step 4: Path discretization took" << stepTimer.elapsed() << "ms"
           << "(" << allPaths.size() << " paths)";

  // Step 5: Get bounds for diagnostics
  stepTimer.restart();
  double minX, minY, maxX, maxY;
  if (nwss::cnc::Transform::getBounds(allPaths, minX, minY, maxX, maxY)) {
    double width = maxX - minX;
    double height = maxY - minY;
    qDebug() << "Paths bounds:" << minX << minY << maxX << maxY;
  }
  qDebug() << "Step 5: Get bounds took" << stepTimer.elapsed() << "ms";

  // Step 6: Transform paths to fit material
  stepTimer.restart();
  nwss::cnc::TransformInfo transformInfo;
  if (nwss::cnc::Transform::fitToMaterial(
          allPaths, CNconfig, preserveAspectRatio, centerDesign, centerDesign,
          flipY, &transformInfo)) {
    qDebug() << "Transform Info: " << transformInfo.message.c_str();
  } else {
    m_lastError = "Failed to fit paths to material.";
    return QString();
  }
  qDebug() << "Step 6: Transform paths took" << stepTimer.elapsed() << "ms";

  // Step 7: Generate GCode
  stepTimer.restart();
  nwss::cnc::GCodeGenerator gCodeGen;
  nwss::cnc::GCodeOptions gCodeOptions;
  gCodeOptions.optimizePaths = optimizePaths;
  gCodeOptions.closeLoops = closeLoops;
  gCodeOptions.separateRetract = separateRetract;
  gCodeOptions.linearizePaths = linearizePaths;
  gCodeOptions.linearizeTolerance = linearizeTolerance;
  // Note: Tool selection should be handled by the GUI, for now disable tool
  // offsets
  gCodeOptions.enableToolOffsets = false;
  gCodeOptions.validateFeatureSizes = false;

  // Set cutout mode parameters
  gCodeOptions.cutoutMode = static_cast<nwss::cnc::CutoutMode>(cutoutMode);
  gCodeOptions.stepover = stepover;
  gCodeOptions.maxStepover = maxStepover;
  gCodeOptions.spiralIn = spiralIn;

  gCodeGen.setConfig(CNconfig);
  gCodeGen.setOptions(gCodeOptions);

  std::string gCode = gCodeGen.generateGCodeString(allPaths);
  if (gCode.empty()) {
    m_lastError = "Failed to generate GCode.";
    return QString();
  }
  QString gCodeString = QString::fromStdString(gCode);
  qDebug() << "Step 7: GCode generation took" << stepTimer.elapsed() << "ms"
           << "(GCode size:" << gCodeString.size() << "bytes)";

  // Total time
  qDebug() << "Total SVG to GCode conversion took" << totalTimer.elapsed()
           << "ms";

  // Calculate and store time estimate
  m_timeEstimate.rapidTime = 0;
  m_timeEstimate.cuttingTime = 0;
  m_timeEstimate.totalTime = 0;
  m_timeEstimate.totalDistance = 0;
  m_timeEstimate.rapidDistance = 0;
  m_timeEstimate.cuttingDistance = 0;

  // Calculate time estimate using the GCodeGenerator
  nwss::cnc::GCodeGenerator::TimeEstimate estimate =
      gCodeGen.calculateTimeEstimate(allPaths);

  // Convert to our struct format
  m_timeEstimate.rapidTime = estimate.rapidTime;
  m_timeEstimate.cuttingTime = estimate.cuttingTime;
  m_timeEstimate.totalTime = estimate.totalTime;
  m_timeEstimate.totalDistance = estimate.totalDistance;
  m_timeEstimate.rapidDistance = estimate.rapidDistance;
  m_timeEstimate.cuttingDistance = estimate.cuttingDistance;

  qDebug() << "Time estimate: " << m_timeEstimate.totalTime << " seconds ("
           << m_timeEstimate.totalTime / 60.0 << " minutes)";

  return gCodeString;
}