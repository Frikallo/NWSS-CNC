#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "stl_loader.h"
#include "geometry_analyzer.h"
#include "validation_engine.h"
#include "toolpath_generator.h"
#include "gcode_generator.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] <input.stl> <output.nc>" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -w, --width <value>       Material width (mm)" << std::endl;
    std::cout << "  -l, --length <value>      Material length (mm)" << std::endl;
    std::cout << "  -h, --height <value>      Material height (mm)" << std::endl;
    std::cout << "  -d, --diameter <value>    Tool diameter (mm, default: 6.0)" << std::endl;
    std::cout << "  -s, --stepdown <value>    Stepdown per pass (mm, default: 1.0)" << std::endl;
    std::cout << "  -f, --feedrate <value>    Feed rate (mm/min, default: 1000)" << std::endl;
    std::cout << "  -r, --spindle <value>     Spindle speed (RPM, default: 12000)" << std::endl;
    std::cout << "  -a, --draft-angle <value> Minimum draft angle (degrees, default: 1.0)" << std::endl;
    std::cout << "  --validate-only           Only validate, don't generate G-code" << std::endl;
    std::cout << "  --help                    Show this help message" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << programName << " -w 100 -l 150 -h 25 part.stl output.nc" << std::endl;
}

bool parseArguments(int argc, char* argv[], std::string& inputFile, std::string& outputFile,
                   MachiningParams& params, bool& validateOnly) {
    if (argc < 3) {
        return false;
    }
    
    // Set defaults
    Tool defaultTool;
    Material defaultMaterial(0, 0, 0); // Will be set by user
    params = MachiningParams(defaultTool, defaultMaterial);
    validateOnly = false;
    
    bool hasMaterialSize = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            return false;
        } else if (arg == "--validate-only") {
            validateOnly = true;
        } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            params.material.width = std::stod(argv[++i]);
            hasMaterialSize = true;
        } else if ((arg == "-l" || arg == "--length") && i + 1 < argc) {
            params.material.length = std::stod(argv[++i]);
            hasMaterialSize = true;
        } else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
            params.material.height = std::stod(argv[++i]);
            hasMaterialSize = true;
        } else if ((arg == "-d" || arg == "--diameter") && i + 1 < argc) {
            params.tool.diameter = std::stod(argv[++i]);
        } else if ((arg == "-s" || arg == "--stepdown") && i + 1 < argc) {
            params.tool.stepdown = std::stod(argv[++i]);
        } else if ((arg == "-f" || arg == "--feedrate") && i + 1 < argc) {
            params.tool.feedrate = std::stod(argv[++i]);
        } else if ((arg == "-r" || arg == "--spindle") && i + 1 < argc) {
            params.tool.spindleSpeed = std::stod(argv[++i]);
        } else if ((arg == "-a" || arg == "--draft-angle") && i + 1 < argc) {
            params.minDraftAngle = std::stod(argv[++i]);
        } else if (arg[0] != '-') {
            if (inputFile.empty()) {
                inputFile = arg;
            } else if (outputFile.empty()) {
                outputFile = arg;
            }
        }
    }
    
    if (inputFile.empty() || (!validateOnly && outputFile.empty()) || !hasMaterialSize) {
        return false;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "==================================" << std::endl;
    std::cout << "STL to G-Code Converter v1.0" << std::endl;
    std::cout << "CNC Milling Toolpath Generator" << std::endl;
    std::cout << "==================================" << std::endl;
    
    // Parse command line arguments
    std::string inputFile, outputFile;
    MachiningParams params(Tool(), Material(0, 0, 0));
    bool validateOnly = false;
    
    if (!parseArguments(argc, argv, inputFile, outputFile, params, validateOnly)) {
        printUsage(argv[0]);
        return 1;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // Load STL file
        std::cout << "\n1. Loading STL file..." << std::endl;
        STLLoader loader;
        std::vector<Triangle> triangles;
        
        if (!loader.loadSTL(inputFile, triangles)) {
            std::cerr << "Error: Failed to load STL file: " << inputFile << std::endl;
            return 1;
        }
        
        if (triangles.empty()) {
            std::cerr << "Error: No triangles found in STL file" << std::endl;
            return 1;
        }
        
        loader.printMeshInfo(triangles);
        
        // Validate geometry for CNC machining
        std::cout << "2. Validating geometry..." << std::endl;
        ValidationEngine validator;
        ValidationResult result = validator.validateForMachining(triangles, params);
        
        validator.printValidationReport(result);
        
        // Generate recommendations
        std::vector<std::string> recommendations = validator.generateRecommendations(result, triangles, params);
        if (!recommendations.empty()) {
            std::cout << "=== Recommendations ===" << std::endl;
            for (size_t i = 0; i < recommendations.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << recommendations[i] << std::endl;
            }
            std::cout << "========================\n" << std::endl;
        }
        
        // if (!result.isValid) {
        //     std::cout << "❌ Validation failed. Please fix the errors before proceeding." << std::endl;
        //     return 0;
        // }
        
        // if (validateOnly) {
        //     std::cout << "✅ Validation complete. Part is suitable for CNC machining." << std::endl;
        //     return 0;
        // }
        
        // Generate toolpaths
        std::cout << "3. Generating toolpaths..." << std::endl;
        ToolpathGenerator pathGen;
        std::vector<ToolpathPoint> toolpaths = pathGen.generateToolpaths(triangles, params);
        
        if (toolpaths.empty()) {
            std::cerr << "Error: No toolpaths generated" << std::endl;
            return 1;
        }
        
        // Generate G-code
        std::cout << "4. Generating G-code..." << std::endl;
        GCodeGenerator gcodeGen;
        
        if (!gcodeGen.generateGCode(toolpaths, params, outputFile)) {
            std::cerr << "Error: Failed to generate G-code file: " << outputFile << std::endl;
            return 1;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "\n=== Conversion Complete ===" << std::endl;
        std::cout << "✅ Successfully converted " << inputFile << " to " << outputFile << std::endl;
        std::cout << "⏱️  Total processing time: " << duration.count() << " ms" << std::endl;
        std::cout << "📊 Generated " << toolpaths.size() << " toolpath points" << std::endl;
        std::cout << "🔧 Tool: Ø" << params.tool.diameter << "mm endmill" << std::endl;
        std::cout << "📏 Material: " << params.material.width << "×" << params.material.length 
                  << "×" << params.material.height << " mm" << std::endl;
        std::cout << "============================" << std::endl;
        
        if (!result.warnings.empty()) {
            std::cout << "\n⚠️  Note: " << result.warnings.size() << " warning(s) were reported." << std::endl;
            std::cout << "Review the G-code and consider the recommendations before machining." << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}