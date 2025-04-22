#ifndef NWSS_CNC_UTILS_HPP
#define NWSS_CNC_UTILS_HPP

#include <string>
#include <vector>
#include "nwss-cnc/geometry.h"
#include "nwss-cnc/config.h"

namespace nwss {
namespace cnc {

class Utils {
public:
    // Save discretized paths to a CSV file
    static bool savePathsToCSV(const std::vector<Path>& paths, const std::string& filename);
    
    // Generate a visualization SVG showing the original shapes and discretized points
    static bool generateVisualization(const std::string& sourceFile, 
                                    const std::vector<Path>& paths,
                                    const std::string& outputFile);
    
    // Generate a visualization showing how the design fits on the material and bed
    static bool generateMaterialVisualization(const std::vector<Path>& paths, 
                                           const CNConfig& config,
                                           const std::string& outputFile);
    
    // Convert an RGBA color value to a hex string
    static std::string colorToHex(uint32_t color);
    
    // Format a number with a specific precision
    static std::string formatNumber(double value, int precision = 4);
    
    // Get the file extension from a path
    static std::string getFileExtension(const std::string& path);
    
    // Get the filename without extension
    static std::string getBaseName(const std::string& path);
    
    // Generate a filename with a different extension
    static std::string replaceExtension(const std::string& path, const std::string& newExtension);
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_UTILS_HPP