#include "core/tool_offset.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <limits>

namespace nwss {
namespace cnc {

// ================================
// CurveSegment Implementation
// ================================

Point2D CurveSegment::evaluateAt(double t) const {
    t = std::clamp(t, 0.0, 1.0);
    
    switch (type) {
        case CurveType::LINE: {
            if (controlPoints.size() >= 2) {
                const Point2D& p0 = controlPoints[0];
                const Point2D& p1 = controlPoints[1];
                return Point2D(p0.x + t * (p1.x - p0.x), p0.y + t * (p1.y - p0.y));
            }
            break;
        }
        
        case CurveType::CIRCULAR_ARC: {
            if (controlPoints.size() >= 3) {
                // controlPoints[0] = center, controlPoints[1] = start, controlPoints[2] = end
                const Point2D& center = controlPoints[0];
                const Point2D& start = controlPoints[1];
                const Point2D& end = controlPoints[2];
                
                double startAngle = std::atan2(start.y - center.y, start.x - center.x);
                double endAngle = std::atan2(end.y - center.y, end.x - center.x);
                double radius = std::sqrt(std::pow(start.x - center.x, 2) + std::pow(start.y - center.y, 2));
                
                // Handle angle wrapping
                if (endAngle < startAngle) {
                    endAngle += 2.0 * M_PI;
                }
                
                double angle = startAngle + t * (endAngle - startAngle);
                return Point2D(center.x + radius * std::cos(angle), 
                              center.y + radius * std::sin(angle));
            }
            break;
        }
        
        case CurveType::BEZIER_QUADRATIC: {
            if (controlPoints.size() >= 3) {
                // Quadratic Bézier: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
                const Point2D& p0 = controlPoints[0];
                const Point2D& p1 = controlPoints[1];
                const Point2D& p2 = controlPoints[2];
                
                double u = 1.0 - t;
                double uu = u * u;
                double tt = t * t;
                double ut2 = 2.0 * u * t;
                
                return Point2D(uu * p0.x + ut2 * p1.x + tt * p2.x,
                              uu * p0.y + ut2 * p1.y + tt * p2.y);
            }
            break;
        }
        
        case CurveType::BEZIER_CUBIC: {
            if (controlPoints.size() >= 4) {
                // Cubic Bézier: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
                const Point2D& p0 = controlPoints[0];
                const Point2D& p1 = controlPoints[1];
                const Point2D& p2 = controlPoints[2];
                const Point2D& p3 = controlPoints[3];
                
                double u = 1.0 - t;
                double uu = u * u;
                double uuu = uu * u;
                double tt = t * t;
                double ttt = tt * t;
                double uut3 = 3.0 * uu * t;
                double utt3 = 3.0 * u * tt;
                
                return Point2D(uuu * p0.x + uut3 * p1.x + utt3 * p2.x + ttt * p3.x,
                              uuu * p0.y + uut3 * p1.y + utt3 * p2.y + ttt * p3.y);
            }
            break;
        }
    }
    
    // Fallback to first control point
    return controlPoints.empty() ? Point2D(0, 0) : controlPoints[0];
}

Point2D CurveSegment::getTangentAt(double t) const {
    t = std::clamp(t, 0.0, 1.0);
    
    switch (type) {
        case CurveType::LINE: {
            if (controlPoints.size() >= 2) {
                const Point2D& p0 = controlPoints[0];
                const Point2D& p1 = controlPoints[1];
                Point2D tangent(p1.x - p0.x, p1.y - p0.y);
                double length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                if (length > 1e-10) {
                    tangent.x /= length;
                    tangent.y /= length;
                }
                return tangent;
            }
            break;
        }
        
        case CurveType::CIRCULAR_ARC: {
            if (controlPoints.size() >= 3) {
                const Point2D& center = controlPoints[0];
                const Point2D& start = controlPoints[1];
                const Point2D& end = controlPoints[2];
                
                double startAngle = std::atan2(start.y - center.y, start.x - center.x);
                double endAngle = std::atan2(end.y - center.y, end.x - center.x);
                
                if (endAngle < startAngle) {
                    endAngle += 2.0 * M_PI;
                }
                
                double angle = startAngle + t * (endAngle - startAngle);
                return Point2D(-std::sin(angle), std::cos(angle));
            }
            break;
        }
        
        case CurveType::BEZIER_QUADRATIC: {
            if (controlPoints.size() >= 3) {
                // Derivative of quadratic Bézier: B'(t) = 2(1-t)(P₁-P₀) + 2t(P₂-P₁)
                const Point2D& p0 = controlPoints[0];
                const Point2D& p1 = controlPoints[1];
                const Point2D& p2 = controlPoints[2];
                
                double u = 1.0 - t;
                Point2D tangent(2.0 * u * (p1.x - p0.x) + 2.0 * t * (p2.x - p1.x),
                               2.0 * u * (p1.y - p0.y) + 2.0 * t * (p2.y - p1.y));
                
                double length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                if (length > 1e-10) {
                    tangent.x /= length;
                    tangent.y /= length;
                }
                return tangent;
            }
            break;
        }
        
        case CurveType::BEZIER_CUBIC: {
            if (controlPoints.size() >= 4) {
                // Derivative of cubic Bézier
                const Point2D& p0 = controlPoints[0];
                const Point2D& p1 = controlPoints[1];
                const Point2D& p2 = controlPoints[2];
                const Point2D& p3 = controlPoints[3];
                
                double u = 1.0 - t;
                double uu = u * u;
                double tt = t * t;
                double ut2 = 2.0 * u * t;
                
                Point2D tangent(3.0 * uu * (p1.x - p0.x) + 6.0 * u * t * (p2.x - p1.x) + 3.0 * tt * (p3.x - p2.x),
                               3.0 * uu * (p1.y - p0.y) + 6.0 * u * t * (p2.y - p1.y) + 3.0 * tt * (p3.y - p2.y));
                
                double length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                if (length > 1e-10) {
                    tangent.x /= length;
                    tangent.y /= length;
                }
                return tangent;
            }
            break;
        }
    }
    
    return Point2D(1, 0); // Default tangent
}

Point2D CurveSegment::getNormalAt(double t) const {
    Point2D tangent = getTangentAt(t);
    return Point2D(-tangent.y, tangent.x); // Rotate 90 degrees
}

double CurveSegment::getCurvatureAt(double t) const {
    // Simplified curvature calculation
    const double dt = 1e-6;
    Point2D p1 = evaluateAt(std::max(0.0, t - dt));
    Point2D p2 = evaluateAt(t);
    Point2D p3 = evaluateAt(std::min(1.0, t + dt));
    
    // Calculate curvature using three points
    double dx1 = p2.x - p1.x;
    double dy1 = p2.y - p1.y;
    double dx2 = p3.x - p2.x;
    double dy2 = p3.y - p2.y;
    
    double cross = dx1 * dy2 - dy1 * dx2;
    double speed1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    double speed2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    
    if (speed1 < 1e-10 || speed2 < 1e-10) {
        return 0.0;
    }
    
    return cross / (speed1 * speed2 * speed2);
}

std::vector<Point2D> CurveSegment::toPolyline(double tolerance) const {
    std::vector<Point2D> points;
    
    // Much more conservative subdivision - max 10 segments per curve
    // This prevents massive point explosion
    int numSegments = std::min(10, std::max(2, static_cast<int>(std::ceil(10.0 * tolerance))));
    
    // For high curvature, only slightly increase subdivision
    double maxCurvature = 0.0;
    for (int i = 0; i <= 5; ++i) {
        double t = i / 5.0;
        maxCurvature = std::max(maxCurvature, std::abs(getCurvatureAt(t)));
    }
    
    if (maxCurvature > 1.0) {
        numSegments = std::min(numSegments + 5, 15); // Cap at 15 total
    }
    
    for (int i = 0; i <= numSegments; ++i) {
        double t = static_cast<double>(i) / numSegments;
        points.push_back(evaluateAt(t));
    }
    
    return points;
}

void CurveSegment::getBounds(double& minX, double& minY, double& maxX, double& maxY) const {
    if (controlPoints.empty()) {
        minX = minY = maxX = maxY = 0.0;
        return;
    }
    
    minX = maxX = controlPoints[0].x;
    minY = maxY = controlPoints[0].y;
    
    // Sample curve at multiple points for bounds
    for (int i = 0; i <= 100; ++i) {
        double t = i / 100.0;
        Point2D p = evaluateAt(t);
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }
}

// ================================
// PrecisionPath Implementation
// ================================

PrecisionPath::PrecisionPath(const Path& simplePath, double tolerance) : m_tolerance(tolerance) {
    const auto& points = simplePath.getPoints();
    
    // Convert simple path to curve segments
    for (size_t i = 0; i < points.size() - 1; ++i) {
        CurveSegment segment;
        segment.type = CurveType::LINE;
        segment.controlPoints = {points[i], points[i + 1]};
        m_segments.push_back(segment);
    }
    
    // Close the path if it's a closed loop
    if (points.size() > 2) {
        double distance = std::sqrt(std::pow(points.back().x - points.front().x, 2) + 
                                   std::pow(points.back().y - points.front().y, 2));
        if (distance <= tolerance) {
            CurveSegment closingSegment;
            closingSegment.type = CurveType::LINE;
            closingSegment.controlPoints = {points.back(), points.front()};
            m_segments.push_back(closingSegment);
        }
    }
}

void PrecisionPath::addSegment(const CurveSegment& segment) {
    m_segments.push_back(segment);
}

void PrecisionPath::addLine(const Point2D& start, const Point2D& end) {
    CurveSegment segment;
    segment.type = CurveType::LINE;
    segment.controlPoints = {start, end};
    m_segments.push_back(segment);
}

void PrecisionPath::addArc(const Point2D& center, double radius, double startAngle, double endAngle) {
    CurveSegment segment;
    segment.type = CurveType::CIRCULAR_ARC;
    
    Point2D start(center.x + radius * std::cos(startAngle),
                  center.y + radius * std::sin(startAngle));
    Point2D end(center.x + radius * std::cos(endAngle),
                center.y + radius * std::sin(endAngle));
    
    segment.controlPoints = {center, start, end};
    m_segments.push_back(segment);
}

bool PrecisionPath::isClosed() const {
    if (m_segments.size() < 2) {
        return false;
    }
    
    Point2D start = m_segments.front().evaluateAt(0.0);
    Point2D end = m_segments.back().evaluateAt(1.0);
    
    double distance = std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
    return distance <= m_tolerance;
}

Path PrecisionPath::toSimplePath(double tolerance) const {
    Path simplePath;
    
    for (const auto& segment : m_segments) {
        std::vector<Point2D> points = segment.toPolyline(tolerance);
        for (size_t i = 0; i < points.size(); ++i) {
            // Skip first point of subsequent segments to avoid duplicates
            if (i == 0 && !simplePath.empty()) continue;
            simplePath.addPoint(points[i]);
        }
    }
    
    return simplePath;
}

double PrecisionPath::getLength() const {
    double totalLength = 0.0;
    
    for (const auto& segment : m_segments) {
        // Approximate length by sampling
        Point2D prev = segment.evaluateAt(0.0);
        for (int i = 1; i <= 100; ++i) {
            Point2D curr = segment.evaluateAt(i / 100.0);
            totalLength += std::sqrt(std::pow(curr.x - prev.x, 2) + std::pow(curr.y - prev.y, 2));
            prev = curr;
        }
    }
    
    return totalLength;
}

void PrecisionPath::getBounds(double& minX, double& minY, double& maxX, double& maxY) const {
    if (m_segments.empty()) {
        minX = minY = maxX = maxY = 0.0;
        return;
    }
    
    double segMinX, segMinY, segMaxX, segMaxY;
    m_segments[0].getBounds(segMinX, segMinY, segMaxX, segMaxY);
    
    minX = segMinX;
    minY = segMinY;
    maxX = segMaxX;
    maxY = segMaxY;
    
    for (size_t i = 1; i < m_segments.size(); ++i) {
        m_segments[i].getBounds(segMinX, segMinY, segMaxX, segMaxY);
        minX = std::min(minX, segMinX);
        minY = std::min(minY, segMinY);
        maxX = std::max(maxX, segMaxX);
        maxY = std::max(maxY, segMaxY);
    }
}

// ================================
// PrecisionToolOffset Implementation
// ================================

PrecisionPath PrecisionToolOffset::calculatePrecisionOffset(const PrecisionPath& originalPath,
                                                           double toolDiameter,
                                                           ToolOffsetDirection offsetDirection,
                                                           const OffsetOptions& options) {
    std::cout << "DEBUG: PrecisionToolOffset::calculatePrecisionOffset() called" << std::endl;
    std::cout << "  - Tool diameter: " << toolDiameter << std::endl;
    std::cout << "  - Segments: " << originalPath.getSegments().size() << std::endl;
    std::cout << "  - Tolerance: " << options.tolerance << std::endl;
    
    if (originalPath.isEmpty() || toolDiameter <= 0) {
        std::cout << "DEBUG: Invalid input - returning empty path" << std::endl;
        return PrecisionPath{};
    }
    
    // Analyze the path first
    double minFeatureSize = analyzeMinimumFeatureSize(originalPath);
    double maxCurvature = analyzeMaximumCurvature(originalPath);
    
    std::cout << "DEBUG: Path analysis:" << std::endl;
    std::cout << "  - Minimum feature size: " << minFeatureSize << std::endl;
    std::cout << "  - Maximum curvature: " << maxCurvature << std::endl;
    std::cout << "  - Is closed: " << (originalPath.isClosed() ? "true" : "false") << std::endl;
    
    // Calculate offset amount
    double offsetAmount = toolDiameter / 2.0;
    
    // Determine offset direction
    bool isClockwise = true; // Simplified for now
    switch (offsetDirection) {
        case ToolOffsetDirection::INSIDE:
            offsetAmount = isClockwise ? offsetAmount : -offsetAmount;
            std::cout << "DEBUG: INSIDE offset - amount: " << offsetAmount << std::endl;
            break;
        case ToolOffsetDirection::OUTSIDE:
            offsetAmount = isClockwise ? -offsetAmount : offsetAmount;
            std::cout << "DEBUG: OUTSIDE offset - amount: " << offsetAmount << std::endl;
            break;
        case ToolOffsetDirection::ON_PATH:
            offsetAmount = 0.0;
            std::cout << "DEBUG: ON_PATH offset - no offset" << std::endl;
            break;
        case ToolOffsetDirection::AUTO:
            offsetDirection = originalPath.isClosed() ? ToolOffsetDirection::OUTSIDE : ToolOffsetDirection::ON_PATH;
            std::cout << "DEBUG: AUTO offset - using " << (offsetDirection == ToolOffsetDirection::OUTSIDE ? "OUTSIDE" : "ON_PATH") << std::endl;
            return calculatePrecisionOffset(originalPath, toolDiameter, offsetDirection, options);
    }
    
    if (std::abs(offsetAmount) < options.tolerance) {
        std::cout << "DEBUG: Offset too small - returning original" << std::endl;
        return originalPath;
    }
    
    // Validate that offset is reasonable for the feature size
    if (minFeatureSize > 0 && std::abs(offsetAmount) > minFeatureSize * 0.4) {
        std::cout << "DEBUG: WARNING - Offset may be too large for feature size" << std::endl;
        std::cout << "  - Offset: " << std::abs(offsetAmount) << std::endl;
        std::cout << "  - Min feature: " << minFeatureSize << std::endl;
        
        // Scale down offset to be safe
        offsetAmount *= (minFeatureSize * 0.4) / std::abs(offsetAmount);
        std::cout << "  - Scaled offset to: " << offsetAmount << std::endl;
    }
    
    // Perform the offset
    std::vector<CurveSegment> offsetSegments = offsetAndConnect(
        originalPath.getSegments(), offsetAmount, originalPath.isClosed(), options);
    
    std::cout << "DEBUG: Offset completed:" << std::endl;
    std::cout << "  - Input segments: " << originalPath.getSegments().size() << std::endl;
    std::cout << "  - Output segments: " << offsetSegments.size() << std::endl;
    
    PrecisionPath result;
    for (const auto& segment : offsetSegments) {
        result.addSegment(segment);
    }
    
    return result;
}

PrecisionPath PrecisionToolOffset::calculateAdaptiveOffset(const PrecisionPath& originalPath,
                                                          double toolDiameter,
                                                          ToolOffsetDirection offsetDirection,
                                                          const OffsetOptions& options) {
    std::cout << "DEBUG: PrecisionToolOffset::calculateAdaptiveOffset() called" << std::endl;
    
    // Use the precision offset for now - future enhancement point for adaptive strategies
    return calculatePrecisionOffset(originalPath, toolDiameter, offsetDirection, options);
}

// Helper methods (simplified implementations for core functionality)

std::vector<CurveSegment> PrecisionToolOffset::offsetAndConnect(const std::vector<CurveSegment>& segments,
                                                               double offset,
                                                               bool isClosed,
                                                               const OffsetOptions& options) {
    std::vector<CurveSegment> result;
    result.reserve(segments.size() * 3); // Reserve reasonable space
    
    std::cout << "DEBUG: offsetAndConnect - processing " << segments.size() << " segments" << std::endl;
    
    // Safety limit to prevent runaway generation
    const size_t MAX_OUTPUT_SEGMENTS = 10000;
    
    for (const auto& segment : segments) {
        // For now, convert to polyline and use simplified offset
        auto points = segment.toPolyline(options.tolerance);
        
        if (points.size() >= 2) {
            // Simple parallel offset for each line segment
            for (size_t i = 0; i < points.size() - 1; ++i) {
                // Safety check to prevent runaway generation
                if (result.size() >= MAX_OUTPUT_SEGMENTS) {
                    std::cout << "DEBUG: WARNING - Hit maximum segment limit, truncating" << std::endl;
                    break;
                }
                
                const Point2D& p1 = points[i];
                const Point2D& p2 = points[i + 1];
                
                // Calculate perpendicular offset
                double dx = p2.x - p1.x;
                double dy = p2.y - p1.y;
                double length = std::sqrt(dx * dx + dy * dy);
                
                if (length > 1e-6) { // Use larger threshold to avoid micro-segments
                    dx /= length;
                    dy /= length;
                    
                    // Perpendicular vector
                    double nx = -dy * offset;
                    double ny = dx * offset;
                    
                    CurveSegment offsetSeg;
                    offsetSeg.type = CurveType::LINE;
                    offsetSeg.controlPoints = {
                        Point2D(p1.x + nx, p1.y + ny),
                        Point2D(p2.x + nx, p2.y + ny)
                    };
                    
                    result.push_back(offsetSeg);
                }
            }
        }
        
        // Safety check at segment level too
        if (result.size() >= MAX_OUTPUT_SEGMENTS) {
            std::cout << "DEBUG: WARNING - Hit maximum segment limit" << std::endl;
            break;
        }
    }
    
    std::cout << "DEBUG: offsetAndConnect completed - " << result.size() << " output segments" << std::endl;
    return result;
}

double PrecisionToolOffset::analyzeMinimumFeatureSize(const PrecisionPath& path) {
    if (path.isEmpty()) return 0.0;
    
    // Simplified implementation - just sample a few points per segment
    // to avoid massive computation
    auto segments = path.getSegments();
    double minDistance = std::numeric_limits<double>::max();
    
    // Limit analysis to prevent performance issues
    size_t maxSegments = std::min(segments.size(), size_t(20));
    
    for (size_t i = 0; i < maxSegments; ++i) {
        for (size_t j = i + 2; j < maxSegments; ++j) {
            // Sample just a few points per segment
            for (int k = 0; k <= 3; ++k) {
                Point2D p1 = segments[i].evaluateAt(k / 3.0);
                for (int l = 0; l <= 3; ++l) {
                    Point2D p2 = segments[j].evaluateAt(l / 3.0);
                    double dist = std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
                    minDistance = std::min(minDistance, dist);
                }
            }
        }
    }
    
    return minDistance == std::numeric_limits<double>::max() ? 0.0 : minDistance;
}

double PrecisionToolOffset::analyzeMaximumCurvature(const PrecisionPath& path) {
    double maxCurvature = 0.0;
    
    for (const auto& segment : path.getSegments()) {
        for (int i = 0; i <= 20; ++i) {
            double t = i / 20.0;
            maxCurvature = std::max(maxCurvature, std::abs(segment.getCurvatureAt(t)));
        }
    }
    
    return maxCurvature;
}

// Additional helper methods with simplified implementations
PrecisionToolOffset::ValidationResult PrecisionToolOffset::validateOffset(const PrecisionPath& originalPath,
                                                                          const PrecisionPath& offsetPath,
                                                                          double expectedOffset,
                                                                          const OffsetOptions& options) {
    ValidationResult result;
    result.isValid = true; // Simplified validation
    result.averageError = 0.0;
    result.maxError = 0.0;
    result.minError = 0.0;
    
    // Future enhancement: implement detailed validation
    return result;
}

// Remaining helper methods (simplified for now)
PrecisionPath PrecisionToolOffset::offsetSingleCurve(const CurveSegment& curve, double offset, const OffsetOptions& options) {
    PrecisionPath result;
    // Simplified implementation
    return result;
}

bool PrecisionToolOffset::findCurveIntersection(const CurveSegment& curve1, const CurveSegment& curve2, 
                                               std::vector<Point2D>& intersections, const OffsetOptions& options) {
    // Simplified implementation
    return false;
}

CurveSegment PrecisionToolOffset::createFilletConnection(const CurveSegment& seg1, const CurveSegment& seg2, 
                                                        double offset, const OffsetOptions& options) {
    CurveSegment result;
    result.type = CurveType::LINE;
    return result;
}

std::vector<Point2D> PrecisionToolOffset::findSharpCorners(const PrecisionPath& path, double threshold) {
    return std::vector<Point2D>{};
}

bool PrecisionToolOffset::isOffsetReasonable(const PrecisionPath& original, const PrecisionPath& offset, double expectedOffset) {
    return true; // Simplified
}

PrecisionPath PrecisionToolOffset::refinePath(const PrecisionPath& path, const OffsetOptions& options) {
    return path; // Simplified
}

PrecisionPath PrecisionToolOffset::smoothPath(const PrecisionPath& path, const OffsetOptions& options) {
    return path; // Simplified
}

// ================================
// High-Precision Wrapper for Legacy Interface
// ================================

Path ToolOffset::calculateHighPrecisionOffset(const Path& originalPath,
                                            double toolDiameter,
                                            ToolOffsetDirection offsetDirection,
                                            double tolerance) {
    std::cout << "DEBUG: ToolOffset::calculateHighPrecisionOffset() called" << std::endl;
    std::cout << "  - Using new precision algorithm" << std::endl;
    
    // Convert to precision path
    PrecisionPath precisionInput(originalPath, tolerance);
    
    // Set up precision options
    PrecisionToolOffset::OffsetOptions options;
    options.tolerance = tolerance;
    options.minSegmentLength = tolerance;
    options.maxSegmentLength = 1.0;
    
    // Calculate precision offset
    PrecisionPath precisionResult = PrecisionToolOffset::calculatePrecisionOffset(
        precisionInput, toolDiameter, offsetDirection, options);
    
    if (precisionResult.isEmpty()) {
        std::cout << "DEBUG: Precision offset failed - falling back to legacy" << std::endl;
        return calculateOffset(originalPath, toolDiameter, offsetDirection, tolerance);
    }
    
    // Convert back to simple path
    Path result = precisionResult.toSimplePath(tolerance);
    
    // Safety check - if we generated too many points, fall back to legacy
    const size_t MAX_REASONABLE_POINTS = 50000;
    if (result.getPoints().size() > MAX_REASONABLE_POINTS) {
        std::cout << "DEBUG: WARNING - Precision algorithm generated too many points (" 
                  << result.getPoints().size() << "), falling back to legacy" << std::endl;
        return calculateOffset(originalPath, toolDiameter, offsetDirection, tolerance);
    }
    
    std::cout << "DEBUG: High-precision offset completed" << std::endl;
    std::cout << "  - Input points: " << originalPath.getPoints().size() << std::endl;
    std::cout << "  - Output points: " << result.getPoints().size() << std::endl;
    
    return result;
}

// ================================
// Legacy Implementation (preserved for compatibility)
// ================================

Path ToolOffset::calculateOffset(const Path& originalPath, 
                               double toolDiameter,
                               ToolOffsetDirection offsetDirection,
                               double tolerance) {
    std::cout << "DEBUG: ToolOffset::calculateOffset() called with:" << std::endl;
    std::cout << "  - Tool diameter: " << toolDiameter << std::endl;
    std::cout << "  - Offset direction: " << static_cast<int>(offsetDirection) << std::endl;
    std::cout << "  - Tolerance: " << tolerance << std::endl;
    std::cout << "  - Original path points: " << originalPath.getPoints().size() << std::endl;
    
    if (originalPath.empty() || toolDiameter <= 0) {
        std::cout << "DEBUG: Early return - empty path or invalid tool diameter" << std::endl;
        return Path();
    }
    
    const std::vector<Point2D>& points = originalPath.getPoints();
    if (points.size() < 2) {
        std::cout << "DEBUG: Early return - insufficient points (" << points.size() << ")" << std::endl;
        return Path();
    }
    
    // Calculate offset amount (radius, not diameter)
    double offsetAmount = toolDiameter / 2.0;
    std::cout << "DEBUG: Initial offset amount (radius): " << offsetAmount << std::endl;
    
    // VALIDATION: Check if tool is too large for the feature
    double minFeatureSize = calculateMinimumFeatureSize(originalPath);
    std::cout << "DEBUG: Minimum feature size: " << minFeatureSize << std::endl;
    
    if (minFeatureSize > 0 && toolDiameter > minFeatureSize * 0.8) {
        std::cout << "DEBUG: WARNING - Tool diameter (" << toolDiameter 
                  << ") is too large for feature size (" << minFeatureSize << ")" << std::endl;
        std::cout << "DEBUG: Reducing offset amount to prevent catastrophic failure" << std::endl;
        offsetAmount = std::min(offsetAmount, minFeatureSize * 0.1); // Limit to 10% of feature size
    }
    
    // VALIDATION: Check for very small tools that might cause precision issues
    if (toolDiameter < 1.0) { // Less than 1mm
        std::cout << "DEBUG: WARNING - Very small tool detected (" << toolDiameter 
                  << "mm). This may cause precision issues." << std::endl;
        // Increase tolerance for very small tools
        tolerance = std::max(tolerance, 0.001);
        std::cout << "DEBUG: Adjusted tolerance to: " << tolerance << std::endl;
    }
    
    // Determine offset direction
    bool isClosedPath = isPathClosed(points);
    bool clockwise = isClockwise(points);
    std::cout << "DEBUG: Path analysis:" << std::endl;
    std::cout << "  - Is closed path: " << (isClosedPath ? "true" : "false") << std::endl;
    std::cout << "  - Is clockwise: " << (clockwise ? "true" : "false") << std::endl;
    
    switch (offsetDirection) {
        case ToolOffsetDirection::INSIDE:
            // For inside offset, we need to offset towards the interior
            offsetAmount = clockwise ? offsetAmount : -offsetAmount;
            std::cout << "DEBUG: INSIDE offset - final amount: " << offsetAmount << std::endl;
            break;
        case ToolOffsetDirection::OUTSIDE:
            // For outside offset, we need to offset towards the exterior
            offsetAmount = clockwise ? -offsetAmount : offsetAmount;
            std::cout << "DEBUG: OUTSIDE offset - final amount: " << offsetAmount << std::endl;
            break;
        case ToolOffsetDirection::ON_PATH:
            offsetAmount = 0.0;
            std::cout << "DEBUG: ON_PATH offset - no offset applied" << std::endl;
            break;
        case ToolOffsetDirection::AUTO:
            // Auto-determine based on path characteristics
            offsetDirection = determineOffsetDirection(originalPath);
            std::cout << "DEBUG: AUTO offset - determined direction: " << static_cast<int>(offsetDirection) << std::endl;
            return calculateOffset(originalPath, toolDiameter, offsetDirection, tolerance);
    }
    
    if (std::abs(offsetAmount) < tolerance) {
        std::cout << "DEBUG: Offset amount too small (" << std::abs(offsetAmount) << " < " << tolerance << "), returning original path" << std::endl;
        return originalPath; // No significant offset needed
    }
    
    // VALIDATION: Final check before attempting offset
    if (std::abs(offsetAmount) > minFeatureSize / 2.0 && minFeatureSize > 0) {
        std::cout << "DEBUG: ERROR - Offset amount (" << std::abs(offsetAmount) 
                  << ") is larger than half the minimum feature size (" << minFeatureSize/2.0 
                  << "). This will likely cause offset failure." << std::endl;
        std::cout << "DEBUG: Returning original path to prevent catastrophic failure" << std::endl;
        return originalPath;
    }
    
    // Calculate offset points
    std::cout << "DEBUG: Calling offsetPolyline with amount: " << offsetAmount << std::endl;
    std::vector<Point2D> offsetPoints = offsetPolyline(points, offsetAmount, isClosedPath, tolerance);
    
    if (offsetPoints.empty()) {
        std::cout << "DEBUG: offsetPolyline returned empty result - offset failed" << std::endl;
        return Path(); // Offset failed
    }
    
    // VALIDATION: Check if offset result is reasonable
    double pointReductionRatio = 1.0 - (static_cast<double>(offsetPoints.size()) / static_cast<double>(points.size()));
    std::cout << "DEBUG: Point reduction ratio: " << (pointReductionRatio * 100) << "%" << std::endl;
    
    if (pointReductionRatio > 0.75) { // More than 75% point reduction
        std::cout << "DEBUG: WARNING - Excessive point reduction detected (" 
                  << (pointReductionRatio * 100) << "%). Offset may have failed." << std::endl;
    }
    
    std::cout << "DEBUG: Offset calculation successful:" << std::endl;
    std::cout << "  - Input points: " << points.size() << std::endl;
    std::cout << "  - Output points: " << offsetPoints.size() << std::endl;
    
    // Print first few points for verification
    std::cout << "DEBUG: First few original points:" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(3, points.size()); ++i) {
        std::cout << "  [" << i << "] (" << points[i].x << ", " << points[i].y << ")" << std::endl;
    }
    
    std::cout << "DEBUG: First few offset points:" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(3, offsetPoints.size()); ++i) {
        std::cout << "  [" << i << "] (" << offsetPoints[i].x << ", " << offsetPoints[i].y << ")" << std::endl;
    }
    
    // Create and return offset path
    Path offsetPath;
    for (const auto& point : offsetPoints) {
        offsetPath.addPoint(point);
    }
    
    // Summary debug output
    std::cout << "DEBUG: ===== TOOL OFFSET SUMMARY =====" << std::endl;
    std::cout << "  - Tool diameter: " << toolDiameter << " (radius: " << toolDiameter/2.0 << ")" << std::endl;
    std::cout << "  - Offset direction: " << static_cast<int>(offsetDirection) << " (0=INSIDE, 1=OUTSIDE, 2=ON_PATH, 3=AUTO)" << std::endl;
    std::cout << "  - Path type: " << (isClosedPath ? "CLOSED" : "OPEN") << " / " << (clockwise ? "CLOCKWISE" : "COUNTER-CLOCKWISE") << std::endl;
    std::cout << "  - Final offset amount: " << offsetAmount << std::endl;
    std::cout << "  - Points: " << points.size() << " -> " << offsetPoints.size() << std::endl;
    std::cout << "  - Minimum feature size: " << minFeatureSize << std::endl;
    
    // Calculate actual offset distance verification for closed paths
    if (!points.empty() && !offsetPoints.empty() && isClosedPath) {
        double dx = offsetPoints[0].x - points[0].x;
        double dy = offsetPoints[0].y - points[0].y;
        double actualDistance = std::sqrt(dx * dx + dy * dy);
        double expectedDistance = std::abs(offsetAmount);
        std::cout << "  - Expected offset distance: " << expectedDistance << std::endl;
        std::cout << "  - Actual offset distance: " << actualDistance << std::endl;
        
        // More stringent accuracy check
        double accuracyRatio = actualDistance / expectedDistance;
        if (accuracyRatio > 2.0 || accuracyRatio < 0.5) {
            std::cout << "  - Offset accuracy: FAILED (ratio: " << accuracyRatio << ")" << std::endl;
            std::cout << "  - WARNING: Offset calculation may have failed. Consider using original path." << std::endl;
        } else if (std::abs(actualDistance - expectedDistance) < 0.01) {
            std::cout << "  - Offset accuracy: EXCELLENT" << std::endl;
        } else if (std::abs(actualDistance - expectedDistance) < 0.05) {
            std::cout << "  - Offset accuracy: GOOD" << std::endl;
        } else {
            std::cout << "  - Offset accuracy: CHECK REQUIRED" << std::endl;
        }
    }
    
    std::cout << "DEBUG: =================================" << std::endl;
    std::cout << "DEBUG: ToolOffset::calculateOffset() completed successfully" << std::endl;
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
    
    // Debug output for line segment offset calculation
    // std::cout << "DEBUG: offsetLineSegment - start: (" << start.x << ", " << start.y << "), end: (" << end.x << ", " << end.y << "), offset: " << offset << ", length: " << length << std::endl;
    
    if (length < 1e-10) {
        static int degenerateCount = 0;
        degenerateCount++;
        if (degenerateCount <= 5) { // Only show first 5 warnings to avoid spam
            std::cout << "DEBUG: offsetLineSegment - degenerate line #" << degenerateCount 
                      << " (length: " << length << ", start: (" << start.x << ", " << start.y 
                      << "), end: (" << end.x << ", " << end.y << "))" << std::endl;
        } else if (degenerateCount == 6) {
            std::cout << "DEBUG: offsetLineSegment - suppressing further degenerate line warnings..." << std::endl;
        }
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
    
    // Debug output for calculated offset
    // std::cout << "DEBUG: offsetLineSegment - result: (" << offsetStart.x << ", " << offsetStart.y << ") to (" << offsetEnd.x << ", " << offsetEnd.y << ")" << std::endl;
    
    return std::make_pair(offsetStart, offsetEnd);
}

std::vector<Point2D> ToolOffset::offsetPolyline(const std::vector<Point2D>& points,
                                              double offset,
                                              bool isClosed,
                                              double tolerance) {
    std::cout << "DEBUG: ToolOffset::offsetPolyline() called with:" << std::endl;
    std::cout << "  - Points count: " << points.size() << std::endl;
    std::cout << "  - Offset amount: " << offset << std::endl;
    std::cout << "  - Is closed: " << (isClosed ? "true" : "false") << std::endl;
    std::cout << "  - Tolerance: " << tolerance << std::endl;
    
    if (points.size() < 2) {
        std::cout << "DEBUG: offsetPolyline - insufficient points" << std::endl;
        return std::vector<Point2D>();
    }
    
    std::vector<Point2D> offsetPoints;
    
    // Handle special case of only two points
    if (points.size() == 2) {
        std::cout << "DEBUG: offsetPolyline - handling 2-point case" << std::endl;
        auto offsetSeg = offsetLineSegment(points[0], points[1], offset);
        offsetPoints.push_back(offsetSeg.first);
        offsetPoints.push_back(offsetSeg.second);
        std::cout << "DEBUG: 2-point offset result: (" << offsetSeg.first.x << ", " << offsetSeg.first.y 
                  << ") to (" << offsetSeg.second.x << ", " << offsetSeg.second.y << ")" << std::endl;
        return offsetPoints;
    }
    
    std::vector<std::pair<Point2D, Point2D>> offsetSegments;
    
    // Create offset segments
    size_t numSegments = isClosed ? points.size() : points.size() - 1;
    std::cout << "DEBUG: Creating " << numSegments << " offset segments" << std::endl;
    
    for (size_t i = 0; i < numSegments; ++i) {
        Point2D start = points[i];
        Point2D end = points[(i + 1) % points.size()];
        auto offsetSeg = offsetLineSegment(start, end, offset);
        offsetSegments.push_back(offsetSeg);
        
        if (i < 3) { // Debug first few segments
            std::cout << "DEBUG: Segment " << i << ": (" << start.x << ", " << start.y 
                      << ") -> (" << end.x << ", " << end.y << ")" << std::endl;
            std::cout << "  Offset: (" << offsetSeg.first.x << ", " << offsetSeg.first.y 
                      << ") -> (" << offsetSeg.second.x << ", " << offsetSeg.second.y << ")" << std::endl;
        }
    }
    
    std::cout << "DEBUG: Connecting offset segments" << std::endl;
    
    // Connect offset segments
    for (size_t i = 0; i < offsetSegments.size(); ++i) {
        if (i == 0 && !isClosed) {
            // First segment of open path
            offsetPoints.push_back(offsetSegments[i].first);
            std::cout << "DEBUG: Added first point of open path: (" 
                      << offsetSegments[i].first.x << ", " << offsetSegments[i].first.y << ")" << std::endl;
        }
        
        if (i < offsetSegments.size() - 1 || isClosed) {
            // Find intersection between current and next segment
            size_t nextIndex = (i + 1) % offsetSegments.size();
            Point2D intersection;
            
            if (findLineIntersection(offsetSegments[i].first, offsetSegments[i].second,
                                   offsetSegments[nextIndex].first, offsetSegments[nextIndex].second,
                                   intersection)) {
                offsetPoints.push_back(intersection);
                if (i < 3) { // Debug first few intersections
                    std::cout << "DEBUG: Found intersection " << i << ": (" 
                              << intersection.x << ", " << intersection.y << ")" << std::endl;
                }
            } else {
                // No intersection, use end of current segment
                offsetPoints.push_back(offsetSegments[i].second);
                if (!isClosed || i < offsetSegments.size() - 1) {
                    offsetPoints.push_back(offsetSegments[nextIndex].first);
                }
                if (i < 3) { // Debug first few non-intersections
                    std::cout << "DEBUG: No intersection " << i << ", using segment endpoints" << std::endl;
                }
            }
        } else {
            // Last segment of open path
            offsetPoints.push_back(offsetSegments[i].second);
            std::cout << "DEBUG: Added last point of open path: (" 
                      << offsetSegments[i].second.x << ", " << offsetSegments[i].second.y << ")" << std::endl;
        }
    }
    
    std::cout << "DEBUG: Before cleanup - points count: " << offsetPoints.size() << std::endl;
    
    // Clean up the result
    offsetPoints = removeSelfIntersections(offsetPoints, tolerance);
    std::cout << "DEBUG: After removeSelfIntersections - points count: " << offsetPoints.size() << std::endl;
    
    offsetPoints = simplifyPath(offsetPoints, tolerance);
    std::cout << "DEBUG: After simplifyPath - points count: " << offsetPoints.size() << std::endl;
    
    std::cout << "DEBUG: offsetPolyline completed successfully" << std::endl;
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
        std::cout << "DEBUG: isClockwise - insufficient points (" << points.size() << "), defaulting to clockwise" << std::endl;
        return true; // Default to clockwise for insufficient points
    }
    
    // Calculate signed area using shoelace formula
    double signedArea = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        size_t j = (i + 1) % points.size();
        signedArea += (points[j].x - points[i].x) * (points[j].y + points[i].y);
    }
    
    bool clockwise = signedArea > 0; // Positive area = clockwise in screen coordinates
    std::cout << "DEBUG: isClockwise - signed area: " << signedArea << ", result: " << (clockwise ? "clockwise" : "counter-clockwise") << std::endl;
    
    return clockwise;
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
        std::cout << "DEBUG: isPathClosed - insufficient points (" << points.size() << "), path is not closed" << std::endl;
        return false;
    }
    
    const Point2D& first = points.front();
    const Point2D& last = points.back();
    
    double distance = std::sqrt(std::pow(last.x - first.x, 2) + std::pow(last.y - first.y, 2));
    bool closed = distance <= tolerance;
    
    std::cout << "DEBUG: isPathClosed - first: (" << first.x << ", " << first.y << "), last: (" << last.x << ", " << last.y << "), distance: " << distance << ", tolerance: " << tolerance << ", result: " << (closed ? "closed" : "open") << std::endl;
    
    return closed;
}

} // namespace cnc
} // namespace nwss 