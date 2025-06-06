#include "core/tool.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

namespace nwss {
namespace cnc {

// Static helper functions
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// Tool implementation
std::string Tool::getTypeString() const {
    switch (type) {
        case ToolType::END_MILL: return "End Mill";
        case ToolType::BALL_NOSE: return "Ball Nose";
        case ToolType::V_BIT: return "V-Bit";
        case ToolType::DRILL: return "Drill";
        case ToolType::ROUTER_BIT: return "Router Bit";
        case ToolType::ENGRAVING_BIT: return "Engraving Bit";
        case ToolType::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

std::string Tool::getMaterialString() const {
    switch (material) {
        case ToolMaterial::HSS: return "HSS";
        case ToolMaterial::CARBIDE: return "Carbide";
        case ToolMaterial::CERAMIC: return "Ceramic";
        case ToolMaterial::DIAMOND: return "Diamond";
        case ToolMaterial::COBALT: return "Cobalt";
        case ToolMaterial::UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

std::string Tool::getCoatingString() const {
    switch (coating) {
        case ToolCoating::NONE: return "None";
        case ToolCoating::TIN: return "TiN";
        case ToolCoating::TICN: return "TiCN";
        case ToolCoating::TIALN: return "TiAlN";
        case ToolCoating::DLC: return "DLC";
        case ToolCoating::UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

double Tool::calculateRecommendedFeedRate(const std::string& materialType) const {
    // Base feed rate calculations based on tool type and material
    double baseFeedRate = 0.0;
    
    // Calculate feed per tooth based on tool diameter and material
    double feedPerTooth = 0.0;
    
    if (materialType == "Wood" || materialType == "Plywood" || materialType == "MDF") {
        feedPerTooth = std::min(0.5, diameter * 0.1); // Aggressive for wood
    } else if (materialType == "Aluminum" || materialType == "Brass") {
        feedPerTooth = std::min(0.2, diameter * 0.05); // Moderate for soft metals
    } else if (materialType == "Steel" || materialType == "Stainless Steel") {
        feedPerTooth = std::min(0.1, diameter * 0.025); // Conservative for hard metals
    } else if (materialType == "Plastic" || materialType == "Acrylic") {
        feedPerTooth = std::min(0.3, diameter * 0.08); // Moderate for plastics
    } else {
        // Default for unknown materials
        feedPerTooth = std::min(0.2, diameter * 0.05);
    }
    
    // Calculate spindle speed for feed calculation
    int rpm = calculateRecommendedSpindleSpeed(materialType);
    
    // Feed rate = feed per tooth * number of flutes * RPM
    baseFeedRate = feedPerTooth * fluteCount * rpm;
    
    // Apply constraints
    if (maxFeedRate > 0) {
        baseFeedRate = std::min(baseFeedRate, maxFeedRate);
    }
    
    return std::max(100.0, baseFeedRate); // Minimum 100 mm/min
}

int Tool::calculateRecommendedSpindleSpeed(const std::string& materialType) const {
    if (diameter <= 0) return 1000;
    
    // Surface feet per minute (SFM) recommendations by material
    double sfm = 0.0;
    
    if (materialType == "Wood" || materialType == "Plywood" || materialType == "MDF") {
        sfm = (material == ToolMaterial::CARBIDE) ? 800 : 600;
    } else if (materialType == "Aluminum" || materialType == "Brass") {
        sfm = (material == ToolMaterial::CARBIDE) ? 1000 : 500;
    } else if (materialType == "Steel" || materialType == "Stainless Steel") {
        sfm = (material == ToolMaterial::CARBIDE) ? 400 : 200;
    } else if (materialType == "Plastic" || materialType == "Acrylic") {
        sfm = (material == ToolMaterial::CARBIDE) ? 1200 : 800;
    } else {
        // Default SFM
        sfm = (material == ToolMaterial::CARBIDE) ? 600 : 400;
    }
    
    // RPM = (SFM * 12) / (π * diameter_inches)
    double diameterInches = diameter / 25.4; // Convert mm to inches
    int rpm = static_cast<int>((sfm * 12) / (M_PI * diameterInches));
    
    // Apply constraints
    if (maxSpindleSpeed > 0) {
        rpm = std::min(rpm, static_cast<int>(maxSpindleSpeed));
    }
    if (minSpindleSpeed > 0) {
        rpm = std::max(rpm, static_cast<int>(minSpindleSpeed));
    }
    
    // Reasonable bounds
    return std::max(500, std::min(30000, rpm));
}

bool Tool::isValid() const {
    return id > 0 && diameter > 0 && !name.empty();
}

// ToolRegistry implementation
ToolRegistry::ToolRegistry() : m_nextToolId(1) {
    loadDefaultTools();
}

ToolRegistry::~ToolRegistry() = default;

int ToolRegistry::addTool(const Tool& tool) {
    Tool newTool = tool;
    if (newTool.id <= 0) {
        newTool.id = generateToolId();
    }
    
    m_tools[newTool.id] = newTool;
    return newTool.id;
}

bool ToolRegistry::removeTool(int toolId) {
    auto it = m_tools.find(toolId);
    if (it != m_tools.end()) {
        m_tools.erase(it);
        return true;
    }
    return false;
}

bool ToolRegistry::updateTool(const Tool& tool) {
    auto it = m_tools.find(tool.id);
    if (it != m_tools.end()) {
        it->second = tool;
        return true;
    }
    return false;
}

const Tool* ToolRegistry::getTool(int toolId) const {
    auto it = m_tools.find(toolId);
    return (it != m_tools.end()) ? &it->second : nullptr;
}

std::vector<Tool> ToolRegistry::getAllTools() const {
    std::vector<Tool> tools;
    tools.reserve(m_tools.size());
    for (const auto& pair : m_tools) {
        tools.push_back(pair.second);
    }
    return tools;
}

std::vector<Tool> ToolRegistry::getActiveTools() const {
    std::vector<Tool> activeTools;
    for (const auto& pair : m_tools) {
        if (pair.second.isActive) {
            activeTools.push_back(pair.second);
        }
    }
    return activeTools;
}

std::vector<Tool> ToolRegistry::getToolsByType(ToolType type) const {
    std::vector<Tool> tools;
    for (const auto& pair : m_tools) {
        if (pair.second.type == type && pair.second.isActive) {
            tools.push_back(pair.second);
        }
    }
    return tools;
}

const Tool* ToolRegistry::findBestToolForFeature(double featureSize, const std::string& materialType) const {
    const Tool* bestTool = nullptr;
    double bestScore = -1.0;
    
    for (const auto& pair : m_tools) {
        const Tool& tool = pair.second;
        if (!tool.isActive || tool.diameter <= 0) continue;
        
        // Tool must be smaller than the feature
        if (tool.diameter >= featureSize) continue;
        
        // Calculate a score based on how well the tool fits the feature
        double sizeRatio = tool.diameter / featureSize;
        double score = sizeRatio; // Prefer larger tools (within constraint)
        
        // Bonus for carbide tools
        if (tool.material == ToolMaterial::CARBIDE) {
            score += 0.1;
        }
        
        // Bonus for more flutes (better finish)
        score += (tool.fluteCount - 2) * 0.05;
        
        // Bonus for appropriate tool types
        if (featureSize < 1.0 && tool.type == ToolType::ENGRAVING_BIT) {
            score += 0.2;
        } else if (featureSize >= 1.0 && tool.type == ToolType::END_MILL) {
            score += 0.1;
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestTool = &tool;
        }
    }
    
    return bestTool;
}

bool ToolRegistry::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    m_tools.clear();
    m_nextToolId = 1;
    
    std::string line;
    Tool currentTool;
    bool inTool = false;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        if (line == "[TOOL]") {
            if (inTool && currentTool.isValid()) {
                m_tools[currentTool.id] = currentTool;
                m_nextToolId = std::max(m_nextToolId, currentTool.id + 1);
            }
            currentTool = Tool();
            inTool = true;
            continue;
        }
        
        if (!inTool) continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        
        if (key == "id") {
            currentTool.id = std::stoi(value);
        } else if (key == "name") {
            currentTool.name = value;
        } else if (key == "type") {
            if (value == "EndMill") currentTool.type = ToolType::END_MILL;
            else if (value == "BallNose") currentTool.type = ToolType::BALL_NOSE;
            else if (value == "VBit") currentTool.type = ToolType::V_BIT;
            else if (value == "Drill") currentTool.type = ToolType::DRILL;
            else if (value == "RouterBit") currentTool.type = ToolType::ROUTER_BIT;
            else if (value == "EngravingBit") currentTool.type = ToolType::ENGRAVING_BIT;
            else currentTool.type = ToolType::CUSTOM;
        } else if (key == "diameter") {
            currentTool.diameter = std::stod(value);
        } else if (key == "length") {
            currentTool.length = std::stod(value);
        } else if (key == "fluteLength") {
            currentTool.fluteLength = std::stod(value);
        } else if (key == "fluteCount") {
            currentTool.fluteCount = std::stoi(value);
        } else if (key == "material") {
            if (value == "HSS") currentTool.material = ToolMaterial::HSS;
            else if (value == "Carbide") currentTool.material = ToolMaterial::CARBIDE;
            else if (value == "Ceramic") currentTool.material = ToolMaterial::CERAMIC;
            else if (value == "Diamond") currentTool.material = ToolMaterial::DIAMOND;
            else if (value == "Cobalt") currentTool.material = ToolMaterial::COBALT;
            else currentTool.material = ToolMaterial::UNKNOWN;
        } else if (key == "maxDepthOfCut") {
            currentTool.maxDepthOfCut = std::stod(value);
        } else if (key == "maxFeedRate") {
            currentTool.maxFeedRate = std::stod(value);
        } else if (key == "maxSpindleSpeed") {
            currentTool.maxSpindleSpeed = std::stoi(value);
        } else if (key == "minSpindleSpeed") {
            currentTool.minSpindleSpeed = std::stoi(value);
        } else if (key == "notes") {
            currentTool.notes = value;
        } else if (key == "active") {
            currentTool.isActive = (value == "true" || value == "1");
        }
    }
    
    // Add the last tool
    if (inTool && currentTool.isValid()) {
        m_tools[currentTool.id] = currentTool;
        m_nextToolId = std::max(m_nextToolId, currentTool.id + 1);
    }
    
    return true;
}

bool ToolRegistry::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# NWSS-CNC Tool Registry\n";
    file << "# Tool definitions for CNC machining\n\n";
    
    for (const auto& pair : m_tools) {
        const Tool& tool = pair.second;
        
        file << "[TOOL]\n";
        file << "id=" << tool.id << "\n";
        file << "name=" << tool.name << "\n";
        file << "type=";
        switch (tool.type) {
            case ToolType::END_MILL: file << "EndMill"; break;
            case ToolType::BALL_NOSE: file << "BallNose"; break;
            case ToolType::V_BIT: file << "VBit"; break;
            case ToolType::DRILL: file << "Drill"; break;
            case ToolType::ROUTER_BIT: file << "RouterBit"; break;
            case ToolType::ENGRAVING_BIT: file << "EngravingBit"; break;
            default: file << "Custom"; break;
        }
        file << "\n";
        file << "diameter=" << tool.diameter << "\n";
        file << "length=" << tool.length << "\n";
        file << "fluteLength=" << tool.fluteLength << "\n";
        file << "fluteCount=" << tool.fluteCount << "\n";
        file << "material=";
        switch (tool.material) {
            case ToolMaterial::HSS: file << "HSS"; break;
            case ToolMaterial::CARBIDE: file << "Carbide"; break;
            case ToolMaterial::CERAMIC: file << "Ceramic"; break;
            case ToolMaterial::DIAMOND: file << "Diamond"; break;
            case ToolMaterial::COBALT: file << "Cobalt"; break;
            default: file << "Unknown"; break;
        }
        file << "\n";
        file << "maxDepthOfCut=" << tool.maxDepthOfCut << "\n";
        file << "maxFeedRate=" << tool.maxFeedRate << "\n";
        file << "maxSpindleSpeed=" << tool.maxSpindleSpeed << "\n";
        file << "minSpindleSpeed=" << tool.minSpindleSpeed << "\n";
        file << "notes=" << tool.notes << "\n";
        file << "active=" << (tool.isActive ? "true" : "false") << "\n";
        file << "\n";
    }
    
    return true;
}

void ToolRegistry::loadDefaultTools() {
    clear();
    
    // Add some common default tools
    Tool tool;
    
    // 1/8" End Mill
    tool = Tool(1, "1/8\" Carbide End Mill", ToolType::END_MILL, 3.175);
    tool.length = 38.0;
    tool.fluteLength = 12.0;
    tool.fluteCount = 2;
    tool.material = ToolMaterial::CARBIDE;
    tool.maxDepthOfCut = 1.5;
    tool.maxFeedRate = 1000.0;
    tool.maxSpindleSpeed = 20000;
    tool.minSpindleSpeed = 8000;
    tool.notes = "General purpose end mill for aluminum and wood";
    m_tools[tool.id] = tool;
    
    // 1/4" End Mill
    tool = Tool(2, "1/4\" Carbide End Mill", ToolType::END_MILL, 6.35);
    tool.length = 50.0;
    tool.fluteLength = 16.0;
    tool.fluteCount = 2;
    tool.material = ToolMaterial::CARBIDE;
    tool.maxDepthOfCut = 3.0;
    tool.maxFeedRate = 2000.0;
    tool.maxSpindleSpeed = 18000;
    tool.minSpindleSpeed = 6000;
    tool.notes = "Heavy duty end mill for larger features";
    m_tools[tool.id] = tool;
    
    // 1/16" End Mill
    tool = Tool(3, "1/16\" Carbide End Mill", ToolType::END_MILL, 1.5875);
    tool.length = 38.0;
    tool.fluteLength = 6.0;
    tool.fluteCount = 2;
    tool.material = ToolMaterial::CARBIDE;
    tool.maxDepthOfCut = 0.5;
    tool.maxFeedRate = 500.0;
    tool.maxSpindleSpeed = 25000;
    tool.minSpindleSpeed = 12000;
    tool.notes = "Fine detail work";
    m_tools[tool.id] = tool;
    
    // V-Bit for engraving
    tool = Tool(4, "60° V-Bit", ToolType::V_BIT, 0.2); // Tip diameter
    tool.length = 38.0;
    tool.fluteLength = 8.0;
    tool.fluteCount = 1;
    tool.material = ToolMaterial::CARBIDE;
    tool.maxDepthOfCut = 0.2;
    tool.maxFeedRate = 300.0;
    tool.maxSpindleSpeed = 20000;
    tool.minSpindleSpeed = 8000;
    tool.notes = "Engraving and fine detail work";
    m_tools[tool.id] = tool;
    
    // Ball nose for 3D work
    tool = Tool(5, "1/8\" Ball Nose", ToolType::BALL_NOSE, 3.175);
    tool.length = 38.0;
    tool.fluteLength = 12.0;
    tool.fluteCount = 2;
    tool.material = ToolMaterial::CARBIDE;
    tool.maxDepthOfCut = 1.0;
    tool.maxFeedRate = 800.0;
    tool.maxSpindleSpeed = 18000;
    tool.minSpindleSpeed = 8000;
    tool.notes = "3D profiling and contouring";
    m_tools[tool.id] = tool;
    
    m_nextToolId = 6;
}

void ToolRegistry::clear() {
    m_tools.clear();
    m_nextToolId = 1;
}

int ToolRegistry::getNextToolId() const {
    return m_nextToolId;
}

bool ToolRegistry::toolExists(int toolId) const {
    return m_tools.find(toolId) != m_tools.end();
}

int ToolRegistry::generateToolId() {
    return m_nextToolId++;
}

} // namespace cnc
} // namespace nwss 