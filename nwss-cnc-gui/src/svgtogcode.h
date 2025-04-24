// svgtogcode.h
#ifndef SVGTOGCODE_H
#define SVGTOGCODE_H

#include <QString>
#include <QObject>
#include <QMap>

class SvgToGCode : public QObject
{
    Q_OBJECT

public:
    enum ConversionMode {
        Outline,    // 2D outline cut along SVG paths
        Pocket,     // 2D pocket/area clearing inside SVG paths
        Engrave,    // 2D engraving along SVG paths
        VCarve      // 3D V-carving based on SVG paths
    };
    
    SvgToGCode(QObject *parent = nullptr);
    
    // Convert SVG file to GCode with given parameters
    QString convertSvgToGCode(
        const QString &svgFilePath,
        ConversionMode mode,
        double toolDiameter,
        double feedRate,
        double plungeRate,
        double passDepth,
        double totalDepth,
        double safetyHeight
    );
    
    // Get any error that occurred during conversion
    QString lastError() const { return m_lastError; }
    
private:
    QString m_lastError;
    
    // Helper methods for different conversion strategies
    QString generateOutlineGCode(const QString &svgPath);
    QString generatePocketGCode(const QString &svgPath);
    QString generateEngraveGCode(const QString &svgPath);
    QString generateVCarveGCode(const QString &svgPath);
    
    // Common GCode generation helpers
    QString generateHeader(double toolDiameter, double feedRate, double safetyHeight);
    QString generateFooter();
};

#endif // SVGTOGCODE_H