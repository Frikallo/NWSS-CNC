#include "core/tool_offset.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace nwss {
namespace cnc {

Path ToolOffset::calculateOffset(const Path& originalPath, 
                               double toolDiameter,
                               ToolOffsetDirection offsetDirection,
                               double tolerance) {
    if (originalPath.empty() || toolDiameter <= 0) {
        return Path();
    }
    
    const std::vector<Point2D>& points = originalPath.getPoints();
    if (points.size() < 2) {
        return Path();
    }
    
    // Calculate offset amount (radius, not diameter)
    double offsetAmount = toolDiameter / 2.0;
    
    // Determine offset direction
    bool isClosedPath = isPathClosed(points);
    bool clockwise = isClockwise(points);
    
    switch (offsetDirection) {
        case ToolOffsetDirection::INSIDE:
            // For inside offset, we need to offset towards the interior
            offsetAmount = clockwise ? offsetAmount : -offsetAmount;
            break;
        case ToolOffsetDirection::OUTSIDE:
            // For outside offset, we need to offset towards the exterior
            offsetAmount = clockwise ? -offsetAmount : offsetAmount;
            break;
        case ToolOffsetDirection::ON_PATH:
            offsetAmount = 0.0;
            break;
        case ToolOffsetDirection::AUTO:
            // Auto-determine based on path characteristics
            offsetDirection = determineOffsetDirection(originalPath);
            return calculateOffset(originalPath, toolDiameter, offsetDirection, tolerance);
    }
    
    if (std::abs(offsetAmount) < tolerance) {
        return originalPath; // No significant offset needed
    }
    
    // Calculate offset points
    std::vector<Point2D> offsetPoints = offsetPolyline(points, offsetAmount, isClosedPath, tolerance);
    
    if (offsetPoints.empty()) {
        return Path(); // Offset failed
    }
    
    // Create and return offset path
    Path offsetPath;
    for (const auto& point : offsetPoints) {
        offsetPath.addPoint(point);
    }
    
    return offsetPath;
}

std::vector<Path> ToolOffset::calculateMultipleOffsets(const Path& originalPath,
                                                     const std::vector<double>& offsets,
                                                     double tolerance) {
    std::vector<Path> offsetPaths;
    offsetPaths.reserve(offsets.size());
    
    for (double offset : offsets) {
        ToolOffsetDirection direction = (offset >= 0) ? ToolOffsetDirection::OUTSIDE : ToolOffsetDirection::INSIDE;
        Path offsetPath = calculateOffset(originalPath, std::abs(offset) * 2.0, direction, tolerance);
        offsetPaths.push_back(offsetPath);
    }
    
    return offsetPaths;
}

ToolOffsetDirection ToolOffset::determineOffsetDirection(const Path& path) {
    if (path.empty()) {
        return ToolOffsetDirection::ON_PATH;
    }
    
    // For closed paths, determine if it's an outer boundary or inner hole
    const std::vector<Point2D>& points = path.getPoints();
    if (isPathClosed(points)) {
        bool clockwise = isClockwise(points);
        
        // Convention: clockwise = outer boundary (cut outside), counter-clockwise = hole (cut inside)
        return clockwise ? ToolOffsetDirection::OUTSIDE : ToolOffsetDirection::INSIDE;
    }
    
    // For open paths, default to center-line cutting
    return ToolOffsetDirection::ON_PATH;
}

bool ToolOffset::isFeatureTooSmall(const Path& path, double toolDiameter) {
    double minFeatureSize = calculateMinimumFeatureSize(path);
    return minFeatureSize < toolDiameter;
}

double ToolOffset::calculateMinimumFeatureSize(const Path& path) {
    if (path.empty()) {
        return 0.0;
    }
    
    const std::vector<Point2D>& points = path.getPoints();
    if (points.size() < 3) {
        return 0.0; // Need at least 3 points to form a feature
    }
    
    double minDistance = std::numeric_limits<double>::max();
    
    // For closed paths, find the minimum distance between non-adjacent segments
    if (isPathClosed(points)) {
        for (size_t i = 0; i < points.size(); ++i) {
            Point2D p1 = points[i];
            Point2D p2 = points[(i + 1) % points.size()];
            
            // Check distance to all other non-adjacent segments
            for (size_t j = i + 2; j < points.size(); ++j) {
                if (j == points.size() - 1 && i == 0) continue; // Skip adjacent segments
                
                Point2D p3 = points[j];
                Point2D p4 = points[(j + 1) % points.size()];
                
                // Calculate minimum distance between two line segments
                double dist = calculateSegmentDistance(p1, p2, p3, p4);
                minDistance = std::min(minDistance, dist);
            }
        }
    } else {
        // For open paths, find minimum distance between any two points
        for (size_t i = 0; i < points.size(); ++i) {
            for (size_t j = i + 2; j < points.size(); ++j) {
                double dist = std::sqrt(std::pow(points[i].x - points[j].x, 2) + 
                                      std::pow(points[i].y - points[j].y, 2));
                minDistance = std::min(minDistance, dist);
            }
        }
    }
    
    return (minDistance == std::numeric_limits<double>::max()) ? 0.0 : minDistance;
}

bool ToolOffset::validateToolForPaths(const std::vector<Path>& paths,
                                     double toolDiameter,
                                     std::vector<std::string>& warnings) {
    warnings.clear();
    bool allValid = true;
    
    for (size_t i = 0; i < paths.size(); ++i) {
        const Path& path = paths[i];
        
        if (isFeatureTooSmall(path, toolDiameter)) {
            std::stringstream ss;
            ss << "Path " << (i + 1) << ": Feature too small for tool diameter " 
               << toolDiameter << "mm (min feature size: " 
               << calculateMinimumFeatureSize(path) << "mm)";
            warnings.push_back(ss.str());
            allValid = false;
        }
    }
    
    return allValid;
}

std::pair<Point2D, Point2D> ToolOffset::offsetLineSegment(const Point2D& start,
                                                        const Point2D& end,
                                                        double offset) {
    // Calculate direction vector
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double length = std::sqrt(dx * dx + dy * dy);
    
    if (length < 1e-10) {
        return std::make_pair(start, end); // Degenerate line
    }
    
    // Normalize direction vector
    dx /= length;
    dy /= length;
    
    // Calculate perpendicular offset vector (rotate 90 degrees)
    double offsetX = -dy * offset;
    double offsetY = dx * offset;
    
    // Apply offset to both points
    Point2D offsetStart(start.x + offsetX, start.y + offsetY);
    Point2D offsetEnd(end.x + offsetX, end.y + offsetY);
    
    return std::make_pair(offsetStart, offsetEnd);
}

std::vector<Point2D> ToolOffset::offsetPolyline(const std::vector<Point2D>& points,
                                              double offset,
                                              bool isClosed,
                                              double tolerance) {
    if (points.size() < 2) {
        return std::vector<Point2D>();
    }
    
    std::vector<Point2D> offsetPoints;
    
    // Handle special case of only two points
    if (points.size() == 2) {
        auto offsetSeg = offsetLineSegment(points[0], points[1], offset);
        offsetPoints.push_back(offsetSeg.first);
        offsetPoints.push_back(offsetSeg.second);
        return offsetPoints;
    }
    
    std::vector<std::pair<Point2D, Point2D>> offsetSegments;
    
    // Create offset segments
    size_t numSegments = isClosed ? points.size() : points.size() - 1;
    for (size_t i = 0; i < numSegments; ++i) {
        Point2D start = points[i];
        Point2D end = points[(i + 1) % points.size()];
        offsetSegments.push_back(offsetLineSegment(start, end, offset));
    }
    
    // Connect offset segments
    for (size_t i = 0; i < offsetSegments.size(); ++i) {
        if (i == 0 && !isClosed) {
            // First segment of open path
            offsetPoints.push_back(offsetSegments[i].first);
        }
        
        if (i < offsetSegments.size() - 1 || isClosed) {
            // Find intersection between current and next segment
            size_t nextIndex = (i + 1) % offsetSegments.size();
            Point2D intersection;
            
            if (findLineIntersection(offsetSegments[i].first, offsetSegments[i].second,
                                   offsetSegments[nextIndex].first, offsetSegments[nextIndex].second,
                                   intersection)) {
                offsetPoints.push_back(intersection);
            } else {
                // No intersection, use end of current segment
                offsetPoints.push_back(offsetSegments[i].second);
                if (!isClosed || i < offsetSegments.size() - 1) {
                    offsetPoints.push_back(offsetSegments[nextIndex].first);
                }
            }
        } else {
            // Last segment of open path
            offsetPoints.push_back(offsetSegments[i].second);
        }
    }
    
    // Clean up the result
    offsetPoints = removeSelfIntersections(offsetPoints, tolerance);
    offsetPoints = simplifyPath(offsetPoints, tolerance);
    
    return offsetPoints;
}

bool ToolOffset::findLineIntersection(const Point2D& p1, const Point2D& p2,
                                     const Point2D& p3, const Point2D& p4,
                                     Point2D& intersection) {
    double x1 = p1.x, y1 = p1.y;
    double x2 = p2.x, y2 = p2.y;
    double x3 = p3.x, y3 = p3.y;
    double x4 = p4.x, y4 = p4.y;
    
    double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    
    if (std::abs(denom) < 1e-10) {
        return false; // Lines are parallel
    }
    
    double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    
    intersection.x = x1 + t * (x2 - x1);
    intersection.y = y1 + t * (y2 - y1);
    
    return true;
}

Point2D ToolOffset::calculatePerpendicularOffset(const Point2D& point,
                                               const Point2D& direction,
                                               double offset) {
    // Rotate direction vector 90 degrees
    Point2D perpendicular(-direction.y, direction.x);
    
    return Point2D(point.x + perpendicular.x * offset,
                   point.y + perpendicular.y * offset);
}

bool ToolOffset::isClockwise(const std::vector<Point2D>& points) {
    if (points.size() < 3) {
        return true; // Default to clockwise for insufficient points
    }
    
    // Calculate signed area using shoelace formula
    double signedArea = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        size_t j = (i + 1) % points.size();
        signedArea += (points[j].x - points[i].x) * (points[j].y + points[i].y);
    }
    
    return signedArea > 0; // Positive area = clockwise in screen coordinates
}

std::vector<Point2D> ToolOffset::removeSelfIntersections(const std::vector<Point2D>& points,
                                                       double tolerance) {
    // Simple implementation - just remove points that are too close together
    if (points.size() < 2) {
        return points;
    }
    
    std::vector<Point2D> cleanedPoints;
    cleanedPoints.push_back(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        Point2D& last = cleanedPoints.back();
        const Point2D& current = points[i];
        
        double distance = std::sqrt(std::pow(current.x - last.x, 2) + 
                                  std::pow(current.y - last.y, 2));
        
        if (distance > tolerance) {
            cleanedPoints.push_back(current);
        }
    }
    
    return cleanedPoints;
}

std::vector<Point2D> ToolOffset::simplifyPath(const std::vector<Point2D>& points,
                                            double tolerance) {
    if (points.size() < 3) {
        return points;
    }
    
    std::vector<Point2D> simplified;
    simplified.push_back(points[0]);
    
    for (size_t i = 1; i < points.size() - 1; ++i) {
        const Point2D& prev = points[i - 1];
        const Point2D& current = points[i];
        const Point2D& next = points[i + 1];
        
        // Calculate cross product to determine if points are collinear
        double cross = (current.x - prev.x) * (next.y - prev.y) - 
                      (current.y - prev.y) * (next.x - prev.x);
        
        if (std::abs(cross) > tolerance) {
            simplified.push_back(current);
        }
    }
    
    simplified.push_back(points.back());
    return simplified;
}

double ToolOffset::calculateSegmentDistance(const Point2D& p1, const Point2D& p2,
                                          const Point2D& p3, const Point2D& p4) {
    // This is a simplified version - full implementation would be more complex
    // For now, just return minimum distance between endpoints
    double dist1 = std::sqrt(std::pow(p1.x - p3.x, 2) + std::pow(p1.y - p3.y, 2));
    double dist2 = std::sqrt(std::pow(p1.x - p4.x, 2) + std::pow(p1.y - p4.y, 2));
    double dist3 = std::sqrt(std::pow(p2.x - p3.x, 2) + std::pow(p2.y - p3.y, 2));
    double dist4 = std::sqrt(std::pow(p2.x - p4.x, 2) + std::pow(p2.y - p4.y, 2));
    
    return std::min({dist1, dist2, dist3, dist4});
}

bool ToolOffset::isPathClosed(const std::vector<Point2D>& points, double tolerance) {
    if (points.size() < 3) {
        return false;
    }
    
    const Point2D& first = points.front();
    const Point2D& last = points.back();
    
    double distance = std::sqrt(std::pow(last.x - first.x, 2) + std::pow(last.y - first.y, 2));
    return distance <= tolerance;
}

} // namespace cnc
} // namespace nwss 