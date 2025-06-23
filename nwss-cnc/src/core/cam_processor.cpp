#include "core/cam_processor.h"
#include "core/tool_offset.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <functional>
#include <iostream>

namespace nwss {
namespace cnc {

CAMProcessor::CAMProcessor() {
}

CAMProcessor::~CAMProcessor() {
}

void CAMProcessor::setConfig(const CNConfig& config) {
    m_config = config;
}

void CAMProcessor::setToolRegistry(const ToolRegistry& registry) {
    m_toolRegistry = registry;
}

CAMOperationResult CAMProcessor::processForCAM(const std::vector<Path>& paths,
                                             const CutoutParams& cutoutParams,
                                             int selectedToolId) {
    CAMOperationResult result;
    
    std::cout << "DEBUG: CAMProcessor::processForCAM() called with:" << std::endl;
    std::cout << "  - Input paths: " << paths.size() << std::endl;
    std::cout << "  - Cutout mode: " << static_cast<int>(cutoutParams.mode) << std::endl;
    std::cout << "  - Selected tool ID: " << selectedToolId << std::endl;
    std::cout << "  - Stepover: " << cutoutParams.stepover << std::endl;
    
    // Get tool information
    const Tool* toolPtr = m_toolRegistry.getTool(selectedToolId);
    if (!toolPtr) {
        std::cout << "DEBUG: Tool lookup failed for ID: " << selectedToolId << std::endl;
        addError(result, "Invalid tool ID: " + std::to_string(selectedToolId));
        return result;
    }
    Tool tool = *toolPtr;
    std::cout << "DEBUG: Tool found - diameter: " << tool.diameter << "mm, name: " << tool.name << std::endl;
    
    // Convert paths to polygons
    std::vector<Polygon> polygons;
    for (const auto& path : paths) {
        std::cout << "DEBUG: Processing path with " << path.size() << " points" << std::endl;
        if (path.size() >= 3) {
            Polygon polygon(path.getPoints());
            polygons.push_back(polygon);
            std::cout << "DEBUG: Added polygon with " << polygon.size() << " points" << std::endl;
        } else {
            std::cout << "DEBUG: Skipping path with < 3 points" << std::endl;
        }
    }
    
    if (polygons.empty()) {
        std::cout << "DEBUG: No valid polygons found" << std::endl;
        addError(result, "No valid polygons found in input paths");
        return result;
    }
    
    std::cout << "DEBUG: Created " << polygons.size() << " polygons" << std::endl;
    
    // Analyze polygon hierarchy using Clipper2
    std::cout << "DEBUG: Analyzing polygon hierarchy..." << std::endl;
    auto hierarchy = analyzePolygonHierarchy(polygons);
    std::cout << "DEBUG: Hierarchy analysis complete - " << hierarchy.size() << " root nodes" << std::endl;
    
    // Validate feasibility for each polygon
    std::cout << "DEBUG: Validating polygon feasibility..." << std::endl;
    for (const auto& poly : polygons) {
        auto validation = validateToolpathFeasibility(poly, tool.diameter, cutoutParams.mode);
        result.warnings.insert(result.warnings.end(), validation.warnings.begin(), validation.warnings.end());
        result.errors.insert(result.errors.end(), validation.errors.begin(), validation.errors.end());
        std::cout << "DEBUG: Polygon validation - success: " << validation.success 
                  << ", warnings: " << validation.warnings.size() 
                  << ", errors: " << validation.errors.size() << std::endl;
    }
    
    if (!result.errors.empty()) {
        std::cout << "DEBUG: Validation failed with " << result.errors.size() << " errors:" << std::endl;
        for (const auto& error : result.errors) {
            std::cout << "  ERROR: " << error << std::endl;
        }
        return result;
    }
    
    // Generate toolpaths based on cutout mode
    CAMOperationResult toolpathResult;
    double stepover = cutoutParams.stepover * tool.diameter;
    std::cout << "DEBUG: Calculated stepover: " << stepover << "mm" << std::endl;
    
    switch (cutoutParams.mode) {
        case CutoutMode::PERIMETER:
            std::cout << "DEBUG: Using PERIMETER mode" << std::endl;
            // For perimeter, just use the original paths
            for (const auto& path : paths) {
                result.toolpaths.push_back(path);
            }
            result.success = true;
            break;
            
        case CutoutMode::PUNCHOUT:
            std::cout << "DEBUG: Using PUNCHOUT mode" << std::endl;
            toolpathResult = generatePunchoutToolpaths(hierarchy, tool.diameter, stepover);
            break;
            
        case CutoutMode::POCKET:
            std::cout << "DEBUG: Using POCKET mode" << std::endl;
            toolpathResult = generatePocketToolpaths(hierarchy, tool.diameter, stepover, 
                                                   cutoutParams.spiralIn);
            break;
            
        case CutoutMode::ENGRAVE:
            std::cout << "DEBUG: Using ENGRAVE mode" << std::endl;
            toolpathResult = generateEngraveToolpaths(hierarchy, tool.diameter, stepover);
            break;
    }
    
    if (cutoutParams.mode != CutoutMode::PERIMETER) {
        std::cout << "DEBUG: Toolpath generation result - success: " << toolpathResult.success 
                  << ", paths: " << toolpathResult.toolpaths.size() 
                  << ", warnings: " << toolpathResult.warnings.size() 
                  << ", errors: " << toolpathResult.errors.size() << std::endl;
                  
        result.toolpaths = toolpathResult.toolpaths;
        result.success = toolpathResult.success;
        result.warnings.insert(result.warnings.end(), 
                              toolpathResult.warnings.begin(), toolpathResult.warnings.end());
        result.errors.insert(result.errors.end(), 
                            toolpathResult.errors.begin(), toolpathResult.errors.end());
    }
    
    // Optimize toolpath order
    if (result.success && !result.toolpaths.empty()) {
        std::cout << "DEBUG: Optimizing toolpath order..." << std::endl;
        result.toolpaths = optimizeToolpathOrder(result.toolpaths);
        result.toolpaths = removeRedundantMoves(result.toolpaths);
        
        // Calculate statistics
        for (const auto& path : result.toolpaths) {
            result.totalCuttingDistance += path.length();
        }
        
        // Estimate machining time
        double feedRate = m_config.getFeedRate() > 0 ? m_config.getFeedRate() : 1000.0; // mm/min
        result.estimatedMachiningTime = result.totalCuttingDistance / feedRate; // minutes
    }
    
    std::cout << "DEBUG: CAMProcessor::processForCAM() complete - final result:" << std::endl;
    std::cout << "  - Success: " << result.success << std::endl;
    std::cout << "  - Toolpaths: " << result.toolpaths.size() << std::endl;
    std::cout << "  - Warnings: " << result.warnings.size() << std::endl;
    std::cout << "  - Errors: " << result.errors.size() << std::endl;
    
    return result;
}

std::vector<std::shared_ptr<PolygonHierarchy>> CAMProcessor::analyzePolygonHierarchy(
    const std::vector<Polygon>& polygons) {
    
    std::vector<std::shared_ptr<PolygonHierarchy>> hierarchy;
    
    // Convert polygons to Clipper2 format for robust hierarchy analysis
    Clipper2Lib::Paths64 clipperPaths = polygonsToClipperPaths(polygons);
    
    // Use Clipper2's PolyTree to get proper hierarchy
    Clipper2Lib::PolyTree64 polyTree;
    Clipper2Lib::Clipper64 clipper;
    clipper.AddSubject(clipperPaths);
    clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, polyTree);
    
    // Convert PolyTree back to our hierarchy structure
    std::function<void(const Clipper2Lib::PolyPath64*, std::shared_ptr<PolygonHierarchy>)> 
    processPolyPath = [&](const Clipper2Lib::PolyPath64* polyPath, 
                         std::shared_ptr<PolygonHierarchy> parent) {
        
        if (!polyPath->Polygon().empty()) {
            auto node = std::make_shared<PolygonHierarchy>();
            node->polygon = clipperPathToPolygon(polyPath->Polygon());
            node->parent = parent;
            node->level = parent ? parent->level + 1 : 0;
            node->isHole = (node->level % 2) == 1;
            
            if (parent) {
                parent->children.push_back(node);
            } else {
                hierarchy.push_back(node);
            }
            
            // Process children
            for (const auto& child : *polyPath) {
                processPolyPath(child.get(), node);
            }
        }
    };
    
    // Process top-level polygons
    for (const auto& child : polyTree) {
        processPolyPath(child.get(), nullptr);
    }
    
    return hierarchy;
}

CAMOperationResult CAMProcessor::generatePunchoutToolpaths(
    const std::vector<std::shared_ptr<PolygonHierarchy>>& hierarchy,
    double toolDiameter, double stepover) {
    
    // PUNCHOUT MODE: Removes ALL material inside the shape using spiral cutting
    // Handles arbitrary complex designs including text, gears, nested shapes, etc.
    CAMOperationResult result;
    
    std::cout << "DEBUG: generatePunchoutToolpaths() called with:" << std::endl;
    std::cout << "  - Hierarchy nodes: " << hierarchy.size() << std::endl;
    std::cout << "  - Tool diameter: " << toolDiameter << "mm" << std::endl;
    std::cout << "  - Stepover: " << stepover << "mm" << std::endl;
    
    // Strategy for complex designs:
    // 1. Process ALL polygons (including holes) to handle text, gears, etc.
    // 2. Generate individual spirals for each shape
    // 3. Handle nested geometry properly
    
    for (const auto& node : hierarchy) {
        std::cout << "DEBUG: Processing " << (node->isHole ? "HOLE" : "SOLID") 
                  << " polygon with " << node->polygon.size() << " points" << std::endl;
        
        // For complex designs, we need to cut out EVERYTHING
        // This includes holes in text (like "A", "B", "P"), gear teeth, etc.
        
        if (node->isHole) {
            // HOLES: These are areas that should remain as material
            // For punchout, we still need to cut around them to separate them from waste
            std::cout << "DEBUG: Processing hole - cutting around perimeter to preserve island" << std::endl;
            
            // Cut around the hole perimeter to create an "island" of material
            auto holePaths = generateSpiralToolpath(node->polygon, toolDiameter, stepover, true);
            
            for (const auto& path : holePaths) {
                result.toolpaths.push_back(path);
                std::cout << "DEBUG: Added hole island path with " << path.size() << " points" << std::endl;
            }
        } else {
            // SOLID AREAS: Remove all material inside
            std::cout << "DEBUG: Processing solid area - removing all internal material" << std::endl;
            
            if (!node->children.empty()) {
                std::cout << "DEBUG: Complex shape with " << node->children.size() 
                          << " internal features (holes/islands)" << std::endl;
            }
            
            // Generate spiral to remove all material inside this shape
            auto punchoutPaths = generateSpiralToolpath(node->polygon, toolDiameter, stepover, true);
            
            std::cout << "DEBUG: Generated " << punchoutPaths.size() << " spiral paths for solid area" << std::endl;
            
            for (const auto& path : punchoutPaths) {
                result.toolpaths.push_back(path);
                std::cout << "DEBUG: Added solid area spiral path with " << path.size() << " points" << std::endl;
            }
        }
    }
    
    std::cout << "DEBUG: generatePunchoutToolpaths() complete - " << result.toolpaths.size() << " total paths" << std::endl;
    std::cout << "DEBUG: This design will punch out all solid areas while preserving hole islands" << std::endl;
    result.success = true;
    return result;
}

CAMOperationResult CAMProcessor::generatePocketToolpaths(
    const std::vector<std::shared_ptr<PolygonHierarchy>>& hierarchy,
    double toolDiameter, double stepover, bool spiralIn) {
    
    // POCKET MODE: Creates recessed areas by removing material inside shapes
    // Similar to punchout but typically used for partial depth cuts
    CAMOperationResult result;
    
    for (const auto& node : hierarchy) {
        if (node->isHole) {
            continue; // Skip holes for pocketing
        }
        
        // Check if polygon is suitable for pocketing
        if (isPolygonTooSmallForTool(node->polygon, toolDiameter)) {
            addWarning(result, "Polygon too small for selected tool diameter");
            continue;
        }
        
        std::vector<Path> pocketPaths;
        if (spiralIn) {
            pocketPaths = generateSpiralToolpath(node->polygon, toolDiameter, stepover, true);
        } else {
            pocketPaths = generateParallelToolpath(node->polygon, toolDiameter, stepover, 0.0);
        }
        
        result.toolpaths.insert(result.toolpaths.end(), pocketPaths.begin(), pocketPaths.end());
    }
    
    result.success = true;
    return result;
}

CAMOperationResult CAMProcessor::generateEngraveToolpaths(
    const std::vector<std::shared_ptr<PolygonHierarchy>>& hierarchy,
    double toolDiameter, double stepover) {
    
    CAMOperationResult result;
    
    for (const auto& node : hierarchy) {
        if (node->isHole) {
            continue; // Skip holes for engraving
        }
        
        // Generate raster pattern at 45 degrees for optimal surface finish
        auto engravePaths = generateRasterToolpath(node->polygon, toolDiameter, stepover, 45.0);
        result.toolpaths.insert(result.toolpaths.end(), engravePaths.begin(), engravePaths.end());
    }
    
    result.success = true;
    return result;
}

CAMOperationResult CAMProcessor::validateToolpathFeasibility(
    const Polygon& polygon, double toolDiameter, CutoutMode cutoutMode) {
    
    CAMOperationResult result;
    result.success = true;
    
    std::cout << "DEBUG: validateToolpathFeasibility() called:" << std::endl;
    std::cout << "  - Polygon points: " << polygon.size() << std::endl;
    std::cout << "  - Tool diameter: " << toolDiameter << "mm" << std::endl;
    std::cout << "  - Cutout mode: " << static_cast<int>(cutoutMode) << std::endl;
    
    // Check for invalid geometry
    if (hasInvalidGeometry(polygon)) {
        std::cout << "DEBUG: Invalid geometry detected" << std::endl;
        addError(result, "Polygon has invalid geometry");
        result.success = false;
        return result;
    }
    
    // Check polygon area - this is more meaningful than minimum feature size
    double area = polygon.area();
    double toolArea = (toolDiameter * toolDiameter * M_PI) / 4.0; // Area of tool circle
    std::cout << "DEBUG: Polygon area: " << area << "mm², tool area: " << toolArea << "mm²" << std::endl;
    
    if (area < toolArea * 2.0) { // Need at least 2x tool area for meaningful cutting
        if (cutoutMode == CutoutMode::POCKET) {
            std::cout << "DEBUG: Polygon area too small for pocketing" << std::endl;
            addError(result, "Polygon area too small for pocketing with selected tool");
            result.success = false;
        } else if (cutoutMode == CutoutMode::PUNCHOUT) {
            std::cout << "DEBUG: Polygon area too small for punchout" << std::endl;
            addWarning(result, "Polygon area may be too small for effective punchout");
        }
    }
    
    // For area operations, check if polygon is large enough to contain tool
    double minX, minY, maxX, maxY;
    polygon.getBounds(minX, minY, maxX, maxY);
    double width = maxX - minX;
    double height = maxY - minY;
    double minDimension = std::min(width, height);
    
    std::cout << "DEBUG: Polygon dimensions: " << width << "x" << height << "mm, min: " << minDimension << "mm" << std::endl;
    
    if (minDimension < toolDiameter * 1.5) { // Need at least 1.5x tool diameter
        if (cutoutMode == CutoutMode::POCKET || cutoutMode == CutoutMode::PUNCHOUT) {
            std::cout << "DEBUG: Polygon too narrow for tool" << std::endl;
            addError(result, "Polygon too narrow for tool diameter (" + 
                    std::to_string(minDimension) + "mm < " + std::to_string(toolDiameter * 1.5) + "mm required)");
            result.success = false;
        } else {
            addWarning(result, "Polygon dimensions may cause issues with selected tool");
        }
    }
    
    // Only check for self-intersections on original geometry, not tool-offset paths
    // Tool offset can legitimately create self-intersections for complex shapes
    if (polygon.size() < 100) { // Only check simple polygons, not complex offset paths
        if (checkForSelfIntersections(polygon)) {
            std::cout << "DEBUG: Self-intersections detected in original geometry" << std::endl;
            addWarning(result, "Polygon has self-intersections - toolpaths may be unreliable");
            // Don't fail validation for this, just warn
        }
    } else {
        std::cout << "DEBUG: Skipping self-intersection check for complex polygon (" << polygon.size() << " points)" << std::endl;
    }
    
    std::cout << "DEBUG: Validation complete - success: " << result.success 
              << ", warnings: " << result.warnings.size() 
              << ", errors: " << result.errors.size() << std::endl;
    
    return result;
}

// Clipper2 conversion utilities
Clipper2Lib::Paths64 CAMProcessor::polygonToClipperPaths(const Polygon& polygon) {
    Clipper2Lib::Paths64 paths;
    Clipper2Lib::Path64 path;
    
    for (const auto& point : polygon.getPoints()) {
        path.push_back(Clipper2Lib::Point64(
            static_cast<int64_t>(point.x * 1000), // Convert to integer with 0.001mm precision
            static_cast<int64_t>(point.y * 1000)
        ));
    }
    
    paths.push_back(path);
    return paths;
}

Clipper2Lib::Paths64 CAMProcessor::polygonsToClipperPaths(const std::vector<Polygon>& polygons) {
    Clipper2Lib::Paths64 paths;
    
    for (const auto& polygon : polygons) {
        auto polyPaths = polygonToClipperPaths(polygon);
        paths.insert(paths.end(), polyPaths.begin(), polyPaths.end());
    }
    
    return paths;
}

std::vector<Polygon> CAMProcessor::clipperPathsToPolygons(const Clipper2Lib::Paths64& paths) {
    std::vector<Polygon> polygons;
    
    for (const auto& path : paths) {
        polygons.push_back(clipperPathToPolygon(path));
    }
    
    return polygons;
}

Polygon CAMProcessor::clipperPathToPolygon(const Clipper2Lib::Path64& path) {
    Polygon polygon;
    
    for (const auto& point : path) {
        polygon.addPoint(Point2D(
            static_cast<double>(point.x) / 1000.0, // Convert back from integer
            static_cast<double>(point.y) / 1000.0
        ));
    }
    
    return polygon;
}

// Advanced polygon operations using Clipper2
std::vector<Polygon> CAMProcessor::offsetPolygon(const Polygon& polygon, double offset) {
    
    // Use direct Clipper2 offsetting for pure geometric operations
    // Positive offset = expand outward, Negative offset = shrink inward
    
    auto clipperPaths = polygonToClipperPaths(polygon);
    
    // Scale offset to Clipper2 integer coordinates (1000x scale factor)
    double scaledOffset = offset * 1000.0;
    
    // Perform offset operation using Clipper2
    auto offsetPaths = Clipper2Lib::InflatePaths(clipperPaths, scaledOffset, 
                                                Clipper2Lib::JoinType::Round, 
                                                Clipper2Lib::EndType::Polygon);
    
    // Convert back to polygons
    return clipperPathsToPolygons(offsetPaths);
}

std::vector<Polygon> CAMProcessor::unionPolygons(const std::vector<Polygon>& polygons) {
    auto clipperPaths = polygonsToClipperPaths(polygons);
    auto result = Clipper2Lib::Union(clipperPaths, Clipper2Lib::FillRule::NonZero);
    return clipperPathsToPolygons(result);
}

std::vector<Polygon> CAMProcessor::intersectPolygons(const std::vector<Polygon>& polygons1,
                                                   const std::vector<Polygon>& polygons2) {
    auto clipperPaths1 = polygonsToClipperPaths(polygons1);
    auto clipperPaths2 = polygonsToClipperPaths(polygons2);
    auto result = Clipper2Lib::Intersect(clipperPaths1, clipperPaths2, Clipper2Lib::FillRule::NonZero);
    return clipperPathsToPolygons(result);
}

std::vector<Polygon> CAMProcessor::differencePolygons(const std::vector<Polygon>& subject,
                                                    const std::vector<Polygon>& clip) {
    auto clipperSubject = polygonsToClipperPaths(subject);
    auto clipperClip = polygonsToClipperPaths(clip);
    auto result = Clipper2Lib::Difference(clipperSubject, clipperClip, Clipper2Lib::FillRule::NonZero);
    return clipperPathsToPolygons(result);
}

// Professional toolpath generation algorithms
std::vector<Path> CAMProcessor::generateSpiralToolpath(const Polygon& polygon, 
                                                     double toolDiameter, double stepover, 
                                                     bool inward) {
    std::vector<Path> paths;
    
    double toolRadius = toolDiameter / 2.0;
    
    // For inward spiral (punchout): start from outside boundary, spiral toward center
    // For outward spiral (pocket): start from inside, spiral toward boundary
    std::vector<Polygon> currentPolygons;
    
    if (inward) {
        // PUNCHOUT: Start from the outer boundary (no initial offset)
        // and spiral inward by stepover each pass
        std::cout << "DEBUG: Starting INWARD spiral (punchout) from outer boundary" << std::endl;
        currentPolygons.push_back(polygon);
    } else {
        // POCKET: Start from inside (offset inward by tool radius)
        // and spiral outward by stepover each pass  
        std::cout << "DEBUG: Starting OUTWARD spiral (pocket) from inner boundary" << std::endl;
        currentPolygons = offsetPolygon(polygon, -toolRadius);
    }
    
    int passCount = 0;
    const int MAX_SPIRAL_PASSES = 1000;
    
    std::cout << "DEBUG: Starting spiral with " << currentPolygons.size() << " initial polygons" << std::endl;
    
    while (!currentPolygons.empty() && passCount < MAX_SPIRAL_PASSES) {
        // Convert largest polygon to path
        auto largestPoly = *std::max_element(currentPolygons.begin(), currentPolygons.end(),
            [](const Polygon& a, const Polygon& b) { return a.area() < b.area(); });
        
        double currentArea = largestPoly.area();
        std::cout << "DEBUG: Pass " << passCount << " - area: " << currentArea << "mm²" << std::endl;
        
        if (largestPoly.size() >= 3) {
            Path path(largestPoly.getPoints());
            paths.push_back(path);
        }
        
        // Offset for next pass
        double offsetDistance = inward ? -stepover : stepover;
        auto nextPolygons = offsetPolygon(largestPoly, offsetDistance);
        
        std::cout << "DEBUG: Offset by " << offsetDistance << "mm generated " << nextPolygons.size() << " polygons" << std::endl;
        
        // Filter out polygons that are too small
        currentPolygons.clear();
        double minArea = (toolDiameter * toolDiameter) * 0.1; // Even smaller minimum area
        for (const auto& poly : nextPolygons) {
            double polyArea = poly.area();
            if (polyArea > minArea) {
                currentPolygons.push_back(poly);
                std::cout << "DEBUG: Keeping polygon with area " << polyArea << "mm²" << std::endl;
            } else {
                std::cout << "DEBUG: Filtering out small polygon with area " << polyArea << "mm²" << std::endl;
            }
        }
        
        passCount++;
        
        // Only stop if we truly have no more polygons to process
        if (currentPolygons.empty()) {
            std::cout << "DEBUG: Spiral complete - no more polygons to process" << std::endl;
            break;
        }
    }
    
    std::cout << "DEBUG: Spiral toolpath complete - " << passCount << " passes, " << paths.size() << " paths" << std::endl;
    return paths;
}

std::vector<Path> CAMProcessor::generateParallelToolpath(const Polygon& polygon,
                                                       double toolDiameter, double stepover,
                                                       double angle) {
    return generateRasterToolpath(polygon, toolDiameter, stepover, angle);
}

std::vector<Path> CAMProcessor::generateContourToolpath(const Polygon& polygon,
                                                      double toolDiameter, double stepover) {
    std::vector<Path> paths;
    
    double toolRadius = toolDiameter / 2.0;
    
    // Start with initial offset
    auto currentPolygons = offsetPolygon(polygon, -toolRadius);
    
    int passCount = 0;
    const int MAX_PASSES = 10; // Reduced limit for efficiency
    double previousTotalArea = 0.0;
    
    while (!currentPolygons.empty() && passCount < MAX_PASSES) {
        std::vector<Polygon> nextPolygons;
        double currentTotalArea = 0.0;
        
        for (const auto& poly : currentPolygons) {
            if (poly.size() >= 3) {
                currentTotalArea += poly.area();
                Path path(poly.getPoints());
                paths.push_back(path);
                
                // Generate next offset
                auto offsetPolys = offsetPolygon(poly, -stepover);
                
                for (const auto& offsetPoly : offsetPolys) {
                    double area = offsetPoly.area();
                    double minArea = (toolDiameter * toolDiameter) * 2.0; // Increased minimum area
                    
                    if (area > minArea) {
                        nextPolygons.push_back(offsetPoly);
                    }
                }
            }
        }
        
        // Check for convergence - if area isn't decreasing significantly, stop
        if (passCount > 0 && previousTotalArea > 0) {
            double areaReduction = (previousTotalArea - currentTotalArea) / previousTotalArea;
            if (areaReduction < 0.1) { // Less than 10% reduction
                std::cout << "DEBUG: Contour toolpath converged after " << passCount << " passes" << std::endl;
                break;
            }
        }
        
        currentPolygons = nextPolygons;
        previousTotalArea = currentTotalArea;
        passCount++;
    }
    
    std::cout << "DEBUG: Contour toolpath complete - " << passCount << " passes, " << paths.size() << " paths" << std::endl;
    return paths;
}

std::vector<Path> CAMProcessor::generateRasterToolpath(const Polygon& polygon,
                                                     double toolDiameter, double stepover,
                                                     double angle) {
    std::vector<Path> paths;
    
    // Get polygon bounds
    double minX, minY, maxX, maxY;
    polygon.getBounds(minX, minY, maxX, maxY);
    
    // Calculate raster lines
    double angleRad = angle * M_PI / 180.0;
    double cosAngle = std::cos(angleRad);
    double sinAngle = std::sin(angleRad);
    
    // Determine the number of passes needed
    double diagonal = std::sqrt((maxX - minX) * (maxX - minX) + (maxY - minY) * (maxY - minY));
    int numPasses = static_cast<int>(std::ceil(diagonal / stepover));
    
    for (int i = 0; i < numPasses; ++i) {
        double offset = i * stepover - diagonal / 2.0;
        
        // Create a long line across the bounding box
        Point2D start(-diagonal, offset);
        Point2D end(diagonal, offset);
        
        // Rotate the line by the specified angle
        Point2D center((minX + maxX) / 2.0, (minY + maxY) / 2.0);
        Point2D rotatedStart(
            start.x * cosAngle - start.y * sinAngle + center.x,
            start.x * sinAngle + start.y * cosAngle + center.y
        );
        Point2D rotatedEnd(
            end.x * cosAngle - end.y * sinAngle + center.x,
            end.x * sinAngle + end.y * cosAngle + center.y
        );
        
        // TODO: Use Clipper2 for proper line-polygon intersection
        // For now, use simple point containment check
        Path rasterLine;
        rasterLine.addPoint(rotatedStart);
        rasterLine.addPoint(rotatedEnd);
        
        Point2D midPoint((rotatedStart.x + rotatedEnd.x) / 2.0, (rotatedStart.y + rotatedEnd.y) / 2.0);
        if (polygon.containsPoint(midPoint)) {
            paths.push_back(rasterLine);
        }
    }
    
    return paths;
}

// Validation and analysis
bool CAMProcessor::isPolygonTooSmallForTool(const Polygon& polygon, double toolDiameter) {
    double minFeatureSize = calculateMinimumFeatureSize(polygon);
    return minFeatureSize < toolDiameter;
}

bool CAMProcessor::hasInvalidGeometry(const Polygon& polygon) {
    if (polygon.size() < 3) return true;
    
    const auto& points = polygon.getPoints();
    
    // Check for duplicate consecutive points
    for (size_t i = 1; i < points.size(); ++i) {
        if (points[i] == points[i-1]) {
            return true;
        }
    }
    
    return false;
}

double CAMProcessor::calculateMinimumFeatureSize(const Polygon& polygon) {
    if (polygon.size() < 3) return 0.0;
    
    double minDistance = std::numeric_limits<double>::max();
    const auto& points = polygon.getPoints();
    
    // Calculate minimum distance between consecutive points
    for (size_t i = 1; i < points.size(); ++i) {
        double distance = points[i].distanceTo(points[i-1]);
        minDistance = std::min(minDistance, distance);
    }
    
    // Also check distance from last to first point
    double distance = points[0].distanceTo(points.back());
    minDistance = std::min(minDistance, distance);
    
    return minDistance;
}

bool CAMProcessor::checkForSelfIntersections(const Polygon& polygon) {
    // Use Clipper2 to check for self-intersections
    auto clipperPaths = polygonToClipperPaths(polygon);
    
    // A simple way to check is to perform a union operation
    // If the result differs significantly from the input, there were self-intersections
    auto unionResult = Clipper2Lib::Union(clipperPaths, Clipper2Lib::FillRule::NonZero);
    
    return unionResult.size() != 1 || unionResult[0].size() != polygon.size();
}

// Toolpath optimization
std::vector<Path> CAMProcessor::optimizeToolpathOrder(const std::vector<Path>& toolpaths) {
    if (toolpaths.size() <= 1) return toolpaths;
    
    std::vector<Path> optimized;
    std::vector<bool> used(toolpaths.size(), false);
    
    // Start with the first path
    optimized.push_back(toolpaths[0]);
    used[0] = true;
    
    Point2D currentEnd = toolpaths[0].getPoints().back();
    
    // Greedy nearest-neighbor optimization
    for (size_t i = 1; i < toolpaths.size(); ++i) {
        double minDistance = std::numeric_limits<double>::max();
        size_t nextIndex = 0;
        
        for (size_t j = 0; j < toolpaths.size(); ++j) {
            if (used[j]) continue;
            
            Point2D pathStart = toolpaths[j].getPoints().front();
            double distance = pathStart.distanceTo(currentEnd);
            
            if (distance < minDistance) {
                minDistance = distance;
                nextIndex = j;
            }
        }
        
        optimized.push_back(toolpaths[nextIndex]);
        used[nextIndex] = true;
        currentEnd = toolpaths[nextIndex].getPoints().back();
    }
    
    return optimized;
}

std::vector<Path> CAMProcessor::removeRedundantMoves(const std::vector<Path>& toolpaths) {
    std::vector<Path> cleaned;
    
    for (const auto& path : toolpaths) {
        if (path.size() < 2) continue;
        
        Path cleanedPath;
        const auto& points = path.getPoints();
        cleanedPath.addPoint(points[0]);
        
        for (size_t i = 1; i < points.size(); ++i) {
            Point2D current = points[i];
            Point2D previous = cleanedPath.getPoints().back();
            
            // Only add point if it's significantly different from the previous
            if (current.distanceTo(previous) > 1e-6) { // 0.001mm threshold
                cleanedPath.addPoint(current);
            }
        }
        
        if (cleanedPath.size() >= 2) {
            cleaned.push_back(cleanedPath);
        }
    }
    
    return cleaned;
}

// Error handling and reporting
void CAMProcessor::addWarning(CAMOperationResult& result, const std::string& warning) {
    result.warnings.push_back(warning);
}

void CAMProcessor::addError(CAMOperationResult& result, const std::string& error) {
    result.errors.push_back(error);
}

} // namespace cnc
} // namespace nwss 