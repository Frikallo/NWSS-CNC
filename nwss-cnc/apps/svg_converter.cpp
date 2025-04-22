#include "nwss-cnc/svg_parser.h"
#include "nwss-cnc/discretizer.h"
#include "nwss-cnc/utils.h"

#include <iostream>
#include <string>
#include <vector>

using namespace nwss::cnc;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <svg_file> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --output <file>         Output file for discretized paths (default: input.csv)" << std::endl;
    std::cout << "  --visualize <file>      Create visualization SVG (default: input.viz.svg)" << std::endl;
    std::cout << "  --bezier-samples <num>  Number of samples per bezier curve (default: 10)" << std::endl;
    std::cout << "  --adaptive <value>      Use adaptive sampling with given flatness (default: 0 = disabled)" << std::endl;
    std::cout << "  --simplify <value>      Simplify paths with given tolerance (default: 0 = disabled)" << std::endl;
    std::cout << "  --units <units>         Units for SVG parsing [mm, cm, in, px] (default: mm)" << std::endl;
    std::cout << "  --dpi <value>           DPI for unit conversion (default: 96)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    std::string svgFile = argv[1];
    std::string outputFile = Utils::replaceExtension(svgFile, "csv");
    std::string visualizeFile = Utils::replaceExtension(svgFile, "viz.svg");
    
    DiscretizerConfig config;
    std::string units = "mm";
    float dpi = 96.0f;
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "--visualize" && i + 1 < argc) {
            visualizeFile = argv[++i];
        }
        else if (arg == "--bezier-samples" && i + 1 < argc) {
            config.bezierSamples = std::stoi(argv[++i]);
        }
        else if (arg == "--adaptive" && i + 1 < argc) {
            config.adaptiveSampling = std::stod(argv[++i]);
        }
        else if (arg == "--simplify" && i + 1 < argc) {
            config.simplifyTolerance = std::stod(argv[++i]);
        }
        else if (arg == "--units" && i + 1 < argc) {
            units = argv[++i];
        }
        else if (arg == "--dpi" && i + 1 < argc) {
            dpi = std::stof(argv[++i]);
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Parse SVG file
    std::cout << "Parsing SVG file: " << svgFile << std::endl;
    SVGParser parser;
    if (!parser.loadFromFile(svgFile, units, dpi)) {
        std::cerr << "Error: Failed to parse SVG file." << std::endl;
        return 1;
    }
    
    // Get SVG dimensions
    float width, height;
    if (parser.getDimensions(width, height)) {
        std::cout << "SVG Dimensions: " << width << " x " << height << " " << units << std::endl;
    }
    
    // Create discretizer with configured settings
    Discretizer discretizer;
    discretizer.setConfig(config);
    
    // Print discretization settings
    std::cout << "Discretization settings:" << std::endl;
    std::cout << "  Bezier samples: " << config.bezierSamples << std::endl;
    if (config.adaptiveSampling > 0.0) {
        std::cout << "  Adaptive sampling: enabled (flatness=" << config.adaptiveSampling << ")" << std::endl;
    } else {
        std::cout << "  Adaptive sampling: disabled" << std::endl;
    }
    if (config.simplifyTolerance > 0.0) {
        std::cout << "  Path simplification: enabled (tolerance=" << config.simplifyTolerance << ")" << std::endl;
    } else {
        std::cout << "  Path simplification: disabled" << std::endl;
    }
    
    // Process shapes
    int shapeCount = parser.getShapeCount();
    std::cout << "\nFound " << shapeCount << " shapes:" << std::endl;
    
    std::vector<SVGShapeInfo> shapeInfoList = parser.getShapeInfo();
    for (int i = 0; i < shapeCount; i++) {
        const auto& info = shapeInfoList[i];
        std::cout << "Shape " << i << ":" << std::endl;
        std::cout << "  ID: " << (info.id.empty() ? "(unnamed)" : info.id) << std::endl;
        std::cout << "  Type: " << info.type << std::endl;
        std::cout << "  Fill: " << Utils::colorToHex(info.fillColor) << std::endl;
        std::cout << "  Stroke: " << Utils::colorToHex(info.strokeColor) << std::endl;
        std::cout << "  Stroke Width: " << info.strokeWidth << std::endl;
        std::cout << "  Bounds: [" << info.bounds[0] << ", " << info.bounds[1] 
                  << ", " << info.bounds[2] << ", " << info.bounds[3] << "]" << std::endl;
        
        // Get paths for this shape
        NSVGshape* shape = parser.getShape(i);
        std::vector<Path> shapePaths = discretizer.discretizeShape(shape);
        
        std::cout << "  Paths: " << shapePaths.size() << std::endl;
        for (size_t j = 0; j < shapePaths.size(); j++) {
            const auto& path = shapePaths[j];
            std::cout << "    Path " << j << ": " << path.size() << " points, length: " 
                      << Utils::formatNumber(path.length(), 2) << " " << units << std::endl;
            
            // Print first few points
            const auto& points = path.getPoints();
            const int maxPointsToPrint = 3;
            if (!points.empty()) {
                std::cout << "      First points: ";
                for (int k = 0; k < std::min(maxPointsToPrint, (int)points.size()); k++) {
                    if (k > 0) std::cout << ", ";
                    std::cout << "(" << Utils::formatNumber(points[k].x, 1) << "," 
                              << Utils::formatNumber(points[k].y, 1) << ")";
                }
                if (points.size() > maxPointsToPrint) {
                    std::cout << ", ...";
                }
                std::cout << std::endl;
            }
        }
    }
    
    // Discretize the entire image
    std::vector<Path> allPaths = discretizer.discretizeImage(parser.getRawImage());
    
    // Save to CSV
    std::cout << "\nSaving " << allPaths.size() << " discretized paths to: " << outputFile << std::endl;
    if (Utils::savePathsToCSV(allPaths, outputFile)) {
        std::cout << "CSV file created successfully." << std::endl;
    }
    
    // Generate visualization
    std::cout << "Generating visualization: " << visualizeFile << std::endl;
    if (Utils::generateVisualization(svgFile, allPaths, visualizeFile)) {
        std::cout << "Visualization created successfully." << std::endl;
    }
    
    // Clean up
    parser.freeImage();
    
    return 0;
}