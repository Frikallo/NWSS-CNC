#ifndef NWSS_CNC_CONFIG_H
#define NWSS_CNC_CONFIG_H

#include <string>

namespace nwss {
namespace cnc {

/**
 * Enum for measurement units
 */
enum class MeasurementUnit {
    MILLIMETERS,
    INCHES
};

/**
 * Configuration class for CNC settings
 */
class CNConfig {
public:
    CNConfig();
    ~CNConfig();
    
    /**
     * Initialize with default values
     */
    void setDefaults();
    
    /**
     * Load configuration from file
     * @param filename Path to the config file
     * @return True if loaded successfully
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * Save configuration to file
     * @param filename Path where to save the config
     * @return True if saved successfully
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * Check if this is the first run (no config file exists)
     * @param filename Path to the config file
     * @return True if the config file doesn't exist
     */
    static bool isFirstRun(const std::string& filename);

    // Getters and setters for configuration properties
    double getBedWidth() const { return m_bedWidth; }
    void setBedWidth(double width) { m_bedWidth = width; }
    
    double getBedHeight() const { return m_bedHeight; }
    void setBedHeight(double height) { m_bedHeight = height; }
    
    MeasurementUnit getUnits() const { return m_units; }
    void setUnits(MeasurementUnit units) { m_units = units; }
    std::string getUnitsString() const;
    void setUnitsFromString(const std::string& units);
    
    double getMaterialWidth() const { return m_materialWidth; }
    void setMaterialWidth(double width) { m_materialWidth = width; }
    
    double getMaterialHeight() const { return m_materialHeight; }
    void setMaterialHeight(double height) { m_materialHeight = height; }
    
    double getMaterialThickness() const { return m_materialThickness; }
    void setMaterialThickness(double thickness) { m_materialThickness = thickness; }
    
    double getFeedRate() const { return m_feedRate; }
    void setFeedRate(double rate) { m_feedRate = rate; }
    
    double getPlungeRate() const { return m_plungeRate; }
    void setPlungeRate(double rate) { m_plungeRate = rate; }
    
    int getSpindleSpeed() const { return m_spindleSpeed; }
    void setSpindleSpeed(int speed) { m_spindleSpeed = speed; }
    
    double getCutDepth() const { return m_cutDepth; }
    void setCutDepth(double depth) { m_cutDepth = depth; }
    
    int getPassCount() const { return m_passCount; }
    void setPassCount(int count) { m_passCount = count; }
    
    double getSafeHeight() const { return m_safeHeight; }
    void setSafeHeight(double height) { m_safeHeight = height; }
    
private:
    // CNC machine physical properties
    double m_bedWidth;         // Width of the CNC bed
    double m_bedHeight;        // Height of the CNC bed
    MeasurementUnit m_units;   // Measurement units (mm or inches)
    
    // Material properties
    double m_materialWidth;    // Width of the material
    double m_materialHeight;   // Height of the material
    double m_materialThickness; // Thickness of the material
    
    // Cutting properties
    double m_feedRate;         // Feed rate for X/Y movement (units/min)
    double m_plungeRate;       // Feed rate for Z movement (units/min)
    int m_spindleSpeed;        // Spindle speed (RPM)
    double m_cutDepth;         // Depth of cut per pass
    int m_passCount;           // Number of passes for full depth
    double m_safeHeight;       // Safe height for travel moves
    
    // Helper methods for parsing
    bool parseLine(const std::string& line, std::string& key, std::string& value) const;
    static std::string trim(const std::string& str);
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_CONFIG_H