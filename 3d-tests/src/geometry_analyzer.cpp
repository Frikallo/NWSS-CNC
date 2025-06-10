#include "geometry_analyzer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

GeometryAnalyzer::GeometryAnalyzer() {}

GeometryAnalyzer::~GeometryAnalyzer() {}

ValidationResult GeometryAnalyzer::analyzeMesh(const std::vector<Triangle>& triangles, const MachiningParams& params) {
    ValidationResult result;
    
    std::cout << "\n=== Geometry Analysis ===" << std::endl;
    
    // 1. Check for undercuts
    std::vector<Triangle> undercutTriangles;
    if (hasUndercuts(triangles, undercutTriangles)) {
        result.addError("Undercuts detected: " + std::to_string(undercutTriangles.size()) + " triangles have overhanging geometry");
        std::cout << "❌ Undercuts detected: " << undercutTriangles.size() << " problematic triangles" << std::endl;
    } else {
        std::cout << "✅ No undercuts detected" << std::endl;
    }
    
    // 2. Validate tool access
    std::vector<Triangle> inaccessibleTriangles;
    if (!validateToolAccess(triangles, params.tool, inaccessibleTriangles)) {
        result.addError("Tool access issues: " + std::to_string(inaccessibleTriangles.size()) + " triangles cannot be reached by tool");
        std::cout << "❌ Tool access issues: " << inaccessibleTriangles.size() << " unreachable triangles" << std::endl;
    } else {
        std::cout << "✅ All surfaces accessible to tool" << std::endl;
    }
    
    // 3. Check draft angles
    std::vector<Triangle> invalidDraftTriangles;
    if (!checkDraftAngles(triangles, params.minDraftAngle, invalidDraftTriangles)) {
        result.addWarning("Draft angle issues: " + std::to_string(invalidDraftTriangles.size()) + 
                         " triangles have insufficient draft angle (< " + std::to_string(params.minDraftAngle) + "°)");
        std::cout << "⚠️  Draft angle warnings: " << invalidDraftTriangles.size() << " triangles with insufficient draft" << std::endl;
    } else {
        std::cout << "✅ All draft angles sufficient" << std::endl;
    }
    
    // 4. Validate material depth
    if (!validateMaterialDepth(triangles, params.material)) {
        result.addError("Part exceeds material thickness");
        std::cout << "❌ Part exceeds material thickness" << std::endl;
    } else {
        std::cout << "✅ Part fits within material bounds" << std::endl;
    }
    
    // 5. Validate stepdown
    std::vector<double> invalidSteps;
    if (!validateStepdown(triangles, params.tool.stepdown, invalidSteps)) {
        result.addWarning("Stepdown validation: Some areas may require smaller stepdown values");
        std::cout << "⚠️  Stepdown warnings: " << invalidSteps.size() << " areas may need adjustment" << std::endl;
    } else {
        std::cout << "✅ Stepdown values appropriate" << std::endl;
    }
    
    std::cout << "=========================\n" << std::endl;
    
    return result;
}

bool GeometryAnalyzer::hasUndercuts(const std::vector<Triangle>& triangles, std::vector<Triangle>& undercutTriangles) {
    undercutTriangles.clear();
    
    for (const auto& triangle : triangles) {
        // Check if triangle normal points significantly downward (indicates undercut)
        // Z-component of normal should be positive for upward-facing surfaces
        if (triangle.normal.z < -0.1) { // Tolerance for near-vertical surfaces
            undercutTriangles.push_back(triangle);
        }
    }
    
    return !undercutTriangles.empty();
}

bool GeometryAnalyzer::validateToolAccess(const std::vector<Triangle>& triangles, const Tool& tool, 
                                         std::vector<Triangle>& inaccessibleTriangles) {
    inaccessibleTriangles.clear();
    
    for (const auto& triangle : triangles) {
        if (!canToolReach(triangle, tool, triangles)) {
            inaccessibleTriangles.push_back(triangle);
        }
    }
    
    return inaccessibleTriangles.empty();
}

bool GeometryAnalyzer::checkDraftAngles(const std::vector<Triangle>& triangles, double minDraftAngle, 
                                       std::vector<Triangle>& invalidTriangles) {
    invalidTriangles.clear();
    
    for (const auto& triangle : triangles) {
        double draftAngle = calculateDraftAngle(triangle);
        if (draftAngle < minDraftAngle && !isUpwardFacing(triangle)) {
            invalidTriangles.push_back(triangle);
        }
    }
    
    return invalidTriangles.empty();
}

bool GeometryAnalyzer::validateMaterialDepth(const std::vector<Triangle>& triangles, const Material& material) {
    if (triangles.empty()) return true;
    
    // Find the Z-range of the part
    double minZ = triangles[0].vertices[0].z;
    double maxZ = triangles[0].vertices[0].z;
    
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            minZ = std::min(minZ, triangle.vertices[i].z);
            maxZ = std::max(maxZ, triangle.vertices[i].z);
        }
    }
    
    double partHeight = maxZ - minZ;
    return partHeight <= material.height;
}

bool GeometryAnalyzer::validateStepdown(const std::vector<Triangle>& triangles, double stepdown, 
                                       std::vector<double>& invalidSteps) {
    invalidSteps.clear();
    
    std::vector<double> layers = getMachiningLayers(triangles, stepdown);
    
    // For each layer, check if the stepdown is appropriate
    for (size_t i = 1; i < layers.size(); ++i) {
        double layerThickness = layers[i-1] - layers[i];
        
        // Check if any triangles in this layer have steep angles that might require smaller stepdown
        for (const auto& triangle : triangles) {
            double triZ = triangleZRange(triangle);
            if (triZ >= layers[i] && triZ <= layers[i-1]) {
                double angle = calculateDraftAngle(triangle);
                if (angle > 45.0 && layerThickness > stepdown * 0.5) {
                    invalidSteps.push_back(layerThickness);
                    break;
                }
            }
        }
    }
    
    return invalidSteps.empty();
}

std::vector<double> GeometryAnalyzer::getMachiningLayers(const std::vector<Triangle>& triangles, double stepdown) {
    if (triangles.empty()) return {};
    
    // Find Z range
    double minZ = triangles[0].vertices[0].z;
    double maxZ = triangles[0].vertices[0].z;
    
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            minZ = std::min(minZ, triangle.vertices[i].z);
            maxZ = std::max(maxZ, triangle.vertices[i].z);
        }
    }
    
    // Create layers from top to bottom
    std::vector<double> layers;
    for (double z = maxZ; z >= minZ; z -= stepdown) {
        layers.push_back(z);
    }
    
    // Make sure we include the bottom
    if (layers.back() > minZ) {
        layers.push_back(minZ);
    }
    
    return layers;
}

bool GeometryAnalyzer::isUpwardFacing(const Triangle& triangle, double tolerance) const {
    return triangle.normal.z > tolerance;
}

double GeometryAnalyzer::calculateDraftAngle(const Triangle& triangle) const {
    // Calculate angle between normal and vertical (Z-axis)
    Point3D vertical(0, 0, 1);
    double cosAngle = triangle.normal.dot(vertical);
    double angle = std::acos(std::abs(cosAngle)) * 180.0 / M_PI;
    return 90.0 - angle; // Convert to draft angle (0° = vertical, 90° = horizontal)
}

bool GeometryAnalyzer::canToolReach(const Triangle& triangle, const Tool& tool, 
                                   const std::vector<Triangle>& allTriangles) const {
    // Check if a straight tool can reach the triangle from above
    Point3D triangleCenter = triangle.getCenter();
    Point3D toolPosition = triangleCenter + Point3D(0, 0, tool.length);
    
    // Check for collisions with other triangles
    return !hasToolCollision(toolPosition, tool, allTriangles, triangle);
}

bool GeometryAnalyzer::hasToolCollision(const Point3D& toolPosition, const Tool& tool, 
                                       const std::vector<Triangle>& triangles, const Triangle& targetTriangle) const {
    double toolRadius = tool.diameter / 2.0;
    
    for (const auto& triangle : triangles) {
        // Skip the target triangle
        if (&triangle == &targetTriangle) continue;
        
        // Check if any vertex of the triangle is within the tool cylinder
        for (int i = 0; i < 3; ++i) {
            Point3D vertex = triangle.vertices[i];
            
            // Check if vertex is within tool height range
            if (vertex.z <= toolPosition.z && vertex.z >= toolPosition.z - tool.length) {
                // Check if vertex is within tool radius
                double distance = std::sqrt((vertex.x - toolPosition.x) * (vertex.x - toolPosition.x) + 
                                          (vertex.y - toolPosition.y) * (vertex.y - toolPosition.y));
                if (distance <= toolRadius) {
                    return true; // Collision detected
                }
            }
        }
    }
    
    return false; // No collision
}

bool GeometryAnalyzer::rayTriangleIntersection(const Point3D& rayOrigin, const Point3D& rayDirection, 
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

double GeometryAnalyzer::triangleZRange(const Triangle& triangle) const {
    return (triangle.vertices[0].z + triangle.vertices[1].z + triangle.vertices[2].z) / 3.0;
}

bool GeometryAnalyzer::isPointInTriangle(const Point3D& point, const Triangle& triangle) const {
    // Use barycentric coordinates
    Point3D v0 = triangle.vertices[2] - triangle.vertices[0];
    Point3D v1 = triangle.vertices[1] - triangle.vertices[0];
    Point3D v2 = point - triangle.vertices[0];
    
    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);
    
    double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

double GeometryAnalyzer::pointToTriangleDistance(const Point3D& point, const Triangle& triangle) const {
    // Simplified distance calculation (perpendicular distance to plane)
    Point3D toPoint = point - triangle.vertices[0];
    return std::abs(toPoint.dot(triangle.normal));
}