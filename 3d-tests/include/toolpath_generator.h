#ifndef TOOLPATH_GENERATOR_H
#define TOOLPATH_GENERATOR_H

#include "types.h"
#include <vector>

class ToolpathGenerator {
public:
    ToolpathGenerator();
    ~ToolpathGenerator();
    
    // Generate toolpaths for CNC machining
    std::vector<ToolpathPoint> generateToolpaths(const std::vector<Triangle>& triangles, 
                                                 const MachiningParams& params);
    
    // Generate roughing toolpath (layer-by-layer removal)
    std::vector<ToolpathPoint> generateRoughingPath(const std::vector<Triangle>& triangles,
                                                    const MachiningParams& params);
    
    // Generate finishing toolpath (surface following)
    std::vector<ToolpathPoint> generateFinishingPath(const std::vector<Triangle>& triangles,
                                                     const MachiningParams& params);
    
    // Generate safety moves
    std::vector<ToolpathPoint> generateSafetyMoves(const Point3D& from, const Point3D& to,
                                                   double safetyHeight);

private:
    // Generate raster pattern for layer
    std::vector<ToolpathPoint> generateRasterPattern(const BoundingBox& bounds, double z,
                                                     double stepover, double feedrate);
    
    // Generate contour following path
    std::vector<ToolpathPoint> generateContourPath(const std::vector<Triangle>& triangles,
                                                   double z, double feedrate);
    
    // Check if point is inside part boundary
    bool isPointInsidePart(const Point3D& point, const std::vector<Triangle>& triangles) const;
    
    // Calculate optimal stepover for tool
    double calculateStepover(const Tool& tool) const;
    
    // Sort triangles by Z-coordinate for layer processing
    std::vector<std::vector<Triangle>> sortTrianglesByLayers(const std::vector<Triangle>& triangles,
                                                            const std::vector<double>& layers) const;
    
    // Generate approach and retract moves
    std::vector<ToolpathPoint> generateApproachMove(const Point3D& target, double safetyHeight);
    std::vector<ToolpathPoint> generateRetractMove(const Point3D& current, double safetyHeight);
    
    // Optimize toolpath for efficiency
    void optimizeToolpath(std::vector<ToolpathPoint>& toolpath) const;
    
    // Remove redundant points
    void removeRedundantPoints(std::vector<ToolpathPoint>& toolpath) const;
    
    // Ray-triangle intersection test
    bool rayTriangleIntersection(const Point3D& rayOrigin, const Point3D& rayDirection,
                                const Triangle& triangle, Point3D& intersection) const;
};

#endif // TOOLPATH_GENERATOR_H