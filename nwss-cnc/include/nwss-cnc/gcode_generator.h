#ifndef NWSS_CNC_GCODE_GENERATOR_H
#define NWSS_CNC_GCODE_GENERATOR_H

#include "nwss-cnc/geometry.h"
#include "nwss-cnc/config.h"
#include <string>
#include <vector>

namespace nwss {
namespace cnc {

/**
 * Structure to hold additional G-code generation options
 */
struct GCodeOptions {
    // Basic options
    std::string comments;         // Additional comments to include in the header
    bool useInches;               // Override units to inches (false = mm)
    bool includeHeader;           // Whether to include a descriptive header
    bool returnToOrigin;          // Whether to return to origin at the end
    
    // Path processing options
    bool optimizePaths;           // Optimize path ordering to minimize travel
    bool closeLoops;              // Ensure paths that should be closed are closed
    bool separateRetract;         // Add a retract between each path
    bool linearizePaths;     // Combine consecutive points that form straight lines
    double linearizeTolerance; // Maximum deviation allowed for linearization
    
    // Tool options
    double toolDiameter;          // Tool diameter for offsetting (0 = no offset)
    
    // Constructor with default values
    GCodeOptions() : 
        comments(""),
        useInches(false),
        includeHeader(true),
        returnToOrigin(true),
        optimizePaths(false),
        closeLoops(false),
        separateRetract(true),
        linearizePaths(true),     // Enable linearization by default
        linearizeTolerance(0.01), // Default tolerance (adjust as needed)
        toolDiameter(0.0)
    {}
};

/**
 * Class for generating G-code from path data
 */
class GCodeGenerator {
public:
    GCodeGenerator();
    ~GCodeGenerator();
    
    /**
     * Set the CNC configuration
     * @param config The machine and cutting configuration
     */
    void setConfig(const CNConfig& config);
    
    /**
     * Set additional G-code generation options
     * @param options The G-code options
     */
    void setOptions(const GCodeOptions& options);
    
    /**
     * Generate G-code from a set of paths
     * @param paths The discretized paths to convert to G-code
     * @param outputFile Path to the output G-code file
     * @return True if G-code was successfully generated
     */
    bool generateGCode(const std::vector<Path>& paths, const std::string& outputFile) const;
    
    /**
     * Generate G-code as a string without writing to a file
     * @param paths The discretized paths to convert to G-code
     * @return A string containing the generated G-code
     */
    std::string generateGCodeString(const std::vector<Path>& paths) const;
    
private:
    CNConfig m_config;            // CNC machine configuration
    GCodeOptions m_options;       // G-code generation options
    
    /**
     * Generate the G-code header
     * @param out The output stream
     */
    void writeHeader(std::ostream& out) const;
    
    /**
     * Generate the G-code footer
     * @param out The output stream
     */
    void writeFooter(std::ostream& out) const;
    
    /**
     * Generate G-code for a single path
     * @param out The output stream
     * @param path The path to process
     * @param pathIndex The index of the path
     */
    void writePath(std::ostream& out, const Path& path, size_t pathIndex) const;

    /**
     * Linearize a path to reduce the number of points
     * @param out The output stream
     * @param points The points to linearize
     * @param feedRate The feed rate for the path
     */
     void linearizePath(std::ostream& out, const std::vector<Point2D>& points, double feedRate) const;

     /**
      * Check if three points are collinear
      * @param p1 First point
      * @param p2 Second point
      * @param p3 Third point
      * @return True if the points are collinear, false otherwise
      */
     bool isCollinear(const Point2D& p1, const Point2D& p2, const Point2D& p3) const;
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_GCODE_GENERATOR_H