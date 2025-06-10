#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <string>
#include <memory>

// 3D Point structure
struct Point3D {
    double x, y, z;
    
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double x, double y, double z) : x(x), y(y), z(z) {}
    
    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }
    
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    Point3D operator*(double scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }
    
    double dot(const Point3D& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Point3D cross(const Point3D& other) const {
        return Point3D(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    double magnitude() const {
        return sqrt(x * x + y * y + z * z);
    }
    
    Point3D normalize() const {
        double mag = magnitude();
        if (mag == 0) return Point3D();
        return Point3D(x / mag, y / mag, z / mag);
    }
};

// Triangle structure for STL mesh
struct Triangle {
    Point3D vertices[3];
    Point3D normal;
    
    Triangle() {}
    Triangle(const Point3D& v1, const Point3D& v2, const Point3D& v3) {
        vertices[0] = v1;
        vertices[1] = v2;
        vertices[2] = v3;
        calculateNormal();
    }
    
    void calculateNormal() {
        Point3D edge1 = vertices[1] - vertices[0];
        Point3D edge2 = vertices[2] - vertices[0];
        normal = edge1.cross(edge2).normalize();
    }
    
    Point3D getCenter() const {
        return (vertices[0] + vertices[1] + vertices[2]) * (1.0/3.0);
    }
};

// Material properties
struct Material {
    double width;
    double length;
    double height;
    std::string name;
    
    Material(double w, double l, double h, const std::string& n = "default") 
        : width(w), length(l), height(h), name(n) {}
};

// Tool properties
struct Tool {
    double diameter;
    double length;
    double stepdown;
    double feedrate;
    double spindleSpeed;
    
    Tool(double d = 6.0, double l = 50.0, double s = 1.0, double f = 1000.0, double sp = 12000.0)
        : diameter(d), length(l), stepdown(s), feedrate(f), spindleSpeed(sp) {}
};

// Machining parameters
struct MachiningParams {
    Tool tool;
    Material material;
    double safetyHeight;
    double retractHeight;
    double minDraftAngle;  // Minimum draft angle in degrees
    
    MachiningParams(const Tool& t, const Material& m, double sh = 10.0, double rh = 5.0, double da = 1.0)
        : tool(t), material(m), safetyHeight(sh), retractHeight(rh), minDraftAngle(da) {}
};

// G-code command structure
struct GCodeCommand {
    std::string command;
    double x, y, z, f;
    bool hasX, hasY, hasZ, hasF;
    
    GCodeCommand(const std::string& cmd) : command(cmd), x(0), y(0), z(0), f(0),
                                          hasX(false), hasY(false), hasZ(false), hasF(false) {}
};

// Toolpath point
struct ToolpathPoint {
    Point3D position;
    double feedrate;
    bool isRapid;
    
    ToolpathPoint(const Point3D& pos, double feed = 0, bool rapid = false)
        : position(pos), feedrate(feed), isRapid(rapid) {}
};

// Validation result
struct ValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    ValidationResult() : isValid(true) {}
    
    void addError(const std::string& error) {
        errors.push_back(error);
        isValid = false;
    }
    
    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

// Bounding box
struct BoundingBox {
    Point3D min, max;
    
    BoundingBox() {
        min = Point3D(1e6, 1e6, 1e6);
        max = Point3D(-1e6, -1e6, -1e6);
    }
    
    void update(const Point3D& point) {
        if (point.x < min.x) min.x = point.x;
        if (point.y < min.y) min.y = point.y;
        if (point.z < min.z) min.z = point.z;
        if (point.x > max.x) max.x = point.x;
        if (point.y > max.y) max.y = point.y;
        if (point.z > max.z) max.z = point.z;
    }
    
    Point3D getSize() const {
        return max - min;
    }
    
    Point3D getCenter() const {
        return (min + max) * 0.5;
    }
};

#endif // TYPES_H