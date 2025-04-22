#include "nwss-cnc/config.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace nwss {
namespace cnc {

CNConfig::CNConfig() {
    setDefaults();
}

CNConfig::~CNConfig() = default;

void CNConfig::setDefaults() {
    // CNC machine properties
    m_bedWidth = 300.0;
    m_bedHeight = 300.0;
    m_units = MeasurementUnit::MILLIMETERS;
    
    // Material properties
    m_materialWidth = 200.0;
    m_materialHeight = 200.0;
    m_materialThickness = 10.0;
    
    // Cutting properties
    m_feedRate = 800.0;
    m_plungeRate = 200.0;
    m_spindleSpeed = 12000;
    m_cutDepth = 1.0;
    m_passCount = 1;
    m_safeHeight = 5.0;
}

bool CNConfig::isFirstRun(const std::string& filename) {
    std::ifstream file(filename);
    return !file.good();
}

bool CNConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::string section;
    
    // First set defaults, then override with values from file
    setDefaults();
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key=value
        std::string key, value;
        if (!parseLine(line, key, value)) {
            continue;
        }
        
        // Process the key-value pair according to the section
        if (section == "machine") {
            if (key == "bed_width") m_bedWidth = std::stod(value);
            else if (key == "bed_height") m_bedHeight = std::stod(value);
            else if (key == "units") setUnitsFromString(value);
        }
        else if (section == "material") {
            if (key == "width") m_materialWidth = std::stod(value);
            else if (key == "height") m_materialHeight = std::stod(value);
            else if (key == "thickness") m_materialThickness = std::stod(value);
        }
        else if (section == "cutting") {
            if (key == "feed_rate") m_feedRate = std::stod(value);
            else if (key == "plunge_rate") m_plungeRate = std::stod(value);
            else if (key == "spindle_speed") m_spindleSpeed = std::stoi(value);
            else if (key == "cut_depth") m_cutDepth = std::stod(value);
            else if (key == "pass_count") m_passCount = std::stoi(value);
            else if (key == "safe_height") m_safeHeight = std::stod(value);
        }
    }
    
    file.close();
    return true;
}

bool CNConfig::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file for writing: " << filename << std::endl;
        return false;
    }
    
    // Write file header
    file << "# NWSS CNC Configuration File" << std::endl;
    file << "# Automatically generated" << std::endl << std::endl;
    
    // Machine section
    file << "[machine]" << std::endl;
    file << "bed_width=" << m_bedWidth << std::endl;
    file << "bed_height=" << m_bedHeight << std::endl;
    file << "units=" << getUnitsString() << std::endl << std::endl;
    
    // Material section
    file << "[material]" << std::endl;
    file << "width=" << m_materialWidth << std::endl;
    file << "height=" << m_materialHeight << std::endl;
    file << "thickness=" << m_materialThickness << std::endl << std::endl;
    
    // Cutting section
    file << "[cutting]" << std::endl;
    file << "feed_rate=" << m_feedRate << std::endl;
    file << "plunge_rate=" << m_plungeRate << std::endl;
    file << "spindle_speed=" << m_spindleSpeed << std::endl;
    file << "cut_depth=" << m_cutDepth << std::endl;
    file << "pass_count=" << m_passCount << std::endl;
    file << "safe_height=" << m_safeHeight << std::endl;
    
    file.close();
    return true;
}

bool CNConfig::parseLine(const std::string& line, std::string& key, std::string& value) const {
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
        return false;
    }
    
    key = trim(line.substr(0, pos));
    value = trim(line.substr(pos + 1));
    
    return !key.empty();
}

std::string CNConfig::trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    
    return (start < end) ? std::string(start, end) : std::string();
}

std::string CNConfig::getUnitsString() const {
    return (m_units == MeasurementUnit::MILLIMETERS) ? "mm" : "in";
}

void CNConfig::setUnitsFromString(const std::string& units) {
    if (units == "in" || units == "inch" || units == "inches") {
        m_units = MeasurementUnit::INCHES;
    } else {
        m_units = MeasurementUnit::MILLIMETERS;
    }
}

} // namespace cnc
} // namespace nwss