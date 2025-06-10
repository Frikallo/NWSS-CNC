#ifndef GEOMETRY_ANALYZER_H
#define GEOMETRY_ANALYZER_H

#include "types.h"
#include <set>
#include <map>

class GeometryAnalyzer {
public:
    GeometryAnalyzer();
    ~GeometryAnalyzer();
    
    // Analyze mesh geometry for CNC machining feasibility
    ValidationResult analyzeMesh(const std::vector<Triangle>& triangles, const MachiningParams& params);
    
    // Check for undercuts (overhanging geometry)
    bool hasUndercuts(const std::vector<Triangle>& triangles, std::vector<Triangle>& undercutTriangles);
    
    // Validate tool access to all surfaces
    bool validateToolAccess(const std::vector<Triangle>& triangles, const Tool& tool, std::vector<Triangle>& inaccessibleTriangles);
    
    // Check draft angles
    bool checkDraftAngles(const std::vector<Triangle>& triangles, double minDraftAngle, std::vector<Triangle>& invalidTriangles);
    
    // Validate material depth requirements
    bool validateMaterialDepth(const std::vector<Triangle>& triangles, const Material& material);
    
    // Check stepdown validation
    bool validateStepdown(const std::vector<Triangle>& triangles, double stepdown, std::vector<double>& invalidSteps);
    
    // Get machining layers for stepdown analysis
    std::vector<double> getMachiningLayers(const std::vector<Triangle>& triangles, double stepdown);
    
    // Check if triangle is facing upward (can be machined from top)
    bool isUpwardFacing(const Triangle& triangle, double tolerance = 0.1) const;
    
    // Calculate draft angle for a triangle
    double calculateDraftAngle(const Triangle& triangle) const;
    
    // Check if tool can reach triangle surface
    bool canToolReach(const Triangle& triangle, const Tool& tool, const std::vector<Triangle>& allTriangles) const;

private:
    // Helper functions
    bool rayTriangleIntersection(const Point3D& rayOrigin, const Point3D& rayDirection, 
                                const Triangle& triangle, Point3D& intersection) const;
    
    double triangleZRange(const Triangle& triangle) const;
    
    bool isPointInTriangle(const Point3D& point, const Triangle& triangle) const;
    
    // Calculate minimum distance from point to triangle
    double pointToTriangleDistance(const Point3D& point, const Triangle& triangle) const;
    
    // Check for collisions along tool path
    bool hasToolCollision(const Point3D& toolPosition, const Tool& tool, 
                         const std::vector<Triangle>& triangles, const Triangle& targetTriangle) const;
};

#endif // GEOMETRY_ANALYZER_H