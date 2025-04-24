// svgtogcode.cpp
#include "svgtogcode.h"
#include <QSvgRenderer>
#include <QPainterPath>
#include <QFile>
#include <QDateTime>

SvgToGCode::SvgToGCode(QObject *parent)
    : QObject(parent)
{
}

QString SvgToGCode::convertSvgToGCode(
    const QString &svgFilePath,
    ConversionMode mode,
    double toolDiameter,
    double feedRate,
    double plungeRate,
    double passDepth,
    double totalDepth,
    double safetyHeight)
{
    m_lastError.clear();
    
    // Check if the file exists
    QFile file(svgFilePath);
    if (!file.exists()) {
        m_lastError = "SVG file does not exist: " + svgFilePath;
        return QString();
    }
    
    // Generate GCode based on the selected mode
    QString gcode;
    
    // Add common header
    gcode += generateHeader(toolDiameter, feedRate, safetyHeight);
    
    // Generate GCode based on selected mode
    switch (mode) {
    case Outline:
        gcode += generateOutlineGCode(svgFilePath);
        break;
    case Pocket:
        gcode += generatePocketGCode(svgFilePath);
        break;
    case Engrave:
        gcode += generateEngraveGCode(svgFilePath);
        break;
    case VCarve:
        gcode += generateVCarveGCode(svgFilePath);
        break;
    }
    
    // Add common footer
    gcode += generateFooter();
    
    // Placeholder for actual implementation
    if (gcode.isEmpty()) {
        gcode = "; This is a placeholder GCode generated from " + svgFilePath + "\n";
        gcode += "; A real implementation would convert SVG paths to actual toolpaths\n\n";
        gcode += "G21 ; Set units to mm\n";
        gcode += "G90 ; Set to absolute positioning\n";
        gcode += "G17 ; XY plane selection\n\n";
        gcode += "G0 Z" + QString::number(safetyHeight) + " ; Move to safety height\n";
        gcode += "G0 X0 Y0 ; Move to origin\n\n";
        gcode += "M3 S" + QString::number(int(12000)) + " ; Start spindle\n";
        gcode += "G4 P2 ; Wait 2 seconds for spindle to spin up\n\n";
        
        // Generate a sample toolpath (a square and circle) for demonstration
        gcode += "; Example toolpath (not based on actual SVG)\n";
        gcode += "G0 X10 Y10 ; Move to start position\n";
        gcode += "G1 Z0 F" + QString::number(plungeRate) + " ; Plunge to surface\n\n";
        
        // Create multiple passes if needed
        int passes = qCeil(totalDepth / passDepth);
        for (int pass = 0; pass < passes; pass++) {
            double depth = -std::min((pass + 1) * passDepth, totalDepth);
            
            gcode += "; Pass " + QString::number(pass + 1) + " - Depth: " + QString::number(depth) + "mm\n";
            gcode += "G1 Z" + QString::number(depth) + " F" + QString::number(plungeRate) + " ; Plunge to pass depth\n";
            gcode += "G1 X50 Y10 F" + QString::number(feedRate) + " ; Cut line\n";
            gcode += "G1 X50 Y50 ; Cut line\n";
            gcode += "G1 X10 Y50 ; Cut line\n";
            gcode += "G1 X10 Y10 ; Cut line (return to start)\n\n";
            
            // Add a circle in the middle
            gcode += "; Circle in the middle\n";
            gcode += "G0 X30 Y30 ; Move to center of square\n";
            
            // Generate a circle with G2 command
            gcode += "G1 X40 Y30 ; Move to edge of circle\n";
            gcode += "G2 X30 Y40 I-10 J0 ; Arc segment\n";
            gcode += "G2 X20 Y30 I0 J-10 ; Arc segment\n";
            gcode += "G2 X30 Y20 I10 J0 ; Arc segment\n";
            gcode += "G2 X40 Y30 I0 J10 ; Arc segment (complete circle)\n\n";
            
            // Return to safe Z height between passes
            gcode += "G0 Z" + QString::number(safetyHeight) + " ; Return to safe height\n\n";
        }
        
        gcode += "M5 ; Stop spindle\n";
        gcode += "G0 Z" + QString::number(safetyHeight) + " ; Return to safe height\n";
        gcode += "G0 X0 Y0 ; Return to origin\n";
        gcode += "M2 ; End program\n";
    }
    
    return gcode;
}

QString SvgToGCode::generateHeader(double toolDiameter, double feedRate, double safetyHeight)
{
    QString header;
    header += "; GCode generated from SVG\n";
    header += "; Generated on: " + QDateTime::currentDateTime().toString() + "\n";
    header += "; Tool diameter: " + QString::number(toolDiameter) + "mm\n";
    header += "; Feed rate: " + QString::number(feedRate) + "mm/min\n";
    header += "; Safety height: " + QString::number(safetyHeight) + "mm\n\n";
    
    return header;
}

QString SvgToGCode::generateFooter()
{
    QString footer;
    footer += "\n; End of generated GCode\n";
    return footer;
}

QString SvgToGCode::generateOutlineGCode(const QString &svgPath)
{
    // Placeholder - in a real implementation, this would trace the SVG outlines
    return QString();
}

QString SvgToGCode::generatePocketGCode(const QString &svgPath)
{
    // Placeholder - in a real implementation, this would generate pocket clearing operations
    return QString();
}

QString SvgToGCode::generateEngraveGCode(const QString &svgPath)
{
    // Placeholder - in a real implementation, this would generate engraving toolpaths
    return QString();
}

QString SvgToGCode::generateVCarveGCode(const QString &svgPath)
{
    // Placeholder - in a real implementation, this would generate V-carving toolpaths
    return QString();
}