#ifndef NWSS_CNC_SVG_PARSER_HPP
#define NWSS_CNC_SVG_PARSER_HPP

#include <string>
#include <vector>
#include <cstdint>

// Forward declarations for NanoSVG types
struct NSVGimage;
struct NSVGshape;

namespace nwss {
namespace cnc {

/**
 * Structure to hold SVG shape information
 */
struct SVGShapeInfo {
    std::string id;
    std::string type;
    uint32_t fillColor;
    uint32_t strokeColor;
    float strokeWidth;
    float bounds[4]; // xmin, ymin, xmax, ymax
};

/**
 * Structure to hold content bounds information
 */
struct SVGContentBounds {
    float minX, minY, maxX, maxY;  // Actual content bounds
    float width, height;           // Content dimensions
    bool isEmpty;                  // True if no content found
    
    SVGContentBounds() : minX(0), minY(0), maxX(0), maxY(0), width(0), height(0), isEmpty(true) {}
};

/**
 * Class to parse and extract information from SVG files
 */
class SVGParser {
public:
    SVGParser();
    ~SVGParser();
    
    // Parse an SVG file and load it into memory
    bool loadFromFile(const std::string& filename, const std::string& units = "mm", float dpi = 96.0f);
    
    // Get the dimensions of the loaded SVG (original dimensions including margins)
    bool getDimensions(float& width, float& height) const;
    
    // Get the content bounds (actual shape boundaries without margins)
    SVGContentBounds getContentBounds() const;
    
    // Get the dimensions of just the content (without margins)
    bool getContentDimensions(float& width, float& height) const;
    
    // Get the content bounds with an optional margin in mm
    SVGContentBounds getContentBoundsWithMargin(float marginMm = 0.0f) const;
    
    // Get all shape information from the loaded SVG
    std::vector<SVGShapeInfo> getShapeInfo() const;
    
    // Get the raw NanoSVG image pointer (for advanced usage)
    NSVGimage* getRawImage() const { return m_image; }
    
    // Get the number of shapes in the SVG
    int getShapeCount() const;
    
    // Get a specific shape by index
    NSVGshape* getShape(int index) const;
    
    // Free the memory used by the SVG image
    void freeImage();

private:
    NSVGimage* m_image;
    
    // Helper function to extract shape info
    SVGShapeInfo extractShapeInfo(NSVGshape* shape) const;
    
    // Helper function to calculate content bounds from all shapes
    void calculateContentBounds(SVGContentBounds& bounds) const;
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_SVG_PARSER_HPP