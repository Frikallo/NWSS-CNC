#include "core/svg_parser.h"
#include "nanosvg.h"
#include <cstdio>

namespace nwss {
namespace cnc {

SVGParser::SVGParser() : m_image(nullptr) {}

SVGParser::~SVGParser() {
    freeImage();
}

bool SVGParser::loadFromFile(const std::string& filename, const std::string& units, float dpi) {
    // Free any previously loaded image
    freeImage();
    
    // Parse the SVG file
    m_image = nsvgParseFromFile(filename.c_str(), units.c_str(), dpi);
    
    return m_image != nullptr;
}

bool SVGParser::getDimensions(float& width, float& height) const {
    if (!m_image) {
        return false;
    }
    
    width = m_image->width;
    height = m_image->height;
    
    return true;
}

SVGContentBounds SVGParser::getContentBounds() const {
    SVGContentBounds bounds;
    
    if (!m_image) {
        return bounds;
    }
    
    calculateContentBounds(bounds);
    return bounds;
}

bool SVGParser::getContentDimensions(float& width, float& height) const {
    SVGContentBounds bounds = getContentBounds();
    
    if (bounds.isEmpty) {
        return false;
    }
    
    width = bounds.width;
    height = bounds.height;
    
    return true;
}

SVGContentBounds SVGParser::getContentBoundsWithMargin(float marginMm) const {
    SVGContentBounds bounds = getContentBounds();
    
    if (bounds.isEmpty) {
        return bounds;
    }
    
    // Add margin to all sides
    bounds.minX -= marginMm;
    bounds.minY -= marginMm;
    bounds.maxX += marginMm;
    bounds.maxY += marginMm;
    
    // Update width and height
    bounds.width = bounds.maxX - bounds.minX;
    bounds.height = bounds.maxY - bounds.minY;
    
    return bounds;
}

std::vector<SVGShapeInfo> SVGParser::getShapeInfo() const {
    std::vector<SVGShapeInfo> shapeInfoList;
    
    if (!m_image) {
        return shapeInfoList;
    }
    
    for (NSVGshape* shape = m_image->shapes; shape != nullptr; shape = shape->next) {
        shapeInfoList.push_back(extractShapeInfo(shape));
    }
    
    return shapeInfoList;
}

int SVGParser::getShapeCount() const {
    if (!m_image) {
        return 0;
    }
    
    int count = 0;
    for (NSVGshape* shape = m_image->shapes; shape != nullptr; shape = shape->next) {
        count++;
    }
    
    return count;
}

NSVGshape* SVGParser::getShape(int index) const {
    if (!m_image) {
        return nullptr;
    }
    
    int current = 0;
    for (NSVGshape* shape = m_image->shapes; shape != nullptr; shape = shape->next) {
        if (current == index) {
            return shape;
        }
        current++;
    }
    
    return nullptr;
}

void SVGParser::freeImage() {
    if (m_image) {
        nsvgDelete(m_image);
        m_image = nullptr;
    }
}

SVGShapeInfo SVGParser::extractShapeInfo(NSVGshape* shape) const {
    SVGShapeInfo info;
    
    if (!shape) {
        return info;
    }
    
    // Extract basic shape information
    info.id = shape->id;
    
    // Extract colors and stroke width
    info.fillColor = shape->fill.color;
    info.strokeColor = shape->stroke.color;
    info.strokeWidth = shape->strokeWidth;
    
    // Extract bounds
    for (int i = 0; i < 4; i++) {
        info.bounds[i] = shape->bounds[i];
    }
    
    return info;
}

void SVGParser::calculateContentBounds(SVGContentBounds& bounds) const {
    if (!m_image || !m_image->shapes) {
        bounds.isEmpty = true;
        return;
    }
    
    bool firstShape = true;
    int shapeCount = 0;
    
    // Iterate through all shapes to find the actual content bounds
    for (NSVGshape* shape = m_image->shapes; shape != nullptr; shape = shape->next) {
        shapeCount++;
        
        // Debug: Print shape information
        printf("Shape %d: bounds[%.3f, %.3f, %.3f, %.3f], visible=%d, fill=%d, stroke=%d\n",
               shapeCount, shape->bounds[0], shape->bounds[1], shape->bounds[2], shape->bounds[3],
               (shape->flags & NSVG_FLAGS_VISIBLE) ? 1 : 0,
               shape->fill.type, shape->stroke.type);
        
        // Skip invisible shapes
        if (!(shape->flags & NSVG_FLAGS_VISIBLE)) {
            printf("  Skipping invisible shape\n");
            continue;
        }
        
        // Skip shapes with no fill and no stroke
        if (shape->fill.type == NSVG_PAINT_NONE && shape->stroke.type == NSVG_PAINT_NONE) {
            printf("  Skipping shape with no fill/stroke\n");
            continue;
        }
        
        // Initialize bounds with first valid shape
        if (firstShape) {
            bounds.minX = shape->bounds[0];
            bounds.minY = shape->bounds[1];
            bounds.maxX = shape->bounds[2];
            bounds.maxY = shape->bounds[3];
            bounds.isEmpty = false;
            firstShape = false;
            printf("  Using as first shape bounds: [%.3f, %.3f, %.3f, %.3f]\n",
                   bounds.minX, bounds.minY, bounds.maxX, bounds.maxY);
        } else {
            // Expand bounds to include this shape
            float oldMinX = bounds.minX, oldMinY = bounds.minY, oldMaxX = bounds.maxX, oldMaxY = bounds.maxY;
            bounds.minX = std::min(bounds.minX, shape->bounds[0]);
            bounds.minY = std::min(bounds.minY, shape->bounds[1]);
            bounds.maxX = std::max(bounds.maxX, shape->bounds[2]);
            bounds.maxY = std::max(bounds.maxY, shape->bounds[3]);
            printf("  Expanding bounds from [%.3f, %.3f, %.3f, %.3f] to [%.3f, %.3f, %.3f, %.3f]\n",
                   oldMinX, oldMinY, oldMaxX, oldMaxY,
                   bounds.minX, bounds.minY, bounds.maxX, bounds.maxY);
        }
    }
    
    // Calculate width and height if we found any content
    if (!bounds.isEmpty) {
        bounds.width = bounds.maxX - bounds.minX;
        bounds.height = bounds.maxY - bounds.minY;
        
        // Ensure we have positive dimensions
        if (bounds.width <= 0 || bounds.height <= 0) {
            bounds.isEmpty = true;
        }
    }
}

} // namespace cnc
} // namespace nwss