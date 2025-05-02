// svgtogcode.cpp
#include "svgtogcode.h"
#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>

// Include necessary library headers
#include "core/svg_parser.h"
#include "core/discretizer.h"
#include "core/config.h"
#include "core/transform.h"
#include "core/gcode_generator.h"

SvgToGCode::SvgToGCode(QObject *parent)
    : QObject(parent)
{
}

SvgToGCode::~SvgToGCode()
{
}

QString SvgToGCode::convertSvgToGCode(
    const QString &svgFilePath,
    int bezierSamples,
    double simplifyTolerance,
    double adaptiveSampling,
    double maxPointDistance,
    double bedWidth,
    double bedHeight,
    std::string units,
    double materialWidth,
    double materialHeight,
    double materialThickness,
    double feedRate,
    double plungeRate,
    double spindleSpeed,
    double passDepth,
    int passCount,
    double safetyHeight,
    bool preserveAspectRatio,
    bool centerDesign,
    bool flipY,
    bool optimizePaths,
    bool closeLoops,
    bool separateRetract,
    bool linearizePaths,
    double linearizeTolerance,
    double toolDiameter) 
{
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
    
    // Step 2: Get dimensions
    stepTimer.restart();
    float width, height;
    if (parser.getDimensions(width, height)) {
        qDebug() << "SVG Dimensions: " << width << " x " << height << " mm";
    } else {
        m_lastError = "Failed to get SVG dimensions.";
        return QString();
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
    std::vector<nwss::cnc::Path> allPaths = discretizer.discretizeImage(parser.getRawImage());
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
    if (nwss::cnc::Transform::fitToMaterial(allPaths, CNconfig, preserveAspectRatio, 
                                centerDesign, centerDesign, flipY, &transformInfo)) {
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
    gCodeOptions.toolDiameter = toolDiameter;
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
    qDebug() << "Total SVG to GCode conversion took" << totalTimer.elapsed() << "ms";
    
    return gCodeString;
}