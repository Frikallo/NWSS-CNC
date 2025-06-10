#ifndef VALIDATION_ENGINE_H
#define VALIDATION_ENGINE_H

#include "types.h"
#include "geometry_analyzer.h"

class ValidationEngine {
public:
    ValidationEngine();
    ~ValidationEngine();
    
    // Main validation function
    ValidationResult validateForMachining(const std::vector<Triangle>& triangles, const MachiningParams& params);
    
    // Print validation report
    void printValidationReport(const ValidationResult& result) const;
    
    // Check if part fits within material bounds
    bool checkMaterialFit(const BoundingBox& partBounds, const Material& material) const;
    
    // Suggest material orientation for optimal machining
    Point3D suggestOptimalOrientation(const std::vector<Triangle>& triangles) const;
    
    // Calculate estimated machining time
    double estimateMachiningTime(const std::vector<Triangle>& triangles, const MachiningParams& params) const;
    
    // Generate machining recommendations
    std::vector<std::string> generateRecommendations(const ValidationResult& result, 
                                                     const std::vector<Triangle>& triangles,
                                                     const MachiningParams& params) const;

private:
    GeometryAnalyzer m_analyzer;
    
    // Helper functions
    double calculateMachiningVolume(const std::vector<Triangle>& triangles) const;
    double estimateRoughingTime(double volume, const Tool& tool) const;
    double estimateFinishingTime(double surfaceArea, const Tool& tool) const;
    double calculateSurfaceArea(const std::vector<Triangle>& triangles) const;
};

#endif // VALIDATION_ENGINE_H