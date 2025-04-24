// svgtogcode.cpp
#include "svgtogcode.h"
#include <QFile>
#include <QDateTime>
#include <QFileInfo>

// Include necessary library headers
#include "nwss-cnc/svg_parser.h"
#include "nwss-cnc/discretizer.h"
#include "nwss-cnc/config.h"
#include "nwss-cnc/transform.h"
#include "nwss-cnc/gcode_generator.h"

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
    m_lastError.clear();
    
    // Check if the file exists
    QFile file(svgFilePath);
    if (!file.exists()) {
        m_lastError = "SVG file does not exist: " + svgFilePath;
        return QString();
    }
    
    nwss::cnc::SVGParser parser;
    if (!parser.loadFromFile(svgFilePath.toStdString(), "mm", 96.0f)) {
        m_lastError = "Failed to load SVG file: " + svgFilePath;
        return QString();
    }

    float width, height;
    if (parser.getDimensions(width, height)) {
        qDebug() << "SVG Dimensions: " << width << " x " << height << " mm";
    } else {
        m_lastError = "Failed to get SVG dimensions.";
        return QString();
    }

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
    CNconfig.setMaterialWidth(bedWidth);
    CNconfig.setMaterialHeight(bedHeight);
    CNconfig.setMaterialThickness(materialThickness);
    CNconfig.setFeedRate(feedRate);
    CNconfig.setPlungeRate(plungeRate);
    CNconfig.setSpindleSpeed(spindleSpeed);
    CNconfig.setCutDepth(passDepth);
    CNconfig.setPassCount(passCount);
    CNconfig.setSafeHeight(safetyHeight);

    std::vector<nwss::cnc::Path> allPaths = discretizer.discretizeImage(parser.getRawImage());

    double minX, minY, maxX, maxY;
    if (nwss::cnc::Transform::getBounds(allPaths, minX, minY, maxX, maxY)) {
        double width = maxX - minX;
        double height = maxY - minY;
    }

    nwss::cnc::TransformInfo transformInfo;
    if (nwss::cnc::Transform::fitToMaterial(allPaths, CNconfig, preserveAspectRatio, 
                                centerDesign, centerDesign, flipY, &transformInfo)) {
        qDebug() << "Transform Info: " << nwss::cnc::Transform::formatTransformInfo(transformInfo, CNconfig);
    } else {
        m_lastError = "Failed to fit paths to material.";
        return QString();
    }

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
    return gCodeString;
}