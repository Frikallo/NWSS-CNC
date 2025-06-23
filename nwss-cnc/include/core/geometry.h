#ifndef NWSS_CNC_GEOMETRY_HPP
#define NWSS_CNC_GEOMETRY_HPP

#include <vector>
#include <cmath>

namespace nwss {
namespace cnc {

/**
 * Enumeration for different cutout modes
 */
enum class CutoutMode {
    PERIMETER,      // Cut only along the perimeter/paths (current behavior)
    PUNCHOUT,       // Cut out the entire area (punch through)
    POCKET,         // Pocket the area (cut inside the shape)
    ENGRAVE         // Engrave the area (shallow cuts)
};

/**
 * Structure to hold cutout parameters
 */
struct CutoutParams {
    CutoutMode mode = CutoutMode::PERIMETER;
    double stepover = 0.5;        // Stepover distance for area cutting (as fraction of tool diameter)
    double overlap = 0.1;         // Overlap between passes (as fraction of stepover)
    bool spiralIn = true;         // Whether to spiral inward for pocketing
    double maxStepover = 2.0;     // Maximum stepover in absolute units (mm)
    
    CutoutParams() = default;
    CutoutParams(CutoutMode m, double so = 0.5, double ol = 0.1, bool si = true, double mso = 2.0)
        : mode(m), stepover(so), overlap(ol), spiralIn(si), maxStepover(mso) {}
};

/**
 * Represents a 2D point with x and y coordinates
 */
struct Point2D {
    double x;
    double y;
    
    Point2D(double _x = 0, double _y = 0) : x(_x), y(_y) {}
    
    // Calculate distance to another point
    double distanceTo(const Point2D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx*dx + dy*dy);
    }
    
    // Operators for point manipulation
    Point2D operator+(const Point2D& other) const {
        return Point2D(x + other.x, y + other.y);
    }
    
    Point2D operator-(const Point2D& other) const {
        return Point2D(x - other.x, y - other.y);
    }
    
    Point2D operator*(double scalar) const {
        return Point2D(x * scalar, y * scalar);
    }
    
    bool operator==(const Point2D& other) const {
        // Using small epsilon for floating point comparison
        const double epsilon = 1e-6;
        return std::fabs(x - other.x) < epsilon && std::fabs(y - other.y) < epsilon;
    }
};

/**
 * Represents a path as a sequence of 2D points
 */
class Path {
public:
    Path() = default;
    explicit Path(const std::vector<Point2D>& points) : m_points(points) {}
    
    // Add a point to the path
    void addPoint(const Point2D& point) {
        m_points.push_back(point);
    }
    
    // Get all points
    const std::vector<Point2D>& getPoints() const {
        return m_points;
    }
    
    // Get a specific point
    const Point2D& getPoint(size_t index) const {
        return m_points.at(index);
    }
    
    // Get number of points
    size_t size() const {
        return m_points.size();
    }
    
    // Check if path is empty
    bool empty() const {
        return m_points.empty();
    }
    
    // Calculate the total length of the path
    double length() const;
    
    // Simplify the path by removing points that are too close together
    Path simplify(double tolerance) const;

private:
    std::vector<Point2D> m_points;
};

/**
 * Represents a closed polygon for area operations
 */
class Polygon {
public:
    Polygon() = default;
    explicit Polygon(const std::vector<Point2D>& points) : m_points(points) {}
    
    // Add a point to the polygon
    void addPoint(const Point2D& point) {
        m_points.push_back(point);
    }
    
    // Get all points
    const std::vector<Point2D>& getPoints() const {
        return m_points;
    }
    
    // Get a specific point
    const Point2D& getPoint(size_t index) const {
        return m_points.at(index);
    }
    
    // Get number of points
    size_t size() const {
        return m_points.size();
    }
    
    // Check if polygon is empty
    bool empty() const {
        return m_points.empty();
    }
    
    // Check if a point is inside the polygon
    bool containsPoint(const Point2D& point) const;
    
    // Calculate the area of the polygon
    double area() const;
    
    // Check if the polygon is clockwise
    bool isClockwise() const;
    
    // Reverse the polygon orientation
    void reverse();
    
    // Get the bounding box
    void getBounds(double& minX, double& minY, double& maxX, double& maxY) const;

private:
    std::vector<Point2D> m_points;
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_GEOMETRY_HPP