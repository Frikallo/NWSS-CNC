#ifndef GCODE_GENERATOR_H
#define GCODE_GENERATOR_H

#include "types.h"
#include <fstream>
#include <sstream>

class GCodeGenerator {
public:
    GCodeGenerator();
    ~GCodeGenerator();
    
    // Generate G-code from toolpath
    bool generateGCode(const std::vector<ToolpathPoint>& toolpath, 
                      const MachiningParams& params,
                      const std::string& outputFilename);
    
    // Generate G-code header
    std::string generateHeader(const MachiningParams& params) const;
    
    // Generate G-code footer
    std::string generateFooter(const MachiningParams& params) const;
    
    // Convert toolpath point to G-code command
    std::string pointToGCode(const ToolpathPoint& point, const ToolpathPoint& previousPoint) const;
    
    // Add comment to G-code
    std::string addComment(const std::string& comment) const;
    
    // Format coordinate value
    std::string formatCoordinate(double value, char axis) const;
    
    // Generate tool change commands
    std::string generateToolChange(int toolNumber, const Tool& tool) const;
    
    // Generate spindle commands
    std::string generateSpindleOn(double speed) const;
    std::string generateSpindleOff() const;
    
    // Generate coolant commands
    std::string generateCoolantOn() const;
    std::string generateCoolantOff() const;

private:
    // Current machine state
    struct MachineState {
        Point3D currentPosition;
        double currentFeedrate;
        bool spindleOn;
        bool coolantOn;
        bool isRapidMode;
        
        MachineState() : currentPosition(0, 0, 0), currentFeedrate(0), 
                        spindleOn(false), coolantOn(false), isRapidMode(true) {}
    };
    
    mutable MachineState m_state;
    
    // G-code formatting options
    int m_decimalPlaces;
    bool m_useIncrementalMode;
    bool m_suppressZeroCoordinates;
    
    // Helper functions
    bool needsCoordinateOutput(const Point3D& newPos, const Point3D& currentPos) const;
    bool needsFeedrateOutput(double newFeedrate, double currentFeedrate) const;
    std::string formatNumber(double value) const;
    
    // Safety checks
    bool validateMove(const ToolpathPoint& from, const ToolpathPoint& to) const;
    bool isValidCoordinate(double value) const;
};

#endif // GCODE_GENERATOR_H