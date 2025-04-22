#include "nwss-cnc/utils.h"
#include "nwss-cnc/svg_parser.h"
#include "third_party/nanosvg.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace nwss {
namespace cnc {

bool Utils::savePathsToCSV(const std::vector<Path>& paths, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return false;
    }
    
    outFile << "# Discretized SVG Paths" << std::endl;
    outFile << "# Format: path_index,point_index,x,y" << std::endl;
    
    for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
        const auto& points = paths[pathIndex].getPoints();
        outFile << "# Path " << pathIndex << " (" << points.size() << " points)" << std::endl;
        
        for (size_t pointIndex = 0; pointIndex < points.size(); pointIndex++) {
            outFile << pathIndex << "," << pointIndex << "," 
                    << points[pointIndex].x << "," << points[pointIndex].y << std::endl;
        }
        
        outFile << std::endl; // Empty line between paths
    }
    
    outFile.close();
    return true;
}

bool Utils::generateVisualization(const std::string& sourceFile, 
                                const std::vector<Path>& paths,
                                const std::string& outputFile) {
    // Parse the original SVG to get dimensions and shapes
    SVGParser parser;
    if (!parser.loadFromFile(sourceFile)) {
        std::cerr << "Error: Could not load source SVG file for visualization." << std::endl;
        return false;
    }
    
    float width, height;
    if (!parser.getDimensions(width, height)) {
        std::cerr << "Error: Could not get SVG dimensions." << std::endl;
        return false;
    }
    
    std::ofstream vizFile(outputFile);
    if (!vizFile.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << outputFile << std::endl;
        return false;
    }
    
    // Generate a new SVG file with original shapes and discretized points
    vizFile << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    vizFile << "<svg width=\"" << width << "mm\" height=\"" << height 
            << "mm\" viewBox=\"0 0 " << width << " " << height 
            << "\" xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
    
    // Get the raw NSVGimage to draw original shapes
    NSVGimage* image = parser.getRawImage();
    
    // Draw original shapes with 50% opacity
    vizFile << "  <!-- Original shapes with 50% opacity -->" << std::endl;
    vizFile << "  <g opacity=\"0.5\">" << std::endl;
    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {
            vizFile << "    <path d=\"";
            
            // Convert NanoSVG path points to SVG path data
            for (int i = 0; i < path->npts-1; i += 3) {
                float* p = &path->pts[i*2];
                if (i == 0) {
                    vizFile << "M" << p[0] << "," << p[1] << " ";
                }
                vizFile << "C" << p[2] << "," << p[3] << " "
                        << p[4] << "," << p[5] << " "
                        << p[6] << "," << p[7] << " ";
            }
            
            // Use the original shape's fill and stroke
            unsigned int fillColor = shape->fill.color;
            unsigned int strokeColor = shape->stroke.color;
            vizFile << "\" fill=\"" << colorToHex(fillColor) 
                    << "\" stroke=\"" << colorToHex(strokeColor) 
                    << "\" stroke-width=\"" << shape->strokeWidth << "\" />" << std::endl;
        }
    }
    vizFile << "  </g>" << std::endl;
    
    // Draw discretized points
    vizFile << "  <!-- Discretized points -->" << std::endl;
    for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
        const auto& points = paths[pathIndex].getPoints();
        
        // Draw lines connecting the discretized points
        vizFile << "  <polyline points=\"";
        for (const auto& point : points) {
            vizFile << point.x << "," << point.y << " ";
        }
        vizFile << "\" fill=\"none\" stroke=\"red\" stroke-width=\"0.5\" />" << std::endl;
        
        // Draw dots at each point
        for (const auto& point : points) {
            vizFile << "  <circle cx=\"" << point.x << "\" cy=\"" << point.y 
                    << "\" r=\"0.5\" fill=\"blue\" />" << std::endl;
        }
    }
    
    vizFile << "</svg>" << std::endl;
    vizFile.close();
    return true;
}

std::string Utils::colorToHex(uint32_t color) {
    std::stringstream ss;
    ss << "#" << std::hex << (color & 0xFFFFFF);
    return ss.str();
}

std::string Utils::formatNumber(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

std::string Utils::getFileExtension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }
    return path.substr(pos + 1);
}

std::string Utils::getBaseName(const std::string& path) {
    // Find the last directory separator
    size_t lastSeparator = path.find_last_of("/\\");
    std::string fileName = (lastSeparator == std::string::npos) ? path : path.substr(lastSeparator + 1);
    
    // Remove extension
    size_t lastDot = fileName.find_last_of('.');
    if (lastDot != std::string::npos) {
        fileName = fileName.substr(0, lastDot);
    }
    
    return fileName;
}

std::string Utils::replaceExtension(const std::string& path, const std::string& newExtension) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {
        return path + "." + newExtension;
    }
    return path.substr(0, pos + 1) + newExtension;
}

} // namespace cnc
} // namespace nwss