#include "nwss-cnc/utils.h"
#include "nwss-cnc/svg_parser.h"
#include "nwss-cnc/config.h"
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

bool Utils::generateMaterialVisualization(const std::vector<Path>& paths, 
                                        const CNConfig& config,
                                        const std::string& outputFile) {
    double materialWidth = config.getMaterialWidth();
    double materialHeight = config.getMaterialHeight();
    double bedWidth = config.getBedWidth();
    double bedHeight = config.getBedHeight();
    std::string units = config.getUnitsString();
    
    // Create SVG file
    std::ofstream vizFile(outputFile);
    if (!vizFile.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << outputFile << std::endl;
        return false;
    }
    
    // Get maximum dimensions to show
    double maxWidth = std::max(materialWidth, bedWidth) * 1.1;  // 10% margin
    double maxHeight = std::max(materialHeight, bedHeight) * 1.1;  // 10% margin
    
    // Generate SVG file with bed, material, and design visualization
    vizFile << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    vizFile << "<svg width=\"" << maxWidth << units << "\" height=\"" << maxHeight 
            << units << "\" viewBox=\"" << -maxWidth * 0.05 << " " << -maxHeight * 0.05 
            << " " << maxWidth << " " << maxHeight 
            << "\" xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
    
    // Add title
    vizFile << "  <title>NWSS CNC Material and Cut Visualization</title>" << std::endl;
    
    // Draw CNC bed outline
    vizFile << "  <!-- CNC Bed -->" << std::endl;
    vizFile << "  <rect x=\"0\" y=\"0\" width=\"" << bedWidth << "\" height=\"" << bedHeight 
            << "\" fill=\"#f0f0f0\" stroke=\"#888888\" stroke-width=\"1\" />" << std::endl;
    
    // Add bed label
    vizFile << "  <text x=\"" << bedWidth / 2 << "\" y=\"" << bedHeight * 0.1 
            << "\" font-family=\"Arial\" font-size=\"10\" text-anchor=\"middle\">"
            << "CNC Bed (" << bedWidth << " x " << bedHeight << " " << units << ")</text>" << std::endl;
    
    // Draw material outline
    vizFile << "  <!-- Material -->" << std::endl;
    vizFile << "  <rect x=\"0\" y=\"0\" width=\"" << materialWidth << "\" height=\"" << materialHeight 
            << "\" fill=\"#e0e0e0\" stroke=\"#444444\" stroke-width=\"1\" />" << std::endl;
    
    // Add material label
    vizFile << "  <text x=\"" << materialWidth / 2 << "\" y=\"" << materialHeight * 0.2 
            << "\" font-family=\"Arial\" font-size=\"8\" text-anchor=\"middle\">"
            << "Material (" << materialWidth << " x " << materialHeight << " " << units << ")</text>" << std::endl;
    
    // Draw coordinate system
    vizFile << "  <!-- Coordinate System -->" << std::endl;
    vizFile << "  <line x1=\"0\" y1=\"0\" x2=\"20\" y2=\"0\" stroke=\"red\" stroke-width=\"0.5\" />" << std::endl;
    vizFile << "  <line x1=\"0\" y1=\"0\" x2=\"0\" y2=\"20\" stroke=\"green\" stroke-width=\"0.5\" />" << std::endl;
    vizFile << "  <text x=\"22\" y=\"4\" font-family=\"Arial\" font-size=\"6\" fill=\"red\">X</text>" << std::endl;
    vizFile << "  <text x=\"2\" y=\"22\" font-family=\"Arial\" font-size=\"6\" fill=\"green\">Y</text>" << std::endl;
    
    // Draw paths (the actual cut design)
    vizFile << "  <!-- Cut Paths -->" << std::endl;
    vizFile << "  <g fill=\"none\" stroke=\"blue\" stroke-width=\"0.75\">" << std::endl;
    
    // Draw each path with points
    for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
        const auto& points = paths[pathIndex].getPoints();
        if (points.empty()) continue;
        
        // Draw polyline for the path
        vizFile << "    <polyline points=\"";
        for (const auto& point : points) {
            vizFile << point.x << "," << point.y << " ";
        }
        vizFile << "\" />" << std::endl;
        
        // Draw dots at each point
        for (size_t i = 0; i < points.size(); i++) {
            // Only draw dots for start and end points and every 10th point to reduce clutter
            if (i == 0 || i == points.size() - 1 || i % 10 == 0) {
                vizFile << "    <circle cx=\"" << points[i].x << "\" cy=\"" << points[i].y 
                        << "\" r=\"0.6\" fill=\"" << (i == 0 ? "green" : (i == points.size() - 1 ? "red" : "blue")) << "\" />" << std::endl;
            }
        }
    }
    
    vizFile << "  </g>" << std::endl;
    
    // Add legend
    vizFile << "  <!-- Legend -->" << std::endl;
    vizFile << "  <g transform=\"translate(" << maxWidth * 0.7 << ", " << maxHeight * 0.8 << ")\">" << std::endl;
    vizFile << "    <rect x=\"0\" y=\"0\" width=\"" << maxWidth * 0.25 << "\" height=\"" << maxHeight * 0.15 
            << "\" fill=\"white\" stroke=\"black\" stroke-width=\"0.5\" />" << std::endl;
    vizFile << "    <text x=\"5\" y=\"10\" font-family=\"Arial\" font-size=\"6\">Legend:</text>" << std::endl;
    vizFile << "    <circle cx=\"7\" cy=\"20\" r=\"0.6\" fill=\"green\" />" << std::endl;
    vizFile << "    <text x=\"12\" y=\"22\" font-family=\"Arial\" font-size=\"6\">Start points</text>" << std::endl;
    vizFile << "    <circle cx=\"7\" cy=\"30\" r=\"0.6\" fill=\"red\" />" << std::endl;
    vizFile << "    <text x=\"12\" y=\"32\" font-family=\"Arial\" font-size=\"6\">End points</text>" << std::endl;
    vizFile << "    <polyline points=\"5,40 10,40\" stroke=\"blue\" stroke-width=\"0.75\" />" << std::endl;
    vizFile << "    <text x=\"12\" y=\"42\" font-family=\"Arial\" font-size=\"6\">Cut paths</text>" << std::endl;
    vizFile << "  </g>" << std::endl;
    
    // Close the SVG file
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