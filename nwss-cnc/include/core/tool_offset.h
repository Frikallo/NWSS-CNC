#ifndef NWSS_CNC_TOOL_OFFSET_H
#define NWSS_CNC_TOOL_OFFSET_H

#include "core/geometry.h"
#include "core/tool.h"
#include <clipper2/clipper.h>
#include <vector>
#include <string>

namespace nwss {
namespace cnc {

/**
 * Clipper2-based tool offset calculator for professional CNC operations
 * This replaces the legacy implementation with robust polygon operations
 */
class ToolOffset {
public:
    /**
     * Tool offset options for fine-tuning the offsetting process
     */
    struct OffsetOptions {
        double arcTolerance;      // Arc approximation tolerance (smaller = smoother)
        double miterLimit;        // Miter limit for sharp corners
        bool preserveCollinear;   // Preserve collinear edges
        bool reverseSolution;     // Reverse solution orientation
        
        // Validation options
        double minFeatureSize;    // Minimum feature size to keep (mm)
        double maxOffsetRatio;    // Maximum offset as ratio of feature size
        bool validateResults;     // Enable result validation
        
        // Precision options
        double precision;         // Coordinate precision (mm)
        int scaleFactor;          // Internal scaling factor for Clipper2
        
        // Constructor with default values
        OffsetOptions() 
            : arcTolerance(0.25)
            , miterLimit(2.0)
            , preserveCollinear(false)
            , reverseSolution(false)
            , minFeatureSize(0.1)
            , maxOffsetRatio(0.8)
            , validateResults(true)
            , precision(0.001)
            , scaleFactor(1000)
        {}
    };

    /**
     * Result of tool offset operation with validation info
     */
    struct OffsetResult {
        std::vector<Path> paths;         // Resulting offset paths
        bool success;                    // Whether operation succeeded
        std::vector<std::string> warnings; // Non-fatal issues
        std::vector<std::string> errors;   // Fatal errors
        
        // Statistics
        size_t originalPathCount;
        size_t resultPathCount;
        double originalTotalLength;
        double resultTotalLength;
        double actualOffsetDistance;
        
        // Validation metrics
        double minFeatureSize;
        double maxFeatureSize;
        bool hasDegenerate;
        bool hasSelfIntersections;
        
        // Constructor with default values
        OffsetResult()
            : success(false)
            , originalPathCount(0)
            , resultPathCount(0)
            , originalTotalLength(0.0)
            , resultTotalLength(0.0)
            , actualOffsetDistance(0.0)
            , minFeatureSize(0.0)
            , maxFeatureSize(0.0)
            , hasDegenerate(false)
            , hasSelfIntersections(false)
        {}
    };

    /**
     * Calculate tool offset using Clipper2 (primary method)
     * @param originalPaths The original paths to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDirection Direction to offset the paths
     * @param options Advanced offsetting options
     * @return Complete offset result with validation
     */
    static OffsetResult calculateToolOffset(const std::vector<Path>& originalPaths,
                                          double toolDiameter,
                                          ToolOffsetDirection offsetDirection,
                                          const OffsetOptions& options = OffsetOptions{});

    /**
     * Calculate tool offset for a single path (convenience method)
     * @param originalPath The original path to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDirection Direction to offset the path
     * @param options Advanced offsetting options
     * @return Complete offset result with validation
     */
    static OffsetResult calculateToolOffset(const Path& originalPath,
                                          double toolDiameter,
                                          ToolOffsetDirection offsetDirection,
                                          const OffsetOptions& options = OffsetOptions{});

    /**
     * Calculate multiple offset passes (for roughing/finishing operations)
     * @param originalPaths The original paths to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDistances List of offset distances to apply
     * @param options Advanced offsetting options
     * @return Vector of offset results, one per distance
     */
    static std::vector<OffsetResult> calculateMultipleOffsets(
        const std::vector<Path>& originalPaths,
        double toolDiameter,
        const std::vector<double>& offsetDistances,
        const OffsetOptions& options = OffsetOptions{});

    /**
     * Validate if a tool can be used with given paths
     * @param paths The paths to validate
     * @param toolDiameter The tool diameter to check
     * @param warnings Output vector for warnings
     * @return True if tool is suitable for all paths
     */
    static bool validateToolForPaths(const std::vector<Path>& paths,
                                   double toolDiameter,
                                   std::vector<std::string>& warnings);

    /**
     * Determine optimal offset direction based on path characteristics
     * @param paths The paths to analyze
     * @return Recommended offset direction
     */
    static ToolOffsetDirection determineOptimalOffsetDirection(const std::vector<Path>& paths);

    /**
     * Calculate minimum feature size in a set of paths
     * @param paths The paths to analyze
     * @return Minimum feature size found (mm)
     */
    static double calculateMinimumFeatureSize(const std::vector<Path>& paths);

    /**
     * Check if any features are too small for the given tool
     * @param paths The paths to check
     * @param toolDiameter The tool diameter
     * @return True if any features are too small
     */
    static bool hasFeaturesTooSmallForTool(const std::vector<Path>& paths, double toolDiameter);

    /**
     * Clean up paths by removing degenerate segments and self-intersections
     * @param paths The paths to clean
     * @param tolerance Tolerance for cleanup operations
     * @return Cleaned paths
     */
    static std::vector<Path> cleanupPaths(const std::vector<Path>& paths, double tolerance = 0.001);

    /**
     * Simplify paths while preserving important features
     * @param paths The paths to simplify
     * @param tolerance Simplification tolerance
     * @return Simplified paths
     */
    static std::vector<Path> simplifyPaths(const std::vector<Path>& paths, double tolerance = 0.01);

private:
    // Core Clipper2 operations
    static Clipper2Lib::Paths64 pathsToClipper(const std::vector<Path>& paths, int scaleFactor);
    static std::vector<Path> clipperToPaths(const Clipper2Lib::Paths64& clipperPaths, int scaleFactor);
    static Clipper2Lib::Path64 pathToClipper(const Path& path, int scaleFactor);
    static Path clipperToPath(const Clipper2Lib::Path64& clipperPath, int scaleFactor);
    
    // Offset calculation helpers
    static double calculateOffsetAmount(double toolDiameter, ToolOffsetDirection direction, bool isClockwise);
    static Clipper2Lib::JoinType getJoinType(const OffsetOptions& options);
    static Clipper2Lib::EndType getEndType(bool isClosedPath);
    static bool isPathClosed(const Path& path, double tolerance = 0.001);
    static bool isClockwise(const Path& path);
    
    // Validation and analysis
    static OffsetResult validateOffsetResult(const std::vector<Path>& originalPaths,
                                           const std::vector<Path>& offsetPaths,
                                           double expectedOffset,
                                           const OffsetOptions& options);
    static double calculatePathLength(const Path& path);
    static double calculateActualOffset(const Path& original, const Path& offset);
    static bool hasValidGeometry(const Path& path);
    static bool hasSelfIntersections(const Path& path);
    
    // Error handling
    static void addWarning(OffsetResult& result, const std::string& message);
    static void addError(OffsetResult& result, const std::string& message);
};

// Legacy compatibility layer (deprecated - use ToolOffset class instead)
namespace legacy {
    Path calculateOffset(const Path& originalPath, 
                        double toolDiameter,
                        ToolOffsetDirection offsetDirection,
                        double tolerance = 0.01);
    
    Path calculateHighPrecisionOffset(const Path& originalPath,
                                    double toolDiameter,
                                    ToolOffsetDirection offsetDirection,
                                    double tolerance = 0.001);
    
    std::vector<Path> calculateMultipleOffsets(const Path& originalPath,
                                             const std::vector<double>& offsets,
                                             double tolerance = 0.01);
}

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_TOOL_OFFSET_H 