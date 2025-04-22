#ifndef NWSS_CNC_TRANSFORM_H
#define NWSS_CNC_TRANSFORM_H

#include "nwss-cnc/geometry.h"
#include "nwss-cnc/config.h"
#include <vector>
#include <string>

namespace nwss {
namespace cnc {

/**
 * Structure to hold information about a transform operation
 */
struct TransformInfo {
    // Original bounds
    double origWidth;
    double origHeight;
    double origMinX;
    double origMinY;
    
    // Transformed bounds
    double newWidth;
    double newHeight;
    double newMinX;
    double newMinY;
    
    // Transformation factors
    double scaleX;
    double scaleY;
    double offsetX;
    double offsetY;
    
    // Transformation result
    bool success;
    bool wasScaled;
    bool wasCropped;
    std::string message;
};

/**
 * Class to handle transformations of paths to fit constraints
 */
class Transform {
public:
    /**
     * Get the bounding box of a set of paths
     * 
     * @param paths The paths to analyze
     * @param minX Output parameter for minimum X coordinate
     * @param minY Output parameter for minimum Y coordinate
     * @param maxX Output parameter for maximum X coordinate
     * @param maxY Output parameter for maximum Y coordinate
     * @return true if bounds were calculated successfully
     */
    static bool getBounds(const std::vector<Path>& paths, 
                          double& minX, double& minY, 
                          double& maxX, double& maxY);

    /**
     * Scale and translate paths to fit within material bounds
     * 
     * @param paths The paths to transform (modified in place)
     * @param config The machine configuration with constraints
     * @param preserveAspectRatio Whether to maintain original aspect ratio
     * @param centerX Whether to center horizontally
     * @param centerY Whether to center vertically
     * @param flipY Whether to flip the Y coordinates (for correct CNC orientation)
     * @param info Output parameter for transformation information
     * @return true if transformation was successful
     */
    static bool fitToMaterial(std::vector<Path>& paths, 
                             const CNConfig& config, 
                             bool preserveAspectRatio = true,
                             bool centerX = true, 
                             bool centerY = true,
                             bool flipY = true,
                             TransformInfo* info = nullptr);

    /**
     * Format the transform information into a human-readable string
     * 
     * @param info The transformation information
     * @param config The machine configuration for unit information
     * @return A string describing the transformation
     */
    static std::string formatTransformInfo(const TransformInfo& info, 
                                          const CNConfig& config);
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_TRANSFORM_H