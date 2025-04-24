#ifndef NWSS_CNC_DISCRETIZER_HPP
#define NWSS_CNC_DISCRETIZER_HPP

#include "core/geometry.h"

struct NSVGimage;
struct NSVGshape;
struct NSVGpath;

namespace nwss {
namespace cnc {

/**
 * Configuration for path discretization
 */
struct DiscretizerConfig {
    // Number of points to sample along a bezier curve
    int bezierSamples = 10;
    
    // Tolerance for path simplification (0 to disable)
    double simplifyTolerance = 0.0;
    
    // Adaptive sampling based on curvature (0 to disable)
    double adaptiveSampling = 0.0;
    
    // Maximum distance between points when using adaptive sampling
    double maxPointDistance = 1.0;
};

/**
 * Class to handle the discretization of SVG paths
 */
class Discretizer {
public:
    Discretizer();
    ~Discretizer();
    
    // Set configuration options
    void setConfig(const DiscretizerConfig& config);
    
    // Get current configuration
    const DiscretizerConfig& getConfig() const;
    
    // Discretize a cubic bezier curve
    Path discretizeBezier(float* points, int numPoints) const;
    
    // Discretize a complete SVG path
    Path discretizePath(NSVGpath* path) const;
    
    // Discretize all paths in an SVG shape
    std::vector<Path> discretizeShape(NSVGshape* shape) const;
    
    // Discretize all shapes in an SVG image
    std::vector<Path> discretizeImage(NSVGimage* image) const;
    
private:
    DiscretizerConfig m_config;
    
    // Evaluate a point on a cubic bezier curve at parameter t
    Point2D evaluateBezier(float x0, float y0, float x1, float y1, 
                         float x2, float y2, float x3, float y3, float t) const;
    
    // Calculate the "flatness" of a bezier curve segment (for adaptive sampling)
    double calculateFlatness(float x0, float y0, float x1, float y1,
                           float x2, float y2, float x3, float y3) const;
    
    // Adaptively sample a bezier curve based on curvature
    void adaptiveSample(float x0, float y0, float x1, float y1,
                      float x2, float y2, float x3, float y3,
                      Path& path, double flatnessTolerance) const;
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_DISCRETIZER_HPP