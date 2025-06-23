#define _USE_MATH_DEFINES
#include "core/tool_offset.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

namespace nwss {
namespace cnc {

// ================================
// Main Tool Offset Implementation
// ================================

ToolOffset::OffsetResult ToolOffset::calculateToolOffset(const std::vector<Path>& originalPaths,
                                                        double toolDiameter,
                                                        ToolOffsetDirection offsetDirection,
                                                        const OffsetOptions& options) {
    OffsetResult result;
    
    // Reduced debug output for efficiency
    if (originalPaths.size() <= 5) {
        std::cout << "DEBUG: ToolOffset called - paths: " << originalPaths.size() 
                  << ", diameter: " << toolDiameter << "mm" << std::endl;
    }
    
    if (originalPaths.empty()) {
        addError(result, "No input paths provided");
        return result;
    }
    
    if (toolDiameter <= 0) {
        addError(result, "Invalid tool diameter: " + std::to_string(toolDiameter));
        return result;
    }
    
    // Initialize result statistics
    result.originalPathCount = originalPaths.size();
    for (const auto& path : originalPaths) {
        result.originalTotalLength += calculatePathLength(path);
    }
    
    // Clean input paths first
    auto cleanedPaths = cleanupPaths(originalPaths, options.precision);
    if (cleanedPaths.empty()) {
        addError(result, "All input paths were invalid or degenerate");
        return result;
    }
    
    if (cleanedPaths.size() != originalPaths.size()) {
        addWarning(result, "Some input paths were removed during cleanup");
    }
    
    // Validate tool size against features
    std::vector<std::string> validationWarnings;
    if (!validateToolForPaths(cleanedPaths, toolDiameter, validationWarnings)) {
        for (const auto& warning : validationWarnings) {
            addWarning(result, warning);
        }
    }
    
    // Convert paths to Clipper2 format
    auto clipperPaths = pathsToClipper(cleanedPaths, options.scaleFactor);
    if (clipperPaths.empty()) {
        addError(result, "Failed to convert paths to Clipper2 format");
        return result;
    }
    
    // Reduced debug output
    
    // Calculate offset amount
    double offsetAmount = 0.0;
    bool needsOffset = true;
    
    switch (offsetDirection) {
        case ToolOffsetDirection::INSIDE:
            offsetAmount = -(toolDiameter / 2.0);
            break;
        case ToolOffsetDirection::OUTSIDE:
            offsetAmount = toolDiameter / 2.0;
            break;
        case ToolOffsetDirection::ON_PATH:
            offsetAmount = 0.0;
            needsOffset = false;
            break;
        case ToolOffsetDirection::AUTO:
            offsetDirection = determineOptimalOffsetDirection(cleanedPaths);
            return calculateToolOffset(originalPaths, toolDiameter, offsetDirection, options);
    }
    
    // Apply offset using Clipper2
    Clipper2Lib::Paths64 offsetPaths;
    
    if (!needsOffset) {
        // No offset needed - just return cleaned paths
        offsetPaths = clipperPaths;
    } else {
        // Set up Clipper offset
        Clipper2Lib::ClipperOffset clipperOffset(options.miterLimit, options.arcTolerance * options.scaleFactor);
        
        // Add paths with appropriate join/end types
        for (const auto& path : clipperPaths) {
            bool isClosed = isPathClosed(clipperToPath(path, options.scaleFactor));
            Clipper2Lib::JoinType joinType = getJoinType(options);
            Clipper2Lib::EndType endType = getEndType(isClosed);
            
            clipperOffset.AddPath(path, joinType, endType);
        }
        
        // Execute offset
        double scaledOffset = offsetAmount * options.scaleFactor;
        
        try {
            clipperOffset.Execute(scaledOffset, offsetPaths);
        } catch (const std::exception& e) {
            addError(result, "Clipper2 offset execution failed: " + std::string(e.what()));
            return result;
        }
    }
    
    // Convert back to Path objects
    auto resultPaths = clipperToPaths(offsetPaths, options.scaleFactor);
    
    // Filter out degenerate results
    std::vector<Path> validPaths;
    for (const auto& path : resultPaths) {
        if (hasValidGeometry(path)) {
            double pathLength = calculatePathLength(path);
            double pathArea = 0.0;
            
            // Calculate area for closed paths
            if (isPathClosed(path)) {
                const auto& points = path.getPoints();
                if (points.size() >= 3) {
                    // Shoelace formula
                    double area = 0.0;
                    for (size_t i = 0; i < points.size(); ++i) {
                        size_t j = (i + 1) % points.size();
                        area += points[i].x * points[j].y - points[j].x * points[i].y;
                    }
                    pathArea = std::abs(area) / 2.0;
                }
            }
            
            // Check minimum feature size
            bool isValid = true;
            if (pathLength < options.minFeatureSize) {
                isValid = false;
            }
            
            if (isPathClosed(path) && pathArea < (options.minFeatureSize * options.minFeatureSize)) {
                isValid = false;
            }
            
            if (isValid) {
                validPaths.push_back(path);
                result.resultTotalLength += pathLength;
            }
        }
    }
    
    result.paths = validPaths;
    result.resultPathCount = validPaths.size();
    result.success = !validPaths.empty();
    
    if (needsOffset && !validPaths.empty()) {
        // Calculate actual offset distance for validation
        if (!cleanedPaths.empty() && !validPaths.empty()) {
            result.actualOffsetDistance = calculateActualOffset(cleanedPaths[0], validPaths[0]);
        }
    }
    
    // Validate results if requested
    if (options.validateResults) {
        auto validation = validateOffsetResult(cleanedPaths, validPaths, offsetAmount, options);
        result.warnings.insert(result.warnings.end(), validation.warnings.begin(), validation.warnings.end());
        result.errors.insert(result.errors.end(), validation.errors.begin(), validation.errors.end());
        
        // Copy validation metrics
        result.minFeatureSize = validation.minFeatureSize;
        result.maxFeatureSize = validation.maxFeatureSize;
        result.hasDegenerate = validation.hasDegenerate;
        result.hasSelfIntersections = validation.hasSelfIntersections;
    }
    
    // Only show summary for small numbers of paths to avoid flooding
    if (result.success && originalPaths.size() <= 5) {
        std::cout << "DEBUG: Tool offset completed - " << result.originalPathCount 
                  << " -> " << result.resultPathCount << " paths" << std::endl;
    } else if (!result.success) {
        std::cout << "DEBUG: Tool offset failed with " << result.errors.size() << " errors" << std::endl;
    }
    
    return result;
}

ToolOffset::OffsetResult ToolOffset::calculateToolOffset(const Path& originalPath,
                                                        double toolDiameter,
                                                        ToolOffsetDirection offsetDirection,
                                                        const OffsetOptions& options) {
    return calculateToolOffset(std::vector<Path>{originalPath}, toolDiameter, offsetDirection, options);
}

std::vector<ToolOffset::OffsetResult> ToolOffset::calculateMultipleOffsets(
    const std::vector<Path>& originalPaths,
    double toolDiameter,
    const std::vector<double>& offsetDistances,
    const OffsetOptions& options) {
    
    std::vector<OffsetResult> results;
    results.reserve(offsetDistances.size());
    
    for (double distance : offsetDistances) {
        ToolOffsetDirection direction = (distance >= 0) ? ToolOffsetDirection::OUTSIDE : ToolOffsetDirection::INSIDE;
        double actualDiameter = std::abs(distance) * 2.0; // Convert offset to diameter
        
        auto result = calculateToolOffset(originalPaths, actualDiameter, direction, options);
        results.push_back(result);
    }
    
    return results;
}

// ================================
// Validation and Analysis Methods
// ================================

bool ToolOffset::validateToolForPaths(const std::vector<Path>& paths,
                                     double toolDiameter,
                                     std::vector<std::string>& warnings) {
    warnings.clear();
    bool allValid = true;
    
    double minFeatureSize = calculateMinimumFeatureSize(paths);
    
    if (minFeatureSize > 0 && toolDiameter > minFeatureSize) {
        warnings.push_back("Tool diameter (" + std::to_string(toolDiameter) + 
                          "mm) is larger than minimum feature size (" + 
                          std::to_string(minFeatureSize) + "mm)");
        allValid = false;
    }
    
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];
        
        if (path.size() < 2) {
            warnings.push_back("Path " + std::to_string(i + 1) + " has insufficient points");
            allValid = false;
            continue;
        }
        
        if (hasSelfIntersections(path)) {
            warnings.push_back("Path " + std::to_string(i + 1) + " has self-intersections");
        }
    }
    
    return allValid;
}

ToolOffsetDirection ToolOffset::determineOptimalOffsetDirection(const std::vector<Path>& paths) {
    if (paths.empty()) {
        return ToolOffsetDirection::ON_PATH;
    }
    
    // Check if we have mostly closed paths
    int closedCount = 0;
    int clockwiseCount = 0;
    
    for (const auto& path : paths) {
        if (isPathClosed(path)) {
            closedCount++;
            if (isClockwise(path)) {
                clockwiseCount++;
            }
        }
    }
    
    if (closedCount == 0) {
        // All open paths - use center-line cutting
        return ToolOffsetDirection::ON_PATH;
    }
    
    // For closed paths, use direction based on winding
    // Convention: clockwise = outer boundary (cut outside), counter-clockwise = hole (cut inside)
    if (clockwiseCount > closedCount / 2) {
        return ToolOffsetDirection::OUTSIDE;
    } else {
        return ToolOffsetDirection::INSIDE;
    }
}

double ToolOffset::calculateMinimumFeatureSize(const std::vector<Path>& paths) {
    if (paths.empty()) {
        return 0.0;
    }
    
    double minSize = std::numeric_limits<double>::max();
    
    for (const auto& path : paths) {
        const auto& points = path.getPoints();
        if (points.size() < 2) continue;
        
        // Calculate minimum segment length
        for (size_t i = 0; i < points.size() - 1; ++i) {
            double dx = points[i + 1].x - points[i].x;
            double dy = points[i + 1].y - points[i].y;
            double length = std::sqrt(dx * dx + dy * dy);
            if (length > 0.001) { // Ignore tiny segments
                minSize = std::min(minSize, length);
            }
        }
        
        // For closed paths, also check minimum distance between non-adjacent segments
        if (isPathClosed(path) && points.size() >= 4) {
            for (size_t i = 0; i < points.size() - 1; ++i) {
                for (size_t j = i + 2; j < points.size() - 1; ++j) {
                    if (j == points.size() - 2 && i == 0) continue; // Skip adjacent
                    
                    double dx = points[i].x - points[j].x;
                    double dy = points[i].y - points[j].y;
                    double dist = std::sqrt(dx * dx + dy * dy);
                    if (dist > 0.001) {
                        minSize = std::min(minSize, dist);
                    }
                }
            }
        }
    }
    
    return (minSize == std::numeric_limits<double>::max()) ? 0.0 : minSize;
}

bool ToolOffset::hasFeaturesTooSmallForTool(const std::vector<Path>& paths, double toolDiameter) {
    double minFeatureSize = calculateMinimumFeatureSize(paths);
    return minFeatureSize > 0 && toolDiameter > minFeatureSize;
}

// ================================
// Path Cleanup and Simplification
// ================================

std::vector<Path> ToolOffset::cleanupPaths(const std::vector<Path>& paths, double tolerance) {
    std::vector<Path> cleanedPaths;
    
    for (const auto& path : paths) {
        if (!hasValidGeometry(path)) {
            continue;
        }
        
        const auto& points = path.getPoints();
        if (points.size() < 2) {
            continue;
        }
        
        // Remove duplicate and very close points
        std::vector<Point2D> cleanedPoints;
        cleanedPoints.push_back(points[0]);
        
        for (size_t i = 1; i < points.size(); ++i) {
            double dx = points[i].x - cleanedPoints.back().x;
            double dy = points[i].y - cleanedPoints.back().y;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist > tolerance) {
                cleanedPoints.push_back(points[i]);
            }
        }
        
        if (cleanedPoints.size() >= 2) {
            Path cleanedPath;
            for (const auto& point : cleanedPoints) {
                cleanedPath.addPoint(point);
            }
            cleanedPaths.push_back(cleanedPath);
        }
    }
    
    return cleanedPaths;
}

std::vector<Path> ToolOffset::simplifyPaths(const std::vector<Path>& paths, double tolerance) {
    std::vector<Path> simplifiedPaths;
    
    for (const auto& path : paths) {
        const auto& points = path.getPoints();
        if (points.size() < 3) {
            simplifiedPaths.push_back(path);
            continue;
        }
        
        // Douglas-Peucker simplification
        std::vector<Point2D> simplified;
        simplified.push_back(points[0]);
        
        for (size_t i = 1; i < points.size() - 1; ++i) {
            const Point2D& prev = points[i - 1];
            const Point2D& curr = points[i];
            const Point2D& next = points[i + 1];
            
            // Calculate perpendicular distance from current point to line prev-next
            double A = next.y - prev.y;
            double B = prev.x - next.x;
            double C = next.x * prev.y - prev.x * next.y;
            
            double distance = std::abs(A * curr.x + B * curr.y + C) / std::sqrt(A * A + B * B);
            
            if (distance > tolerance) {
                simplified.push_back(curr);
            }
        }
        
        simplified.push_back(points.back());
        
        if (simplified.size() >= 2) {
            Path simplifiedPath;
            for (const auto& point : simplified) {
                simplifiedPath.addPoint(point);
            }
            simplifiedPaths.push_back(simplifiedPath);
        }
    }
    
    return simplifiedPaths;
}

// ================================
// Clipper2 Conversion Methods
// ================================

Clipper2Lib::Paths64 ToolOffset::pathsToClipper(const std::vector<Path>& paths, int scaleFactor) {
    Clipper2Lib::Paths64 clipperPaths;
    clipperPaths.reserve(paths.size());
    
    for (const auto& path : paths) {
        auto clipperPath = pathToClipper(path, scaleFactor);
        if (!clipperPath.empty()) {
            clipperPaths.push_back(clipperPath);
        }
    }
    
    return clipperPaths;
}

std::vector<Path> ToolOffset::clipperToPaths(const Clipper2Lib::Paths64& clipperPaths, int scaleFactor) {
    std::vector<Path> paths;
    paths.reserve(clipperPaths.size());
    
    for (const auto& clipperPath : clipperPaths) {
        auto path = clipperToPath(clipperPath, scaleFactor);
        if (!path.empty()) {
            paths.push_back(path);
        }
    }
    
    return paths;
}

Clipper2Lib::Path64 ToolOffset::pathToClipper(const Path& path, int scaleFactor) {
    Clipper2Lib::Path64 clipperPath;
    const auto& points = path.getPoints();
    
    if (points.empty()) {
        return clipperPath;
    }
    
    clipperPath.reserve(points.size());
    double scale = static_cast<double>(scaleFactor);
    
    for (const auto& point : points) {
        // Validate input coordinates
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
            continue;
        }
        
        // Scale and round to integer coordinates
        int64_t x = static_cast<int64_t>(std::round(point.x * scale));
        int64_t y = static_cast<int64_t>(std::round(point.y * scale));
        
        clipperPath.push_back(Clipper2Lib::Point64(x, y));
    }
    
    return clipperPath;
}

Path ToolOffset::clipperToPath(const Clipper2Lib::Path64& clipperPath, int scaleFactor) {
    Path path;
    if (clipperPath.empty()) {
        return path;
    }
    
    double scale = 1.0 / static_cast<double>(scaleFactor);
    
    for (const auto& point : clipperPath) {
        double x = static_cast<double>(point.x) * scale;
        double y = static_cast<double>(point.y) * scale;
        
        // Validate coordinates
        if (std::isfinite(x) && std::isfinite(y)) {
            path.addPoint(Point2D(x, y));
        }
    }
    
    return path;
}

// ================================
// Helper Methods
// ================================

double ToolOffset::calculateOffsetAmount(double toolDiameter, ToolOffsetDirection direction, bool isClockwise) {
    double radius = toolDiameter / 2.0;
    
    switch (direction) {
        case ToolOffsetDirection::INSIDE:
            return isClockwise ? -radius : radius;
        case ToolOffsetDirection::OUTSIDE:
            return isClockwise ? radius : -radius;
        case ToolOffsetDirection::ON_PATH:
            return 0.0;
        case ToolOffsetDirection::AUTO:
            return 0.0; // Should be handled by caller
    }
    
    return 0.0;
}

Clipper2Lib::JoinType ToolOffset::getJoinType(const OffsetOptions& options) {
    // Use Round joins for best quality
    return Clipper2Lib::JoinType::Round;
}

Clipper2Lib::EndType ToolOffset::getEndType(bool isClosedPath) {
    return isClosedPath ? Clipper2Lib::EndType::Polygon : Clipper2Lib::EndType::Round;
}

bool ToolOffset::isPathClosed(const Path& path, double tolerance) {
    const auto& points = path.getPoints();
    if (points.size() < 3) {
        return false;
    }
    
    double dx = points.back().x - points.front().x;
    double dy = points.back().y - points.front().y;
    double distance = std::sqrt(dx * dx + dy * dy);
    
    return distance <= tolerance;
}

bool ToolOffset::isClockwise(const Path& path) {
    const auto& points = path.getPoints();
    if (points.size() < 3) {
        return true; // Default
    }
    
    // Calculate signed area using shoelace formula
    double signedArea = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        size_t j = (i + 1) % points.size();
        signedArea += (points[j].x - points[i].x) * (points[j].y + points[i].y);
    }
    
    return signedArea > 0; // Positive area = clockwise in screen coordinates
}

// ================================
// Validation Methods
// ================================

ToolOffset::OffsetResult ToolOffset::validateOffsetResult(const std::vector<Path>& originalPaths,
                                                         const std::vector<Path>& offsetPaths,
                                                         double expectedOffset,
                                                         const OffsetOptions& options) {
    OffsetResult validation;
    
    // Basic metrics
    validation.originalPathCount = originalPaths.size();
    validation.resultPathCount = offsetPaths.size();
    
    for (const auto& path : originalPaths) {
        validation.originalTotalLength += calculatePathLength(path);
    }
    
    for (const auto& path : offsetPaths) {
        validation.resultTotalLength += calculatePathLength(path);
    }
    
    // Feature size analysis
    if (!offsetPaths.empty()) {
        double minSize = calculateMinimumFeatureSize(offsetPaths);
        validation.minFeatureSize = minSize;
        validation.maxFeatureSize = minSize; // Simplified for now
        
        if (minSize < options.minFeatureSize) {
            addWarning(validation, "Result contains features smaller than minimum size");
        }
    }
    
    // Check for degenerate paths
    for (const auto& path : offsetPaths) {
        if (!hasValidGeometry(path)) {
            validation.hasDegenerate = true;
            addWarning(validation, "Result contains degenerate geometry");
            break;
        }
    }
    
    // Check for self-intersections
    for (const auto& path : offsetPaths) {
        if (hasSelfIntersections(path)) {
            validation.hasSelfIntersections = true;
            addWarning(validation, "Result contains self-intersections");
            break;
        }
    }
    
    validation.success = validation.errors.empty();
    return validation;
}

double ToolOffset::calculatePathLength(const Path& path) {
    const auto& points = path.getPoints();
    if (points.size() < 2) {
        return 0.0;
    }
    
    double length = 0.0;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        double dx = points[i + 1].x - points[i].x;
        double dy = points[i + 1].y - points[i].y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    
    return length;
}

double ToolOffset::calculateActualOffset(const Path& original, const Path& offset) {
    if (original.empty() || offset.empty()) {
        return 0.0;
    }
    
    const auto& origPoints = original.getPoints();
    const auto& offsetPoints = offset.getPoints();
    
    if (origPoints.empty() || offsetPoints.empty()) {
        return 0.0;
    }
    
    // For closed paths, calculate the actual offset by finding the minimum distance
    // between any point on the original path and the offset path
    double minDistance = std::numeric_limits<double>::max();
    
    // Sample a few points to estimate the offset
    size_t sampleCount = std::min<size_t>(5, origPoints.size());
    for (size_t i = 0; i < sampleCount; ++i) {
        size_t origIndex = (i * origPoints.size()) / sampleCount;
        const Point2D& origPoint = origPoints[origIndex];
        
        // Find closest point on offset path
        double closestDist = std::numeric_limits<double>::max();
        for (const auto& offsetPoint : offsetPoints) {
            double dx = offsetPoint.x - origPoint.x;
            double dy = offsetPoint.y - origPoint.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            closestDist = std::min(closestDist, dist);
        }
        
        minDistance = std::min(minDistance, closestDist);
    }
    
    return (minDistance == std::numeric_limits<double>::max()) ? 0.0 : minDistance;
}

bool ToolOffset::hasValidGeometry(const Path& path) {
    const auto& points = path.getPoints();
    if (points.size() < 2) {
        return false;
    }
    
    // Check for NaN or infinite coordinates
    for (const auto& point : points) {
        if (std::isnan(point.x) || std::isnan(point.y) || 
            std::isinf(point.x) || std::isinf(point.y)) {
            return false;
        }
    }
    
    // Check minimum path length
    double length = calculatePathLength(path);
    return length > 0.001; // Minimum 1 micron
}

bool ToolOffset::hasSelfIntersections(const Path& path) {
    const auto& points = path.getPoints();
    if (points.size() < 4) {
        return false; // Need at least 4 points to self-intersect
    }
    
    // Simple check for self-intersections (could be more sophisticated)
    for (size_t i = 0; i < points.size() - 1; ++i) {
        for (size_t j = i + 2; j < points.size() - 1; ++j) {
            if (j == points.size() - 2 && i == 0) continue; // Skip adjacent segments
            
            // Check if segments i and j intersect
            // This is a simplified check - a full implementation would use line-line intersection
            const Point2D& p1 = points[i];
            const Point2D& p2 = points[i + 1];
            const Point2D& p3 = points[j];
            const Point2D& p4 = points[j + 1];
            
            // Simple bounding box check
            double minX1 = std::min(p1.x, p2.x), maxX1 = std::max(p1.x, p2.x);
            double minY1 = std::min(p1.y, p2.y), maxY1 = std::max(p1.y, p2.y);
            double minX2 = std::min(p3.x, p4.x), maxX2 = std::max(p3.x, p4.x);
            double minY2 = std::min(p3.y, p4.y), maxY2 = std::max(p3.y, p4.y);
            
            if (maxX1 >= minX2 && maxX2 >= minX1 && maxY1 >= minY2 && maxY2 >= minY1) {
                // Bounding boxes overlap - potential intersection
                return true;
            }
        }
    }
    
    return false;
}

void ToolOffset::addWarning(OffsetResult& result, const std::string& message) {
    result.warnings.push_back(message);
}

void ToolOffset::addError(OffsetResult& result, const std::string& message) {
    result.errors.push_back(message);
    result.success = false;
}

// ================================
// Legacy Compatibility Layer
// ================================

namespace legacy {
    Path calculateOffset(const Path& originalPath, 
                        double toolDiameter,
                        ToolOffsetDirection offsetDirection,
                        double tolerance) {
        ToolOffset::OffsetOptions options;
        options.precision = tolerance;
        
        auto result = ToolOffset::calculateToolOffset(originalPath, toolDiameter, offsetDirection, options);
        return result.success && !result.paths.empty() ? result.paths[0] : Path();
    }
    
    Path calculateHighPrecisionOffset(const Path& originalPath,
                                    double toolDiameter,
                                    ToolOffsetDirection offsetDirection,
                                                       double tolerance) {
        ToolOffset::OffsetOptions options;
        options.precision = tolerance;
        options.scaleFactor = 10000; // Higher precision
        
        auto result = ToolOffset::calculateToolOffset(originalPath, toolDiameter, offsetDirection, options);
        return result.success && !result.paths.empty() ? result.paths[0] : Path();
    }
    
    std::vector<Path> calculateMultipleOffsets(const Path& originalPath,
                                             const std::vector<double>& offsets,
                                            double tolerance) {
        std::vector<Path> results;
        ToolOffset::OffsetOptions options;
        options.precision = tolerance;
        
        for (double offset : offsets) {
            ToolOffsetDirection direction = (offset >= 0) ? ToolOffsetDirection::OUTSIDE : ToolOffsetDirection::INSIDE;
            auto result = ToolOffset::calculateToolOffset(originalPath, std::abs(offset) * 2.0, direction, options);
            if (result.success && !result.paths.empty()) {
                results.push_back(result.paths[0]);
            } else {
                results.push_back(Path()); // Empty path for failed offsets
            }
        }
        
        return results;
    }
}

} // namespace cnc
} // namespace nwss 