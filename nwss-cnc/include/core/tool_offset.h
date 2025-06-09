#ifndef NWSS_CNC_TOOL_OFFSET_H
#define NWSS_CNC_TOOL_OFFSET_H

#include "core/geometry.h"
#include "core/tool.h"
#include <vector>
#include <memory>

namespace nwss {
namespace cnc {

/**
 * Curve segment types for high-precision offsetting
 */
enum class CurveType {
    LINE,
    CIRCULAR_ARC,
    BEZIER_CUBIC,
    BEZIER_QUADRATIC
};

/**
 * High-precision curve segment for detailed offsetting
 */
struct CurveSegment {
    CurveType type;
    std::vector<Point2D> controlPoints;
    double startParameter = 0.0;
    double endParameter = 1.0;
    
    // Evaluate point on curve at parameter t [0,1]
    Point2D evaluateAt(double t) const;
    
    // Get tangent vector at parameter t [0,1]
    Point2D getTangentAt(double t) const;
    
    // Get normal vector at parameter t [0,1] 
    Point2D getNormalAt(double t) const;
    
    // Get curvature at parameter t [0,1]
    double getCurvatureAt(double t) const;
    
    // Convert to polyline with given tolerance
    std::vector<Point2D> toPolyline(double tolerance) const;
    
    // Get bounding box
    void getBounds(double& minX, double& minY, double& maxX, double& maxY) const;
};

/**
 * High-precision path representation
 */
class PrecisionPath {
public:
    PrecisionPath() = default;
    explicit PrecisionPath(const Path& simplePath, double tolerance = 0.001);
    
    void addSegment(const CurveSegment& segment);
    void addLine(const Point2D& start, const Point2D& end);
    void addArc(const Point2D& center, double radius, double startAngle, double endAngle);
    
    const std::vector<CurveSegment>& getSegments() const { return m_segments; }
    bool isClosed() const;
    bool isEmpty() const { return m_segments.empty(); }
    
    // Convert back to simple path
    Path toSimplePath(double tolerance = 0.001) const;
    
    // Get total length
    double getLength() const;
    
    // Get bounding box
    void getBounds(double& minX, double& minY, double& maxX, double& maxY) const;
    
private:
    std::vector<CurveSegment> m_segments;
    double m_tolerance;
};

/**
 * Advanced tool offset calculator for high-precision work
 */
class PrecisionToolOffset {
public:
    struct OffsetOptions {
        double tolerance;           // Geometric tolerance
        double minSegmentLength;     // Minimum segment length
        double maxSegmentLength;      // Maximum segment length
        int maxIterations;            // Maximum iterations for convergence
        bool preserveSharpCorners;   // Preserve sharp corners
        bool adaptiveRefinement;     // Use adaptive refinement
        double cornerThreshold;       // Threshold for corner detection (radians)
        double maxCurvatureError;   // Maximum curvature approximation error
        
        // Constructor with default values
        OffsetOptions() 
            : tolerance(0.001)
            , minSegmentLength(0.01)
            , maxSegmentLength(1.0)
            , maxIterations(100)
            , preserveSharpCorners(true)
            , adaptiveRefinement(true)
            , cornerThreshold(0.1)
            , maxCurvatureError(0.001)
        {}
    };
    
    /**
     * High-precision offset calculation
     * @param originalPath The original path to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDirection Direction to offset the path
     * @param options Advanced offsetting options
     * @return Precision offset path
     */
    static PrecisionPath calculatePrecisionOffset(const PrecisionPath& originalPath,
                                                 double toolDiameter,
                                                 ToolOffsetDirection offsetDirection,
                                                 const OffsetOptions& options = OffsetOptions{});
    
    /**
     * Adaptive offset that chooses strategy based on feature analysis
     * @param originalPath The original path to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDirection Direction to offset the path
     * @param options Advanced offsetting options
     * @return Precision offset path with optimal strategy
     */
    static PrecisionPath calculateAdaptiveOffset(const PrecisionPath& originalPath,
                                                double toolDiameter,
                                                ToolOffsetDirection offsetDirection,
                                                const OffsetOptions& options = OffsetOptions{});
    
    /**
     * Validate offset quality and accuracy
     * @param originalPath Original path
     * @param offsetPath Offset path
     * @param expectedOffset Expected offset distance
     * @param options Validation options
     * @return Quality metrics and validation results
     */
    struct ValidationResult {
        bool isValid = false;
        double averageError = 0.0;
        double maxError = 0.0;
        double minError = 0.0;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };
    
    static ValidationResult validateOffset(const PrecisionPath& originalPath,
                                         const PrecisionPath& offsetPath,
                                         double expectedOffset,
                                         const OffsetOptions& options = OffsetOptions{});

private:
    // Core offset algorithms
    static PrecisionPath offsetSingleCurve(const CurveSegment& curve,
                                          double offset,
                                          const OffsetOptions& options);
    
    static std::vector<CurveSegment> offsetAndConnect(const std::vector<CurveSegment>& segments,
                                                     double offset,
                                                     bool isClosed,
                                                     const OffsetOptions& options);
    
    // Intersection and connection algorithms
    static bool findCurveIntersection(const CurveSegment& curve1,
                                    const CurveSegment& curve2,
                                    std::vector<Point2D>& intersections,
                                    const OffsetOptions& options);
    
    static CurveSegment createFilletConnection(const CurveSegment& seg1,
                                             const CurveSegment& seg2,
                                             double offset,
                                             const OffsetOptions& options);
    
    // Feature analysis
    static double analyzeMinimumFeatureSize(const PrecisionPath& path);
    static double analyzeMaximumCurvature(const PrecisionPath& path);
    static std::vector<Point2D> findSharpCorners(const PrecisionPath& path, double threshold);
    
    // Quality control
    static bool isOffsetReasonable(const PrecisionPath& original,
                                 const PrecisionPath& offset,
                                 double expectedOffset);
    
    static PrecisionPath refinePath(const PrecisionPath& path, const OffsetOptions& options);
    static PrecisionPath smoothPath(const PrecisionPath& path, const OffsetOptions& options);
};

/**
 * Legacy tool offset calculator (maintained for compatibility)
 */
class ToolOffset {
public:
    /**
     * Calculate offset path for tool compensation (LEGACY - use PrecisionToolOffset for new work)
     * @param originalPath The original path to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDirection Direction to offset the path
     * @param tolerance Tolerance for offset calculations
     * @return Offset path, empty if offset failed
     */
    static Path calculateOffset(const Path& originalPath, 
                               double toolDiameter,
                               ToolOffsetDirection offsetDirection,
                               double tolerance = 0.01);
    
    /**
     * High-precision wrapper that uses the new algorithm
     * @param originalPath The original path to offset
     * @param toolDiameter The diameter of the cutting tool
     * @param offsetDirection Direction to offset the path
     * @param tolerance Tolerance for offset calculations
     * @return High-precision offset path converted to simple path
     */
    static Path calculateHighPrecisionOffset(const Path& originalPath,
                                           double toolDiameter,
                                           ToolOffsetDirection offsetDirection,
                                           double tolerance = 0.001);
    
    // Legacy compatibility methods (existing interface preserved)
    static std::vector<Path> calculateMultipleOffsets(const Path& originalPath,
                                                     const std::vector<double>& offsets,
                                                     double tolerance = 0.01);
    
    static ToolOffsetDirection determineOffsetDirection(const Path& path);
    static bool isFeatureTooSmall(const Path& path, double toolDiameter);
    static double calculateMinimumFeatureSize(const Path& path);
    static bool validateToolForPaths(const std::vector<Path>& paths,
                                   double toolDiameter,
                                   std::vector<std::string>& warnings);

private:
    // Legacy implementation methods (existing code)
    static std::pair<Point2D, Point2D> offsetLineSegment(const Point2D& start,
                                                        const Point2D& end,
                                                        double offset);
    
    static std::vector<Point2D> offsetPolyline(const std::vector<Point2D>& points,
                                              double offset,
                                              bool isClosed,
                                              double tolerance);
    
    static bool findLineIntersection(const Point2D& p1, const Point2D& p2,
                                   const Point2D& p3, const Point2D& p4,
                                   Point2D& intersection);
    
    static Point2D calculatePerpendicularOffset(const Point2D& point,
                                              const Point2D& direction,
                                              double offset);
    
    static bool isClockwise(const std::vector<Point2D>& points);
    
    static std::vector<Point2D> removeSelfIntersections(const std::vector<Point2D>& points,
                                                       double tolerance);
    
    static std::vector<Point2D> simplifyPath(const std::vector<Point2D>& points,
                                            double tolerance);
    
    static double calculateSegmentDistance(const Point2D& p1, const Point2D& p2,
                                          const Point2D& p3, const Point2D& p4);
     
    static bool isPathClosed(const std::vector<Point2D>& points, double tolerance = 0.01);
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_TOOL_OFFSET_H 