#include "nwss-cnc/discretizer.h"
#include "third_party/nanosvg.h"

namespace nwss {
namespace cnc {

Discretizer::Discretizer() = default;
Discretizer::~Discretizer() = default;

void Discretizer::setConfig(const DiscretizerConfig& config) {
    m_config = config;
}

const DiscretizerConfig& Discretizer::getConfig() const {
    return m_config;
}

Point2D Discretizer::evaluateBezier(float x0, float y0, float x1, float y1, 
                                float x2, float y2, float x3, float y3, float t) const {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    // Cubic bezier formula
    float x = uuu * x0 + 3 * uu * t * x1 + 3 * u * tt * x2 + ttt * x3;
    float y = uuu * y0 + 3 * uu * t * y1 + 3 * u * tt * y2 + ttt * y3;
    
    return Point2D(x, y);
}

double Discretizer::calculateFlatness(float x0, float y0, float x1, float y1,
                                   float x2, float y2, float x3, float y3) const {
    // Calculate the maximum distance between the control points and the line from start to end
    double ux = 3.0 * x1 - 2.0 * x0 - x3;
    double uy = 3.0 * y1 - 2.0 * y0 - y3;
    double vx = 3.0 * x2 - 2.0 * x3 - x0;
    double vy = 3.0 * y2 - 2.0 * y3 - y0;
    
    ux = ux * ux;
    uy = uy * uy;
    vx = vx * vx;
    vy = vy * vy;
    
    if (ux < vx) ux = vx;
    if (uy < vy) uy = vy;
    
    return ux + uy;
}

void Discretizer::adaptiveSample(float x0, float y0, float x1, float y1,
                              float x2, float y2, float x3, float y3,
                              Path& path, double flatnessTolerance) const {
    // Check the flatness of the curve
    double flatness = calculateFlatness(x0, y0, x1, y1, x2, y2, x3, y3);
    
    if (flatness <= flatnessTolerance) {
        // If the curve is flat enough, just add the endpoint
        path.addPoint(Point2D(x3, y3));
    } else {
        // Otherwise, split the curve in half and recurse
        float x01 = (x0 + x1) / 2;
        float y01 = (y0 + y1) / 2;
        float x12 = (x1 + x2) / 2;
        float y12 = (y1 + y2) / 2;
        float x23 = (x2 + x3) / 2;
        float y23 = (y2 + y3) / 2;
        
        float x012 = (x01 + x12) / 2;
        float y012 = (y01 + y12) / 2;
        float x123 = (x12 + x23) / 2;
        float y123 = (y12 + y23) / 2;
        
        float x0123 = (x012 + x123) / 2;
        float y0123 = (y012 + y123) / 2;
        
        // Recurse on the two halves
        adaptiveSample(x0, y0, x01, y01, x012, y012, x0123, y0123, path, flatnessTolerance);
        adaptiveSample(x0123, y0123, x123, y123, x23, y23, x3, y3, path, flatnessTolerance);
    }
}

Path Discretizer::discretizeBezier(float* points, int numPoints) const {
    Path result;
    
    // For each bezier segment in the path
    for (int i = 0; i < numPoints-1; i += 3) {
        float* p = &points[i*2];
        
        // Start point
        float x0 = p[0];
        float y0 = p[1];
        // Control point 1
        float x1 = p[2];
        float y1 = p[3];
        // Control point 2
        float x2 = p[4];
        float y2 = p[5];
        // End point
        float x3 = p[6];
        float y3 = p[7];
        
        // Add the start point (only for the first segment)
        if (i == 0) {
            result.addPoint(Point2D(x0, y0));
        }
        
        if (m_config.adaptiveSampling > 0.0) {
            // Use adaptive sampling based on curvature
            double flatnessTolerance = m_config.adaptiveSampling;
            adaptiveSample(x0, y0, x1, y1, x2, y2, x3, y3, result, flatnessTolerance);
        } else {
            // Use fixed sampling
            int samples = m_config.bezierSamples;
            for (int j = 1; j <= samples; j++) {
                float t = (float)j / samples;
                result.addPoint(evaluateBezier(x0, y0, x1, y1, x2, y2, x3, y3, t));
            }
        }
    }
    
    // Apply path simplification if enabled
    if (m_config.simplifyTolerance > 0.0) {
        return result.simplify(m_config.simplifyTolerance);
    }
    
    return result;
}

Path Discretizer::discretizePath(NSVGpath* path) const {
    if (!path) {
        return Path();
    }
    
    return discretizeBezier(path->pts, path->npts);
}

std::vector<Path> Discretizer::discretizeShape(NSVGshape* shape) const {
    std::vector<Path> paths;
    
    if (!shape) {
        return paths;
    }
    
    for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
        paths.push_back(discretizePath(path));
    }
    
    return paths;
}

std::vector<Path> Discretizer::discretizeImage(NSVGimage* image) const {
    std::vector<Path> allPaths;
    
    if (!image) {
        return allPaths;
    }
    
    for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
        auto shapePaths = discretizeShape(shape);
        allPaths.insert(allPaths.end(), shapePaths.begin(), shapePaths.end());
    }
    
    return allPaths;
}

} // namespace cnc
} // namespace nwss