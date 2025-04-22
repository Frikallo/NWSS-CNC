#include "nwss-cnc/geometry.h"
#include <functional>

namespace nwss {
namespace cnc {

double Path::length() const {
    if (m_points.size() < 2) {
        return 0.0;
    }
    
    double totalLength = 0.0;
    for (size_t i = 1; i < m_points.size(); ++i) {
        totalLength += m_points[i-1].distanceTo(m_points[i]);
    }
    
    return totalLength;
}

Path Path::simplify(double tolerance) const {
    if (m_points.size() < 3 || tolerance <= 0.0) {
        return *this;
    }
    
    // Using a simple Douglas-Peucker algorithm for path simplification
    std::vector<bool> marked(m_points.size(), false);
    marked[0] = marked[m_points.size() - 1] = true;  // Keep endpoints
    
    // Recursive function to mark points to keep
    std::function<void(size_t, size_t)> simplifyRecursive = [&](size_t start, size_t end) {
        if (end <= start + 1) {
            return;
        }
        
        double maxDistance = 0.0;
        size_t maxIndex = start;
        
        // Line from start to end
        const Point2D& p1 = m_points[start];
        const Point2D& p2 = m_points[end];
        
        // Find point with maximum distance from the line
        for (size_t i = start + 1; i < end; ++i) {
            const Point2D& p = m_points[i];
            
            // Calculate perpendicular distance
            double lineLength = p1.distanceTo(p2);
            if (lineLength < 1e-6) {
                continue;  // Points are too close
            }
            
            double distance = std::abs((p2.y - p1.y) * p.x - (p2.x - p1.x) * p.y + 
                                      p2.x * p1.y - p2.y * p1.x) / lineLength;
            
            if (distance > maxDistance) {
                maxDistance = distance;
                maxIndex = i;
            }
        }
        
        // If max distance is greater than tolerance, mark the point and recurse
        if (maxDistance > tolerance) {
            marked[maxIndex] = true;
            simplifyRecursive(start, maxIndex);
            simplifyRecursive(maxIndex, end);
        }
    };
    
    // Start recursion
    simplifyRecursive(0, m_points.size() - 1);
    
    // Build new path with only marked points
    Path simplified;
    for (size_t i = 0; i < m_points.size(); ++i) {
        if (marked[i]) {
            simplified.addPoint(m_points[i]);
        }
    }
    
    return simplified;
}

} // namespace cnc
} // namespace nwss