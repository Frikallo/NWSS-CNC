#include "nwss-cnc/svg_parser.h"
#include "third_party/nanosvg.h"

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
    info.id = shape->id ? shape->id : "";
    
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

} // namespace cnc
} // namespace nwss