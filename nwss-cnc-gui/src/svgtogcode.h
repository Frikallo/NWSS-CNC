// svgtogcode.h
#ifndef SVGTOGCODE_H
#define SVGTOGCODE_H

#include <QString>
#include <QObject>

// Forward declarations for nwss-cnc library types
namespace nwss {
namespace cnc {
    class SVGParser;
    class Discretizer;
    class GCodeGenerator;
    struct DiscretizerConfig;
    class Path;
}
}

class SvgToGCode : public QObject
{
    Q_OBJECT

public:
    SvgToGCode(QObject *parent = nullptr);
    ~SvgToGCode();
    
    // Convert SVG file to GCode with given parameters
    QString convertSvgToGCode(
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
        double toolDiameter);
    
    // Get any error that occurred during conversion
    QString lastError() const { return m_lastError; }
    
private:
    QString m_lastError;
};

#endif // SVGTOGCODE_H