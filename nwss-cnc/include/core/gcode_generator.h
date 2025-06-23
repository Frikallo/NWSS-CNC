#ifndef NWSS_CNC_GCODE_GENERATOR_H
#define NWSS_CNC_GCODE_GENERATOR_H

#include "core/geometry.h"
#include "core/config.h"
#include "core/tool.h"
#include "core/area_cutter.h"
#include <string>
#include <vector>

namespace nwss {
namespace cnc {

/**
 * Structure to hold additional G-code generation options
 */
struct GCodeOptions {
    // Basic options
    bool includeComments = false;
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
    int selectedToolId;           // ID of the selected tool from registry
    ToolOffsetDirection offsetDirection; // Tool offset direction
    bool enableToolOffsets;       // Whether to apply tool offsets
    bool validateFeatureSizes;    // Whether to validate feature sizes against tool
    std::string materialType;     // Material type for optimized cutting parameters
    
    // Cutout mode options
    CutoutMode cutoutMode;        // The type of cutting operation to perform
    double stepover;              // Stepover distance for area cutting (as fraction of tool diameter)
    double overlap;               // Overlap between passes (as fraction of stepover)
    bool spiralIn;                // Whether to spiral inward for pocketing
    double maxStepover;           // Maximum stepover in absolute units (mm)
    
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
        selectedToolId(0),
        offsetDirection(ToolOffsetDirection::AUTO),
        enableToolOffsets(true),
        validateFeatureSizes(true),
        materialType("Unknown"),
        cutoutMode(CutoutMode::PERIMETER),
        stepover(0.5),
        overlap(0.1),
        spiralIn(true),
        maxStepover(2.0)
    {}
};

/**
 * Class for generating G-code from path data
 */
class GCodeGenerator {
public:
    GCodeGenerator();
    ~GCodeGenerator();

    struct TimeEstimate {
        double rapidTime;     // Time for rapid moves (seconds)
        double cuttingTime;   // Time for cutting moves (seconds)
        double totalTime;     // Total time (seconds)
        double totalDistance; // Total travel distance (in configured units)
        double rapidDistance; // Distance of rapid moves
        double cuttingDistance; // Distance of cutting moves
    };
    
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
     * Set the tool registry
     * @param registry The tool registry to use
     */
    void setToolRegistry(const ToolRegistry& registry);
    
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

    /**
     * Calculate time estimates for the given paths
     * @param paths The discretized paths to estimate time for
     * @return A TimeEstimate structure with the calculated times
     */
     TimeEstimate calculateTimeEstimate(const std::vector<Path>& paths) const;
     
     /**
      * Validate paths against the selected tool
      * @param paths The paths to validate
      * @param warnings Output vector for warning messages
      * @return True if all paths can be machined with the selected tool
      */
     bool validatePaths(const std::vector<Path>& paths, std::vector<std::string>& warnings) const;
    
private:
    CNConfig m_config;            // CNC machine configuration
    GCodeOptions m_options;       // G-code generation options
    ToolRegistry m_toolRegistry;  // Tool registry
    AreaCutter m_areaCutter;      // Area cutting operations
    
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
     
     /**
      * Apply tool offset to paths if enabled
      * @param paths The input paths
      * @return Vector of offset paths
      */
     std::vector<Path> applyToolOffsets(const std::vector<Path>& paths) const;
     
     /**
      * Convert paths to polygons for area operations
      * @param paths The input paths
      * @return Vector of polygons
      */
     std::vector<Polygon> pathsToPolygons(const std::vector<Path>& paths) const;
     
     /**
      * Generate area cutting paths based on cutout mode
      * @param polygons The input polygons
      * @return Vector of area cutting paths
      */
     std::vector<Path> generateAreaCuttingPaths(const std::vector<Polygon>& polygons) const;
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_GCODE_GENERATOR_H