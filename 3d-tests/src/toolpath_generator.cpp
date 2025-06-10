#include "toolpath_generator.h"
#include "geometry_analyzer.h"
#include <algorithm>
#include <iostream>

ToolpathGenerator::ToolpathGenerator() {}

ToolpathGenerator::~ToolpathGenerator() {}

std::vector<ToolpathPoint> ToolpathGenerator::generateToolpaths(const std::vector<Triangle>& triangles, 
                                                               const MachiningParams& params) {
    std::vector<ToolpathPoint> allPaths;
    
    std::cout << "\n=== Generating Toolpaths ===" << std::endl;
    
    // Generate roughing passes
    std::cout << "Generating roughing toolpaths..." << std::endl;
    std::vector<ToolpathPoint> roughingPaths = generateRoughingPath(triangles, params);
    allPaths.insert(allPaths.end(), roughingPaths.begin(), roughingPaths.end());
    
    // Generate finishing passes
    std::cout << "Generating finishing toolpaths..." << std::endl;
    std::vector<ToolpathPoint> finishingPaths = generateFinishingPath(triangles, params);
    allPaths.insert(allPaths.end(), finishingPaths.begin(), finishingPaths.end());
    
    // Optimize toolpath
    std::cout << "Optimizing toolpaths..." << std::endl;
    optimizeToolpath(allPaths);
    
    std::cout << "Generated " << allPaths.size() << " toolpath points" << std::endl;
    std::cout << "===========================\n" << std::endl;
    
    return allPaths;
}

std::vector<ToolpathPoint> ToolpathGenerator::generateRoughingPath(const std::vector<Triangle>& triangles,
                                                                  const MachiningParams& params) {
    std::vector<ToolpathPoint> path;
    
    if (triangles.empty()) return path;
    
    // Calculate bounding box
    BoundingBox bounds;
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            bounds.update(triangle.vertices[i]);
        }
    }
    
    // Generate machining layers
    GeometryAnalyzer analyzer;
    std::vector<double> layers = analyzer.getMachiningLayers(triangles, params.tool.stepdown);
    
    double stepover = calculateStepover(params.tool);
    
    // Start with safety move to first position
    Point3D startPos(bounds.min.x, bounds.min.y, bounds.max.z + params.safetyHeight);
    path.push_back(ToolpathPoint(startPos, 0, true)); // Rapid move
    
    // Process each layer from top to bottom
    for (size_t i = 0; i < layers.size() - 1; ++i) {
        double layerZ = layers[i+1]; // Start from the lower layer
        
        // Generate raster pattern for this layer
        std::vector<ToolpathPoint> layerPath = generateRasterPattern(bounds, layerZ, stepover, params.tool.feedrate);
        
        // Filter points that are inside the part
        std::vector<ToolpathPoint> filteredPath;
        for (const auto& point : layerPath) {
            if (isPointInsidePart(point.position, triangles)) {
                filteredPath.push_back(point);
            }
        }
        
        // Add approach move to start of layer
        if (!filteredPath.empty()) {
            Point3D approachPoint = filteredPath[0].position + Point3D(0, 0, params.retractHeight);
            path.push_back(ToolpathPoint(approachPoint, 0, true)); // Rapid
            path.insert(path.end(), filteredPath.begin(), filteredPath.end());
            
            // Add retract move at end of layer
            Point3D retractPoint = filteredPath.back().position + Point3D(0, 0, params.retractHeight);
            path.push_back(ToolpathPoint(retractPoint, 0, true)); // Rapid
        }
    }
    
    return path;
}

std::vector<ToolpathPoint> ToolpathGenerator::generateFinishingPath(const std::vector<Triangle>& triangles,
                                                                   const MachiningParams& params) {
    std::vector<ToolpathPoint> path;
    
    if (triangles.empty()) return path;
    
    // For finishing, follow the actual surface geometry
    // Simplified approach: create contour paths at fine Z increments
    
    BoundingBox bounds;
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            bounds.update(triangle.vertices[i]);
        }
    }
    
    double finishStepdown = params.tool.stepdown * 0.1; // Fine stepdown for finishing
    double finishFeedrate = params.tool.feedrate * 0.5; // Slower feed for finishing
    
    // Generate fine layers for finishing
    std::vector<double> finishLayers;
    for (double z = bounds.max.z; z >= bounds.min.z; z -= finishStepdown) {
        finishLayers.push_back(z);
    }
    
    // Generate contour paths for each layer
    for (double z : finishLayers) {
        std::vector<ToolpathPoint> contourPath = generateContourPath(triangles, z, finishFeedrate);
        
        if (!contourPath.empty()) {
            // Add approach move
            Point3D approachPoint = contourPath[0].position + Point3D(0, 0, params.retractHeight);
            path.push_back(ToolpathPoint(approachPoint, 0, true));
            
            path.insert(path.end(), contourPath.begin(), contourPath.end());
            
            // Add retract move
            Point3D retractPoint = contourPath.back().position + Point3D(0, 0, params.retractHeight);
            path.push_back(ToolpathPoint(retractPoint, 0, true));
        }
    }
    
    return path;
}

std::vector<ToolpathPoint> ToolpathGenerator::generateSafetyMoves(const Point3D& from, const Point3D& to,
                                                                 double safetyHeight) {
    std::vector<ToolpathPoint> moves;
    
    // Move to safety height above 'from'
    Point3D safeFrom(from.x, from.y, from.z + safetyHeight);
    moves.push_back(ToolpathPoint(safeFrom, 0, true));
    
    // Move to safety height above 'to'
    Point3D safeTo(to.x, to.y, to.z + safetyHeight);
    moves.push_back(ToolpathPoint(safeTo, 0, true));
    
    // Move down to 'to'
    moves.push_back(ToolpathPoint(to, 0, false));
    
    return moves;
}

std::vector<ToolpathPoint> ToolpathGenerator::generateRasterPattern(const BoundingBox& bounds, double z,
                                                                   double stepover, double feedrate) {
    std::vector<ToolpathPoint> pattern;
    
    bool leftToRight = true;
    
    for (double y = bounds.min.y; y <= bounds.max.y; y += stepover) {
        if (leftToRight) {
            // Left to right pass
            for (double x = bounds.min.x; x <= bounds.max.x; x += stepover * 0.1) {
                pattern.push_back(ToolpathPoint(Point3D(x, y, z), feedrate, false));
            }
        } else {
            // Right to left pass
            for (double x = bounds.max.x; x >= bounds.min.x; x -= stepover * 0.1) {
                pattern.push_back(ToolpathPoint(Point3D(x, y, z), feedrate, false));
            }
        }
        leftToRight = !leftToRight;
    }
    
    return pattern;
}

std::vector<ToolpathPoint> ToolpathGenerator::generateContourPath(const std::vector<Triangle>& triangles,
                                                                 double z, double feedrate) {
    std::vector<ToolpathPoint> contour;
    
    // Simplified contour generation: find triangle intersections at Z height
    // This is a basic implementation - real CAM software uses sophisticated algorithms
    
    for (const auto& triangle : triangles) {
        // Check if triangle intersects with Z plane
        double minZ = std::min({triangle.vertices[0].z, triangle.vertices[1].z, triangle.vertices[2].z});
        double maxZ = std::max({triangle.vertices[0].z, triangle.vertices[1].z, triangle.vertices[2].z});
        
        if (z >= minZ && z <= maxZ) {
            // Find intersection points
            std::vector<Point3D> intersections;
            
            for (int i = 0; i < 3; ++i) {
                int next = (i + 1) % 3;
                Point3D v1 = triangle.vertices[i];
                Point3D v2 = triangle.vertices[next];
                
                // Check if edge crosses Z plane
                if ((v1.z <= z && v2.z >= z) || (v1.z >= z && v2.z <= z)) {
                    if (std::abs(v2.z - v1.z) > 1e-9) { // Avoid division by zero
                        double t = (z - v1.z) / (v2.z - v1.z);
                        Point3D intersection = v1 + (v2 - v1) * t;
                        intersections.push_back(intersection);
                    }
                }
            }
            
            // Add intersection points to contour
            for (const auto& point : intersections) {
                contour.push_back(ToolpathPoint(point, feedrate, false));
            }
        }
    }
    
    return contour;
}

bool ToolpathGenerator::isPointInsidePart(const Point3D& point, const std::vector<Triangle>& triangles) const {
    // Simplified inside test using ray casting from above
    Point3D rayOrigin(point.x, point.y, point.z + 1000); // Start well above
    Point3D rayDirection(0, 0, -1); // Downward ray
    
    int intersectionCount = 0;
    
    for (const auto& triangle : triangles) {
        Point3D intersection;
        if (rayTriangleIntersection(rayOrigin, rayDirection, triangle, intersection)) {
            if (intersection.z >= point.z) { // Only count intersections above the point
                intersectionCount++;
            }
        }
    }
    
    return (intersectionCount % 2) == 1; // Odd number of intersections = inside
}

bool ToolpathGenerator::rayTriangleIntersection(const Point3D& rayOrigin, const Point3D& rayDirection,
                                               const Triangle& triangle, Point3D& intersection) const {
    const double EPSILON = 1e-9;
    
    Point3D edge1 = triangle.vertices[1] - triangle.vertices[0];
    Point3D edge2 = triangle.vertices[2] - triangle.vertices[0];
    
    Point3D h = rayDirection.cross(edge2);
    double a = edge1.dot(h);
    
    if (a > -EPSILON && a < EPSILON) {
        return false; // Ray is parallel to triangle
    }
    
    double f = 1.0 / a;
    Point3D s = rayOrigin - triangle.vertices[0];
    double u = f * s.dot(h);
    
    if (u < 0.0 || u > 1.0) {
        return false;
    }
    
    Point3D q = s.cross(edge1);
    double v = f * rayDirection.dot(q);
    
    if (v < 0.0 || u + v > 1.0) {
        return false;
    }
    
    double t = f * edge2.dot(q);
    
    if (t > EPSILON) {
        intersection = rayOrigin + rayDirection * t;
        return true;
    }
    
    return false;
}

double ToolpathGenerator::calculateStepover(const Tool& tool) const {
    // Typical stepover is 60-80% of tool diameter for roughing
    return tool.diameter * 0.7;
}

void ToolpathGenerator::optimizeToolpath(std::vector<ToolpathPoint>& toolpath) const {
    removeRedundantPoints(toolpath);
    // Additional optimizations could be added here
}

void ToolpathGenerator::removeRedundantPoints(std::vector<ToolpathPoint>& toolpath) const {
    if (toolpath.size() < 3) return;
    
    std::vector<ToolpathPoint> optimized;
    optimized.push_back(toolpath[0]);
    
    const double TOLERANCE = 0.001; // 1 micron tolerance
    
    for (size_t i = 1; i < toolpath.size() - 1; ++i) {
        Point3D prev = toolpath[i-1].position;
        Point3D curr = toolpath[i].position;
        Point3D next = toolpath[i+1].position;
        
        // Check if current point is collinear with previous and next
        Point3D v1 = curr - prev;
        Point3D v2 = next - curr;
        
        Point3D cross = v1.cross(v2);
        if (cross.magnitude() > TOLERANCE) {
            optimized.push_back(toolpath[i]);
        }
    }
    
    optimized.push_back(toolpath.back());
    toolpath = optimized;
}