#ifndef NWSS_CNC_TOOL_OFFSET_H
#define NWSS_CNC_TOOL_OFFSET_H

#include "core/geometry.h"
#include "core/tool.h"
#include <vector>

namespace nwss {
namespace cnc {

/**
 * Tool offset calculator for compensating tool paths
 */
class ToolOffset {
public:
    /**
     * Calculate offset path for tool compensation
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
     * Calculate multiple offset paths for different offset amounts
     * @param originalPath The original path to offset
     * @param offsets Vector of offset amounts (positive = outside, negative = inside)
     * @param tolerance Tolerance for offset calculations
     * @return Vector of offset paths
     */
    static std::vector<Path> calculateMultipleOffsets(const Path& originalPath,
                                                     const std::vector<double>& offsets,
                                                     double tolerance = 0.01);
    
    /**
     * Determine automatic offset direction based on path characteristics
     * @param path The path to analyze
     * @return Recommended offset direction
     */
    static ToolOffsetDirection determineOffsetDirection(const Path& path);
    
    /**
     * Check if a feature is too small for the given tool
     * @param path The path representing the feature
     * @param toolDiameter The tool diameter
     * @return True if the feature is too small to machine with this tool
     */
    static bool isFeatureTooSmall(const Path& path, double toolDiameter);
    
    /**
     * Calculate minimum feature size in a path
     * @param path The path to analyze
     * @return Minimum feature size (diameter of smallest circle that fits)
     */
    static double calculateMinimumFeatureSize(const Path& path);
    
    /**
     * Validate if tool can machine all features in paths
     * @param paths Vector of paths to check
     * @param toolDiameter Tool diameter
     * @param warnings Output vector for warning messages
     * @return True if all features can be machined
     */
    static bool validateToolForPaths(const std::vector<Path>& paths,
                                   double toolDiameter,
                                   std::vector<std::string>& warnings);

private:
    /**
     * Offset a single line segment
     * @param start Start point of the line
     * @param end End point of the line
     * @param offset Offset amount (positive = left side, negative = right side)
     * @return Pair of offset start and end points
     */
    static std::pair<Point2D, Point2D> offsetLineSegment(const Point2D& start,
                                                        const Point2D& end,
                                                        double offset);
    
    /**
     * Calculate offset for a polyline path
     * @param points Vector of path points
     * @param offset Offset amount
     * @param isClosed Whether the path is closed
     * @param tolerance Tolerance for calculations
     * @return Vector of offset points
     */
    static std::vector<Point2D> offsetPolyline(const std::vector<Point2D>& points,
                                              double offset,
                                              bool isClosed,
                                              double tolerance);
    
    /**
     * Find intersection point of two lines
     * @param p1 First point of first line
     * @param p2 Second point of first line
     * @param p3 First point of second line
     * @param p4 Second point of second line
     * @param intersection Output intersection point
     * @return True if intersection found
     */
    static bool findLineIntersection(const Point2D& p1, const Point2D& p2,
                                   const Point2D& p3, const Point2D& p4,
                                   Point2D& intersection);
    
    /**
     * Calculate perpendicular offset point
     * @param point The original point
     * @param direction Direction vector (normalized)
     * @param offset Offset distance
     * @return Offset point
     */
    static Point2D calculatePerpendicularOffset(const Point2D& point,
                                              const Point2D& direction,
                                              double offset);
    
    /**
     * Determine if a path is clockwise oriented
     * @param points Path points
     * @return True if clockwise
     */
    static bool isClockwise(const std::vector<Point2D>& points);
    
    /**
     * Remove self-intersections from offset path
     * @param points Input points that may have self-intersections
     * @param tolerance Tolerance for intersection detection
     * @return Cleaned path without self-intersections
     */
    static std::vector<Point2D> removeSelfIntersections(const std::vector<Point2D>& points,
                                                       double tolerance);
    
         /**
      * Simplify path by removing redundant points
      * @param points Input points
      * @param tolerance Tolerance for simplification
      * @return Simplified path
      */
     static std::vector<Point2D> simplifyPath(const std::vector<Point2D>& points,
                                            double tolerance);
    
     /**
      * Calculate minimum distance between two line segments
      * @param p1 First point of first segment
      * @param p2 Second point of first segment
      * @param p3 First point of second segment
      * @param p4 Second point of second segment
      * @return Minimum distance between segments
      */
     static double calculateSegmentDistance(const Point2D& p1, const Point2D& p2,
                                          const Point2D& p3, const Point2D& p4);
     
     /**
      * Check if a path forms a closed loop
      * @param points Path points
      * @param tolerance Tolerance for determining if first and last points are close
      * @return True if path is closed
      */
     static bool isPathClosed(const std::vector<Point2D>& points, double tolerance = 0.01);
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_TOOL_OFFSET_H 