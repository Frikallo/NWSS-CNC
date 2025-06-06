#ifndef NWSS_CNC_TOOL_H
#define NWSS_CNC_TOOL_H

#include <string>
#include <vector>
#include <map>

namespace nwss {
namespace cnc {

/**
 * Enum for different types of CNC tools
 */
enum class ToolType {
    END_MILL,           // Standard end mill for cutting
    BALL_NOSE,          // Ball nose end mill for 3D work
    V_BIT,              // V-bit for engraving
    DRILL,              // Drill bit for holes
    ROUTER_BIT,         // Router bit for wood/plastic
    ENGRAVING_BIT,      // Fine engraving bit
    CUSTOM              // Custom tool type
};

/**
 * Enum for tool materials
 */
enum class ToolMaterial {
    HSS,                // High-Speed Steel
    CARBIDE,            // Carbide
    CERAMIC,            // Ceramic
    DIAMOND,            // Diamond-coated
    COBALT,             // Cobalt steel
    UNKNOWN             // Unknown material
};

/**
 * Enum for tool coating
 */
enum class ToolCoating {
    NONE,               // No coating
    TIN,                // Titanium Nitride
    TICN,               // Titanium Carbonitride
    TIALN,              // Titanium Aluminum Nitride
    DLC,                // Diamond-Like Carbon
    UNKNOWN             // Unknown coating
};

/**
 * Enum for tool offset direction
 */
enum class ToolOffsetDirection {
    INSIDE,             // Offset inside the path (for cutting out parts)
    OUTSIDE,            // Offset outside the path (for cutting pockets)
    ON_PATH,            // Cut exactly on the path (center of tool on path)
    AUTO                // Automatically determine based on path type
};

/**
 * Structure to hold tool information
 */
struct Tool {
    int id;                         // Unique tool ID
    std::string name;               // Tool name/description
    ToolType type;                  // Type of tool
    double diameter;                // Tool diameter in mm
    double length;                  // Tool length in mm
    double fluteLength;             // Flute length in mm
    int fluteCount;                 // Number of flutes
    ToolMaterial material;          // Tool material
    ToolCoating coating;            // Tool coating
    double maxDepthOfCut;           // Maximum depth of cut per pass
    double maxFeedRate;             // Maximum recommended feed rate
    double maxSpindleSpeed;         // Maximum spindle speed (RPM)
    double minSpindleSpeed;         // Minimum spindle speed (RPM)
    std::string notes;              // Additional notes
    bool isActive;                  // Whether tool is currently available
    
    // Constructor
    Tool(int id = 0, const std::string& name = "", ToolType type = ToolType::END_MILL, 
         double diameter = 0.0) 
        : id(id), name(name), type(type), diameter(diameter), length(0.0), 
          fluteLength(0.0), fluteCount(2), material(ToolMaterial::HSS), 
          coating(ToolCoating::NONE), maxDepthOfCut(0.0), maxFeedRate(0.0), 
          maxSpindleSpeed(0), minSpindleSpeed(0), notes(""), isActive(true) {}
    
    // Get tool type as string
    std::string getTypeString() const;
    
    // Get tool material as string
    std::string getMaterialString() const;
    
    // Get tool coating as string
    std::string getCoatingString() const;
    
    // Calculate recommended feed rate based on material and conditions
    double calculateRecommendedFeedRate(const std::string& materialType) const;
    
    // Calculate recommended spindle speed based on material
    int calculateRecommendedSpindleSpeed(const std::string& materialType) const;
    
    // Validate tool parameters
    bool isValid() const;
};

/**
 * Tool registry for managing all available tools
 */
class ToolRegistry {
public:
    ToolRegistry();
    ~ToolRegistry();
    
    /**
     * Add a new tool to the registry
     * @param tool The tool to add
     * @return The assigned tool ID
     */
    int addTool(const Tool& tool);
    
    /**
     * Remove a tool from the registry
     * @param toolId The ID of the tool to remove
     * @return True if tool was removed successfully
     */
    bool removeTool(int toolId);
    
    /**
     * Update an existing tool
     * @param tool The updated tool information
     * @return True if tool was updated successfully
     */
    bool updateTool(const Tool& tool);
    
    /**
     * Get a tool by ID
     * @param toolId The ID of the tool
     * @return Pointer to the tool, or nullptr if not found
     */
    const Tool* getTool(int toolId) const;
    
    /**
     * Get all tools
     * @return Vector of all tools
     */
    std::vector<Tool> getAllTools() const;
    
    /**
     * Get all active tools
     * @return Vector of active tools
     */
    std::vector<Tool> getActiveTools() const;
    
    /**
     * Get tools by type
     * @param type The tool type to search for
     * @return Vector of tools matching the type
     */
    std::vector<Tool> getToolsByType(ToolType type) const;
    
    /**
     * Find the best tool for a given feature size
     * @param featureSize The minimum feature size to cut
     * @param materialType The material being cut
     * @return Pointer to the best tool, or nullptr if none suitable
     */
    const Tool* findBestToolForFeature(double featureSize, const std::string& materialType = "") const;
    
    /**
     * Load tools from file
     * @param filename The file to load from
     * @return True if loaded successfully
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * Save tools to file
     * @param filename The file to save to
     * @return True if saved successfully
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * Load default tools
     */
    void loadDefaultTools();
    
    /**
     * Clear all tools
     */
    void clear();
    
    /**
     * Get the next available tool ID
     * @return Next available tool ID
     */
    int getNextToolId() const;
    
    /**
     * Check if a tool ID exists
     * @param toolId The tool ID to check
     * @return True if the tool exists
     */
    bool toolExists(int toolId) const;
    
    /**
     * Get the default tools file path
     * @return Path to the default tools file
     */
    std::string getDefaultToolsFilePath() const;
    
    /**
     * Save tools to default location
     * @return True if saved successfully
     */
    bool saveToDefaultLocation() const;
    
    /**
     * Load tools from default location if it exists
     * @return True if loaded successfully, false if file doesn't exist
     */
    bool loadFromDefaultLocation();

private:
    std::map<int, Tool> m_tools;    // Map of tool ID to tool
    int m_nextToolId;               // Next available tool ID
    
    /**
     * Generate a unique tool ID
     * @return Unique tool ID
     */
    int generateToolId();
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_TOOL_H 