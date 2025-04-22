/**
 * SVG to CNC Converter
 * 
 * This application converts SVG files to discretized paths for CNC machining,
 * with options for path simplification, transformation, and G-code generation.
 */

#include "nwss-cnc/svg_parser.h"
#include "nwss-cnc/discretizer.h"
#include "nwss-cnc/utils.h"
#include "nwss-cnc/config.h"
#include "nwss-cnc/transform.h"
#include "nwss-cnc/gcode_generator.h"

#include <iostream>
#include <string>
#include <vector>

using namespace nwss::cnc;

// Function declarations
void printUsage(const char* programName);
//bool generateGCode(const std::vector<Path>& paths, const CNConfig& config, const std::string& outputFile);

// Main application
int main(int argc, char* argv[]) {
    // Check for minimum arguments
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Input SVG file is the first argument
    std::string svgFile = argv[1];
    
    // Default settings
    DiscretizerConfig discretizerConfig;
    std::string outputFile = Utils::replaceExtension(svgFile, "csv");
    std::string visualizeFile = Utils::replaceExtension(svgFile, "viz.svg");
    std::string units = "mm";
    float dpi = 96.0f;
    
    // CNC settings
    std::string configFile = "nwss-cnc.cfg";
    bool preserveAspectRatio = true;
    bool centerDesign = true;
    bool flipY = true;
    bool generateGCodeOutput = false;
    std::string gCodeFile;
    bool generateMaterialViz = false;
    std::string materialVizFile;

    GCodeOptions gCodeOptions;
    
    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        // Output options
        if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "--visualize" && i + 1 < argc) {
            visualizeFile = argv[++i];
        }
        else if (arg == "--material-viz" && i + 1 < argc) {
            generateMaterialViz = true;
            materialVizFile = argv[++i];
        }
        else if (arg == "--gcode" && i + 1 < argc) {
            generateGCodeOutput = true;
            gCodeFile = argv[++i];
        }
        
        // SVG parsing options
        else if (arg == "--units" && i + 1 < argc) {
            units = argv[++i];
        }
        else if (arg == "--dpi" && i + 1 < argc) {
            dpi = std::stof(argv[++i]);
        }
        
        // Discretizer options
        else if (arg == "--bezier-samples" && i + 1 < argc) {
            discretizerConfig.bezierSamples = std::stoi(argv[++i]);
        }
        else if (arg == "--adaptive" && i + 1 < argc) {
            discretizerConfig.adaptiveSampling = std::stod(argv[++i]);
        }
        else if (arg == "--simplify" && i + 1 < argc) {
            discretizerConfig.simplifyTolerance = std::stod(argv[++i]);
        }
        
        // CNC options
        else if (arg == "--config" && i + 1 < argc) {
            configFile = argv[++i];
        }
        else if (arg == "--no-scale") {
            preserveAspectRatio = false;
        }
        else if (arg == "--no-center") {
            centerDesign = false;
        }
        else if (arg == "--no-flip-y") {
            flipY = false;
        }
        
        // Unknown option
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    //----------------------------------------
    // Step 1: Parse SVG file
    //----------------------------------------
    std::cout << "\n=== SVG Parsing ===\n";
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
    
    //----------------------------------------
    // Step 2: Set up discretizer
    //----------------------------------------
    std::cout << "\n=== Discretization Settings ===\n";
    Discretizer discretizer;
    discretizer.setConfig(discretizerConfig);
    
    // Print discretization settings
    std::cout << "  Bezier samples: " << discretizerConfig.bezierSamples << std::endl;
    if (discretizerConfig.adaptiveSampling > 0.0) {
        std::cout << "  Adaptive sampling: enabled (flatness=" << discretizerConfig.adaptiveSampling << ")" << std::endl;
    } else {
        std::cout << "  Adaptive sampling: disabled" << std::endl;
    }
    if (discretizerConfig.simplifyTolerance > 0.0) {
        std::cout << "  Path simplification: enabled (tolerance=" << discretizerConfig.simplifyTolerance << ")" << std::endl;
    } else {
        std::cout << "  Path simplification: disabled" << std::endl;
    }
    
    //----------------------------------------
    // Step 3: Process shapes and paths
    //----------------------------------------
    // std::cout << "\n=== Shape Processing ===\n";
    // int shapeCount = parser.getShapeCount();
    // std::cout << "Found " << shapeCount << " shapes:" << std::endl;
    
    // std::vector<SVGShapeInfo> shapeInfoList = parser.getShapeInfo();
    // for (int i = 0; i < shapeCount; i++) {
    //     const auto& info = shapeInfoList[i];
    //     std::cout << "Shape " << i << ":" << std::endl;
    //     std::cout << "  ID: " << (info.id.empty() ? "(unnamed)" : info.id) << std::endl;
    //     std::cout << "  Fill: " << Utils::colorToHex(info.fillColor) << std::endl;
    //     std::cout << "  Stroke: " << Utils::colorToHex(info.strokeColor) << std::endl;
    //     std::cout << "  Stroke Width: " << info.strokeWidth << std::endl;
    //     std::cout << "  Bounds: [" << info.bounds[0] << ", " << info.bounds[1] 
    //               << ", " << info.bounds[2] << ", " << info.bounds[3] << "]" << std::endl;
        
    //     // Get paths for this shape
    //     NSVGshape* shape = parser.getShape(i);
    //     std::vector<Path> shapePaths = discretizer.discretizeShape(shape);
        
    //     std::cout << "  Paths: " << shapePaths.size() << std::endl;
    //     for (size_t j = 0; j < shapePaths.size(); j++) {
    //         const auto& path = shapePaths[j];
    //         std::cout << "    Path " << j << ": " << path.size() << " points, length: " 
    //                   << Utils::formatNumber(path.length(), 2) << " " << units << std::endl;
    //     }
    // }
    
    //----------------------------------------
    // Step 4: Discretize the entire image
    //----------------------------------------
    std::cout << "\n=== Discretizing All Paths ===\n";
    std::vector<Path> allPaths = discretizer.discretizeImage(parser.getRawImage());
    std::cout << "Generated " << allPaths.size() << " discretized paths." << std::endl;
    
    //----------------------------------------
    // Step 5: Load CNC configuration
    //----------------------------------------
    std::cout << "\n=== CNC Configuration ===\n";
    CNConfig config;
    if (CNConfig::isFirstRun(configFile)) {
        std::cout << "No configuration file found. Using default settings." << std::endl;
        std::cout << "Run config-wizard to create a configuration file." << std::endl;
        config.setDefaults();
    } else if (!config.loadFromFile(configFile)) {
        std::cout << "Warning: Failed to load configuration, using defaults." << std::endl;
        config.setDefaults();
    } else {
        std::cout << "Loaded CNC configuration from: " << configFile << std::endl;
    }
    
    // Display machine and material settings
    std::cout << "Machine: " << config.getBedWidth() << " x " << config.getBedHeight() 
              << " " << config.getUnitsString() << std::endl;
    std::cout << "Material: " << config.getMaterialWidth() << " x " << config.getMaterialHeight() 
              << " x " << config.getMaterialThickness() << " " << config.getUnitsString() << std::endl;
    std::cout << "Cutting: " << config.getFeedRate() << " " << config.getUnitsString() << "/min, " 
              << "Depth: " << config.getCutDepth() << " " << config.getUnitsString() 
              << " x " << config.getPassCount() << " passes" << std::endl;
    
    //----------------------------------------
    // Step 6: Transform paths to fit material
    //----------------------------------------
    std::cout << "\n=== Path Transformation ===\n";
    
    // Display original design dimensions
    double minX, minY, maxX, maxY;
    if (Transform::getBounds(allPaths, minX, minY, maxX, maxY)) {
        double width = maxX - minX;
        double height = maxY - minY;
        std::cout << "Original design dimensions: " 
                  << Utils::formatNumber(width, 3) << " x " 
                  << Utils::formatNumber(height, 3) << " " 
                  << config.getUnitsString() << std::endl;
        std::cout << "Origin at: (" << Utils::formatNumber(minX, 3) << ", " 
                  << Utils::formatNumber(minY, 3) << ")" << std::endl;
    }
    
    // Transform paths to fit material
    TransformInfo transformInfo;
    if (Transform::fitToMaterial(allPaths, config, preserveAspectRatio, 
                                centerDesign, centerDesign, flipY, &transformInfo)) {
        std::cout << "\n" << Transform::formatTransformInfo(transformInfo, config) << std::endl;
    } else {
        std::cerr << "Error transforming paths: " << transformInfo.message << std::endl;
    }
    
    //----------------------------------------
    // Step 7: Generate output files
    //----------------------------------------
    std::cout << "\n=== Generating Output Files ===\n";
    
    // Save discretized paths to CSV
    std::cout << "Saving " << allPaths.size() << " discretized paths to: " << outputFile << std::endl;
    if (Utils::savePathsToCSV(allPaths, outputFile)) {
        std::cout << "CSV file created successfully." << std::endl;
    }
    
    // Generate path visualization
    std::cout << "Generating path visualization: " << visualizeFile << std::endl;
    if (Utils::generateVisualization(svgFile, allPaths, visualizeFile)) {
        std::cout << "Visualization created successfully." << std::endl;
    }
    
    // Generate material placement visualization if requested
    if (generateMaterialViz) {
        if (materialVizFile.empty()) {
            materialVizFile = Utils::replaceExtension(svgFile, "material.svg");
        }
        std::cout << "Generating material placement visualization: " << materialVizFile << std::endl;
        if (Utils::generateMaterialVisualization(allPaths, config, materialVizFile)) {
            std::cout << "Material visualization created successfully." << std::endl;
        }
    }
    
    // Generate G-code if requested
    if (generateGCodeOutput) {
        if (gCodeFile.empty()) {
            gCodeFile = Utils::replaceExtension(svgFile, "gcode");
        }
        
        std::cout << "Generating G-code to: " << gCodeFile << std::endl;
        
        // Use the dedicated G-code generator
        GCodeGenerator gCodeGen;
        gCodeGen.setConfig(config);
        gCodeGen.setOptions(gCodeOptions);
        
        if (gCodeGen.generateGCode(allPaths, gCodeFile)) {
            std::cout << "G-code file created successfully." << std::endl;
        } else {
            std::cerr << "Error generating G-code file." << std::endl;
        }
    }
    
    // Clean up
    parser.freeImage();
    std::cout << "\nProcessing complete." << std::endl;
    
    return 0;
}

/**
 * Print application usage information
 */
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <svg_file> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    
    // Output options
    std::cout << "  Output options:" << std::endl;
    std::cout << "  --output <file>         Output CSV file for discretized paths (default: input.csv)" << std::endl;
    std::cout << "  --visualize <file>      Create visualization SVG (default: input.viz.svg)" << std::endl;
    std::cout << "  --material-viz <file>   Generate material placement visualization" << std::endl;
    std::cout << "  --gcode <file>          Generate G-code output (requires config)" << std::endl;
    
    // SVG parsing options
    std::cout << "\n  SVG parsing options:" << std::endl;
    std::cout << "  --units <units>         Units for SVG parsing [mm, cm, in, px] (default: mm)" << std::endl;
    std::cout << "  --dpi <value>           DPI for unit conversion (default: 96)" << std::endl;
    
    // Discretization options
    std::cout << "\n  Discretization options:" << std::endl;
    std::cout << "  --bezier-samples <num>  Number of samples per bezier curve (default: 10)" << std::endl;
    std::cout << "  --adaptive <value>      Use adaptive sampling with given flatness (default: 0 = disabled)" << std::endl;
    std::cout << "  --simplify <value>      Simplify paths with given tolerance (default: 0 = disabled)" << std::endl;
    
    // CNC options
    std::cout << "\n  CNC options:" << std::endl;
    std::cout << "  --config <file>         Load CNC configuration from file (default: nwss-cnc.cfg)" << std::endl;
    std::cout << "  --no-scale              Do not scale the design to fit material" << std::endl;
    std::cout << "  --no-center             Do not center the design on material" << std::endl;
    std::cout << "  --no-flip-y             Do not flip Y coordinates for CNC orientation" << std::endl;
}