#include "validation_engine.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

ValidationEngine::ValidationEngine() {}

ValidationEngine::~ValidationEngine() {}

ValidationResult ValidationEngine::validateForMachining(const std::vector<Triangle>& triangles, const MachiningParams& params) {
    std::cout << "\n=== CNC Machining Validation ===" << std::endl;
    std::cout << "Material: " << params.material.width << " x " << params.material.length << " x " << params.material.height << std::endl;
    std::cout << "Tool: Ø" << params.tool.diameter << "mm, Length: " << params.tool.length << "mm" << std::endl;
    std::cout << "Stepdown: " << params.tool.stepdown << "mm" << std::endl;
    std::cout << "Min Draft Angle: " << params.minDraftAngle << "°" << std::endl;
    
    // Run comprehensive geometry analysis
    ValidationResult result = m_analyzer.analyzeMesh(triangles, params);
    
    // Additional material fit check
    BoundingBox partBounds;
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            partBounds.update(triangle.vertices[i]);
        }
    }
    
    if (!checkMaterialFit(partBounds, params.material)) {
        result.addError("Part dimensions exceed material bounds");
    }
    
    // Estimate machining time
    double estimatedTime = estimateMachiningTime(triangles, params);
    std::cout << "Estimated machining time: " << std::fixed << std::setprecision(1) << estimatedTime << " minutes" << std::endl;
    
    return result;
}

void ValidationEngine::printValidationReport(const ValidationResult& result) const {
    std::cout << "\n=== Validation Report ===" << std::endl;
    
    if (result.isValid) {
        std::cout << "✅ VALIDATION PASSED - Part is suitable for CNC machining" << std::endl;
    } else {
        std::cout << "❌ VALIDATION FAILED - Issues must be resolved before machining" << std::endl;
    }
    
    if (!result.errors.empty()) {
        std::cout << "\n🚫 ERRORS:" << std::endl;
        for (size_t i = 0; i < result.errors.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << result.errors[i] << std::endl;
        }
    }
    
    if (!result.warnings.empty()) {
        std::cout << "\n⚠️  WARNINGS:" << std::endl;
        for (size_t i = 0; i < result.warnings.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << result.warnings[i] << std::endl;
        }
    }
    
    if (result.errors.empty() && result.warnings.empty()) {
        std::cout << "\n✅ No issues detected" << std::endl;
    }
    
    std::cout << "========================\n" << std::endl;
}

bool ValidationEngine::checkMaterialFit(const BoundingBox& partBounds, const Material& material) const {
    Point3D partSize = partBounds.getSize();
    
    bool fitsX = partSize.x <= material.width;
    bool fitsY = partSize.y <= material.length;
    bool fitsZ = partSize.z <= material.height;
    
    if (!fitsX) {
        std::cout << "❌ Part width (" << partSize.x << ") exceeds material width (" << material.width << ")" << std::endl;
    }
    if (!fitsY) {
        std::cout << "❌ Part length (" << partSize.y << ") exceeds material length (" << material.length << ")" << std::endl;
    }
    if (!fitsZ) {
        std::cout << "❌ Part height (" << partSize.z << ") exceeds material height (" << material.height << ")" << std::endl;
    }
    
    return fitsX && fitsY && fitsZ;
}

Point3D ValidationEngine::suggestOptimalOrientation(const std::vector<Triangle>& triangles) const {
    // Find the orientation that minimizes undercuts
    // For simplicity, suggest keeping the part as-is (could be enhanced with rotation analysis)
    return Point3D(0, 0, 0); // No rotation suggested
}

double ValidationEngine::estimateMachiningTime(const std::vector<Triangle>& triangles, const MachiningParams& params) const {
    if (triangles.empty()) return 0.0;
    
    double volume = calculateMachiningVolume(triangles);
    double surfaceArea = calculateSurfaceArea(triangles);
    
    double roughingTime = estimateRoughingTime(volume, params.tool);
    double finishingTime = estimateFinishingTime(surfaceArea, params.tool);
    
    return roughingTime + finishingTime;
}

std::vector<std::string> ValidationEngine::generateRecommendations(const ValidationResult& result, 
                                                                  const std::vector<Triangle>& triangles,
                                                                  const MachiningParams& params) const {
    std::vector<std::string> recommendations;
    
    if (!result.isValid) {
        recommendations.push_back("Resolve all validation errors before proceeding");
    }
    
    if (!result.warnings.empty()) {
        recommendations.push_back("Consider addressing warnings for optimal results");
    }
    
    // Analyze geometry for specific recommendations
    double volume = calculateMachiningVolume(triangles);
    if (volume > 1000.0) { // Large volume
        recommendations.push_back("Consider using a larger tool for roughing operations");
    }
    
    if (params.tool.stepdown > 2.0) {
        recommendations.push_back("Consider reducing stepdown for better surface finish");
    }
    
    if (recommendations.empty()) {
        recommendations.push_back("Part is well-suited for CNC machining as configured");
    }
    
    return recommendations;
}

double ValidationEngine::calculateMachiningVolume(const std::vector<Triangle>& triangles) const {
    if (triangles.empty()) return 0.0;
    
    // Calculate bounding box volume as approximation
    BoundingBox bounds;
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            bounds.update(triangle.vertices[i]);
        }
    }
    
    Point3D size = bounds.getSize();
    return size.x * size.y * size.z;
}

double ValidationEngine::estimateRoughingTime(double volume, const Tool& tool) const {
    // Simplified time estimation: volume / material removal rate
    double materialRemovalRate = (tool.diameter * tool.stepdown * tool.feedrate) / 1000.0; // mm³/min
    return volume / std::max(materialRemovalRate, 1.0); // Avoid division by zero
}

double ValidationEngine::estimateFinishingTime(double surfaceArea, const Tool& tool) const {
    // Simplified finishing time: surface area / feed rate
    double finishingRate = tool.feedrate / 100.0; // Slower for finishing
    return surfaceArea / std::max(finishingRate, 1.0);
}

double ValidationEngine::calculateSurfaceArea(const std::vector<Triangle>& triangles) const {
    double totalArea = 0.0;
    
    for (const auto& triangle : triangles) {
        // Calculate triangle area using cross product
        Point3D edge1 = triangle.vertices[1] - triangle.vertices[0];
        Point3D edge2 = triangle.vertices[2] - triangle.vertices[0];
        Point3D cross = edge1.cross(edge2);
        totalArea += cross.magnitude() * 0.5;
    }
    
    return totalArea;
}