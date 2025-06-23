#ifndef NWSS_CNC_CAM_PROCESSOR_H
#define NWSS_CNC_CAM_PROCESSOR_H

#include "core/geometry.h"
#include "core/config.h"
#include "core/tool.h"
#include <clipper2/clipper.h>
#include <vector>
#include <string>
#include <memory>

namespace nwss {
namespace cnc {

/**
 * Represents a CAM operation result with validation information
 */
struct CAMOperationResult {
    std::vector<Path> toolpaths;
    bool success = false;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    double estimatedMachiningTime = 0.0;
    double totalCuttingDistance = 0.0;
    
    bool hasWarnings() const { return !warnings.empty(); }
    bool hasErrors() const { return !errors.empty(); }
    bool isValid() const { return success && !hasErrors(); }
};

/**
 * Represents a polygon hierarchy for nested shape analysis
 */
struct PolygonHierarchy {
    Polygon polygon;
    std::vector<std::shared_ptr<PolygonHierarchy>> children;  // Inner polygons (holes)
    std::shared_ptr<PolygonHierarchy> parent;                 // Outer polygon
    int level = 0;                                            // Nesting level (0 = outermost)
    bool isHole = false;                                      // True if this is a hole
    
    PolygonHierarchy() = default;
    PolygonHierarchy(const Polygon& poly) : polygon(poly) {}
};

/**
 * Professional CAM processor using Clipper2 for robust 2D operations
 */
class CAMProcessor {
public:
    CAMProcessor();
    ~CAMProcessor();

    /**
     * Set the CNC configuration
     */
    void setConfig(const CNConfig& config);

    /**
     * Set the tool registry
     */
    void setToolRegistry(const ToolRegistry& registry);

    /**
     * Process paths for CAM operations with comprehensive validation
     * @param paths Input paths from SVG
     * @param cutoutParams Cutting parameters
     * @param selectedToolId ID of the selected tool
     * @return CAM operation result with toolpaths and validation info
     */
    CAMOperationResult processForCAM(const std::vector<Path>& paths, 
                                   const CutoutParams& cutoutParams,
                                   int selectedToolId);

    /**
     * Analyze polygon hierarchy for nested shapes
     * @param polygons Input polygons
     * @return Hierarchical structure of polygons
     */
    std::vector<std::shared_ptr<PolygonHierarchy>> analyzePolygonHierarchy(
        const std::vector<Polygon>& polygons);

    /**
     * Generate professional punchout toolpaths
     * @param hierarchy Polygon hierarchy
     * @param toolDiameter Tool diameter
     * @param stepover Stepover distance
     * @return CAM operation result
     */
    CAMOperationResult generatePunchoutToolpaths(
        const std::vector<std::shared_ptr<PolygonHierarchy>>& hierarchy,
        double toolDiameter, double stepover);

    /**
     * Generate professional pocket toolpaths
     * @param hierarchy Polygon hierarchy  
     * @param toolDiameter Tool diameter
     * @param stepover Stepover distance
     * @param spiralIn Whether to spiral inward
     * @return CAM operation result
     */
    CAMOperationResult generatePocketToolpaths(
        const std::vector<std::shared_ptr<PolygonHierarchy>>& hierarchy,
        double toolDiameter, double stepover, bool spiralIn);

    /**
     * Generate professional engrave toolpaths
     * @param hierarchy Polygon hierarchy
     * @param toolDiameter Tool diameter
     * @param stepover Stepover distance
     * @return CAM operation result
     */
    CAMOperationResult generateEngraveToolpaths(
        const std::vector<std::shared_ptr<PolygonHierarchy>>& hierarchy,
        double toolDiameter, double stepover);

    /**
     * Validate toolpath feasibility
     * @param polygon Input polygon
     * @param toolDiameter Tool diameter
     * @param cutoutMode Cutting mode
     * @return Validation result with warnings/errors
     */
    CAMOperationResult validateToolpathFeasibility(
        const Polygon& polygon, double toolDiameter, CutoutMode cutoutMode);

    // Professional toolpath generation algorithms (public for AreaCutter access)
    std::vector<Path> generateSpiralToolpath(const Polygon& polygon, 
                                           double toolDiameter, double stepover, 
                                           bool inward = true);
    std::vector<Path> generateParallelToolpath(const Polygon& polygon,
                                             double toolDiameter, double stepover,
                                             double angle = 0.0);
    std::vector<Path> generateContourToolpath(const Polygon& polygon,
                                            double toolDiameter, double stepover);
    std::vector<Path> generateRasterToolpath(const Polygon& polygon,
                                           double toolDiameter, double stepover,
                                           double angle = 0.0);

private:
    CNConfig m_config;
    ToolRegistry m_toolRegistry;

    // Clipper2 conversion utilities
    Clipper2Lib::Paths64 polygonToClipperPaths(const Polygon& polygon);
    Clipper2Lib::Paths64 polygonsToClipperPaths(const std::vector<Polygon>& polygons);
    std::vector<Polygon> clipperPathsToPolygons(const Clipper2Lib::Paths64& paths);
    Polygon clipperPathToPolygon(const Clipper2Lib::Path64& path);

    // Advanced polygon operations using Clipper2
    std::vector<Polygon> offsetPolygon(const Polygon& polygon, double offset);
    std::vector<Polygon> unionPolygons(const std::vector<Polygon>& polygons);
    std::vector<Polygon> intersectPolygons(const std::vector<Polygon>& polygons1,
                                         const std::vector<Polygon>& polygons2);
    std::vector<Polygon> differencePolygons(const std::vector<Polygon>& subject,
                                          const std::vector<Polygon>& clip);

    // Validation and analysis
    bool isPolygonTooSmallForTool(const Polygon& polygon, double toolDiameter);
    bool hasInvalidGeometry(const Polygon& polygon);
    double calculateMinimumFeatureSize(const Polygon& polygon);
    bool checkForSelfIntersections(const Polygon& polygon);
    bool isPolygonInsidePolygon(const Polygon& inner, const Polygon& outer);
    
    // Toolpath optimization
    std::vector<Path> optimizeToolpathOrder(const std::vector<Path>& toolpaths);
    std::vector<Path> removeRedundantMoves(const std::vector<Path>& toolpaths);
    
    // Error handling and reporting
    void addWarning(CAMOperationResult& result, const std::string& warning);
    void addError(CAMOperationResult& result, const std::string& error);
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_CAM_PROCESSOR_H 