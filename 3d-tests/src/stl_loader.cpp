#include "stl_loader.h"
#include <algorithm>
#include <cstring>

STLLoader::STLLoader() {}

STLLoader::~STLLoader() {}

bool STLLoader::loadSTL(const std::string& filename, std::vector<Triangle>& triangles) {
    triangles.clear();
    m_boundingBox = BoundingBox();
    
    if (isBinarySTL(filename)) {
        std::cout << "Loading binary STL file: " << filename << std::endl;
        return loadBinarySTL(filename, triangles);
    } else {
        std::cout << "Loading ASCII STL file: " << filename << std::endl;
        return loadAsciiSTL(filename, triangles);
    }
}

bool STLLoader::isBinarySTL(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Read first 5 bytes to check for "solid" keyword
    char header[6];
    file.read(header, 5);
    header[5] = '\0';
    
    // If it starts with "solid", it might be ASCII, but we need to check further
    if (std::string(header) == "solid") {
        // Skip to byte 80 and read triangle count
        file.seekg(80);
        uint32_t triangleCount;
        file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
        
        // Calculate expected file size for binary STL
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        size_t expectedSize = 80 + 4 + triangleCount * 50; // Header + count + triangles
        
        return (fileSize == expectedSize);
    }
    
    return true; // Assume binary if doesn't start with "solid"
}

bool STLLoader::loadBinarySTL(const std::string& filename, std::vector<Triangle>& triangles) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    // Skip 80-byte header
    file.seekg(80);
    
    // Read triangle count
    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
    
    std::cout << "Triangle count: " << triangleCount << std::endl;
    triangles.reserve(triangleCount);
    
    for (uint32_t i = 0; i < triangleCount; ++i) {
        Triangle triangle;
        
        // Read normal vector
        triangle.normal.x = readFloat(file);
        triangle.normal.y = readFloat(file);
        triangle.normal.z = readFloat(file);
        
        // Read vertices
        for (int j = 0; j < 3; ++j) {
            triangle.vertices[j].x = readFloat(file);
            triangle.vertices[j].y = readFloat(file);
            triangle.vertices[j].z = readFloat(file);
            
            // Update bounding box
            m_boundingBox.update(triangle.vertices[j]);
        }
        
        // Skip attribute byte count (2 bytes)
        file.seekg(2, std::ios::cur);
        
        if (isValidTriangle(triangle)) {
            triangles.push_back(triangle);
        }
    }
    
    file.close();
    return true;
}

bool STLLoader::loadAsciiSTL(const std::string& filename, std::vector<Triangle>& triangles) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        if (keyword == "facet") {
            Triangle triangle;
            
            // Read normal
            std::string normal_keyword;
            iss >> normal_keyword; // should be "normal"
            iss >> triangle.normal.x >> triangle.normal.y >> triangle.normal.z;
            
            // Skip "outer loop" line
            std::getline(file, line);
            
            // Read three vertices
            for (int i = 0; i < 3; ++i) {
                std::getline(file, line);
                std::istringstream vertex_iss(line);
                std::string vertex_keyword;
                vertex_iss >> vertex_keyword; // should be "vertex"
                vertex_iss >> triangle.vertices[i].x >> triangle.vertices[i].y >> triangle.vertices[i].z;
                
                // Update bounding box
                m_boundingBox.update(triangle.vertices[i]);
            }
            
            // Skip "endloop" and "endfacet" lines
            std::getline(file, line);
            std::getline(file, line);
            
            if (isValidTriangle(triangle)) {
                triangles.push_back(triangle);
            }
        }
    }
    
    file.close();
    return true;
}

float STLLoader::readFloat(std::ifstream& file) {
    float value;
    file.read(reinterpret_cast<char*>(&value), sizeof(float));
    return value;
}

bool STLLoader::isValidTriangle(const Triangle& triangle) const {
    const double EPSILON = 1e-9;
    
    // Check for degenerate triangle (zero area)
    Point3D edge1 = triangle.vertices[1] - triangle.vertices[0];
    Point3D edge2 = triangle.vertices[2] - triangle.vertices[0];
    Point3D cross = edge1.cross(edge2);
    
    return cross.magnitude() > EPSILON;
}

BoundingBox STLLoader::getBoundingBox() const {
    return m_boundingBox;
}

void STLLoader::printMeshInfo(const std::vector<Triangle>& triangles) const {
    std::cout << "\n=== Mesh Information ===" << std::endl;
    std::cout << "Triangle count: " << triangles.size() << std::endl;
    
    Point3D size = m_boundingBox.getSize();
    std::cout << "Bounding box:" << std::endl;
    std::cout << "  Min: (" << m_boundingBox.min.x << ", " << m_boundingBox.min.y << ", " << m_boundingBox.min.z << ")" << std::endl;
    std::cout << "  Max: (" << m_boundingBox.max.x << ", " << m_boundingBox.max.y << ", " << m_boundingBox.max.z << ")" << std::endl;
    std::cout << "  Size: " << size.x << " x " << size.y << " x " << size.z << std::endl;
    std::cout << "========================\n" << std::endl;
}