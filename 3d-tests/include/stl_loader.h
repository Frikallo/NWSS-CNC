#ifndef STL_LOADER_H
#define STL_LOADER_H

#include "types.h"
#include <fstream>
#include <iostream>
#include <sstream>

class STLLoader {
public:
    STLLoader();
    ~STLLoader();
    
    // Load STL file (auto-detects binary vs ASCII)
    bool loadSTL(const std::string& filename, std::vector<Triangle>& triangles);
    
    // Get bounding box of loaded mesh
    BoundingBox getBoundingBox() const;
    
    // Get mesh statistics
    void printMeshInfo(const std::vector<Triangle>& triangles) const;

private:
    // Check if file is binary STL
    bool isBinarySTL(const std::string& filename);
    
    // Load binary STL file
    bool loadBinarySTL(const std::string& filename, std::vector<Triangle>& triangles);
    
    // Load ASCII STL file
    bool loadAsciiSTL(const std::string& filename, std::vector<Triangle>& triangles);
    
    // Read a float from binary file (little endian)
    float readFloat(std::ifstream& file);
    
    // Validate triangle (check for degenerate triangles)
    bool isValidTriangle(const Triangle& triangle) const;
    
    BoundingBox m_boundingBox;
};

#endif // STL_LOADER_H