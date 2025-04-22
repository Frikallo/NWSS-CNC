#include "nwss-cnc/config.h"
#include <iostream>
#include <limits>
#include <string>

using namespace nwss::cnc;

// Helper function to get numeric input with validation
template<typename T>
T getNumericInput(const std::string& prompt, T minValue, T maxValue) {
    T value;
    while (true) {
        std::cout << prompt;
        
        if (std::cin >> value) {
            if (value >= minValue && value <= maxValue) {
                break;
            } else {
                std::cout << "Error: Value must be between " << minValue << " and " << maxValue << std::endl;
            }
        } else {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Error: Invalid input. Please enter a number." << std::endl;
        }
    }
    
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

// Helper function to get yes/no input
bool getYesNoInput(const std::string& prompt, bool defaultValue) {
    std::string input;
    std::string defaultStr = defaultValue ? "Y/n" : "y/N";
    
    std::cout << prompt << " [" << defaultStr << "]: ";
    std::getline(std::cin, input);
    
    if (input.empty()) {
        return defaultValue;
    }
    
    return (input[0] == 'Y' || input[0] == 'y');
}

// Helper function to get string input with default value
std::string getStringInput(const std::string& prompt, const std::string& defaultValue) {
    std::string input;
    
    std::cout << prompt << " [" << defaultValue << "]: ";
    std::getline(std::cin, input);
    
    if (input.empty()) {
        return defaultValue;
    }
    
    return input;
}

void runConfigWizard(CNConfig& config) {
    std::cout << "\n====================================" << std::endl;
    std::cout << "NWSS CNC Configuration Wizard" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "This wizard will help you set up your CNC machine configuration." << std::endl;
    std::cout << "Press Enter to accept default values shown in brackets." << std::endl;
    std::cout << "------------------------------------" << std::endl;
    
    // Units selection first since it affects other measurements
    std::string unitStr = getStringInput("Select measurement units (mm/in)", "mm");
    config.setUnitsFromString(unitStr);
    bool isMetric = (config.getUnits() == MeasurementUnit::MILLIMETERS);
    
    // Machine settings
    std::cout << "\n--- Machine Settings ---" << std::endl;
    double defaultBedWidth = isMetric ? 300.0 : 12.0;
    double defaultBedHeight = isMetric ? 300.0 : 12.0;
    double maxBedDim = isMetric ? 2000.0 : 80.0;
    
    config.setBedWidth(getNumericInput<double>("Enter bed width (" + config.getUnitsString() + "): ", 
                                             0.1, maxBedDim));
    config.setBedHeight(getNumericInput<double>("Enter bed height (" + config.getUnitsString() + "): ", 
                                              0.1, maxBedDim));
    
    // Material settings
    std::cout << "\n--- Material Settings ---" << std::endl;
    double defaultMatWidth = isMetric ? 200.0 : 8.0;
    double defaultMatHeight = isMetric ? 200.0 : 8.0;
    double defaultMatThickness = isMetric ? 10.0 : 0.4;
    double maxMatDim = isMetric ? 1000.0 : 40.0;
    
    config.setMaterialWidth(getNumericInput<double>("Enter material width (" + config.getUnitsString() + "): ", 
                                                  0.1, maxMatDim));
    config.setMaterialHeight(getNumericInput<double>("Enter material height (" + config.getUnitsString() + "): ", 
                                                   0.1, maxMatDim));
    config.setMaterialThickness(getNumericInput<double>("Enter material thickness (" + config.getUnitsString() + "): ", 
                                                      0.1, maxMatDim));
    
    // Cutting settings
    std::cout << "\n--- Cutting Settings ---" << std::endl;
    double defaultFeedRate = isMetric ? 800.0 : 30.0;
    double defaultPlungeRate = isMetric ? 200.0 : 8.0;
    double defaultCutDepth = isMetric ? 1.0 : 0.04;
    double defaultSafeHeight = isMetric ? 5.0 : 0.2;
    
    config.setFeedRate(getNumericInput<double>("Enter feed rate (" + config.getUnitsString() + "/min): ", 
                                             1.0, isMetric ? 10000.0 : 400.0));
    config.setPlungeRate(getNumericInput<double>("Enter plunge rate (" + config.getUnitsString() + "/min): ", 
                                               1.0, isMetric ? 5000.0 : 200.0));
    config.setSpindleSpeed(getNumericInput<int>("Enter spindle speed (RPM): ", 
                                              1000, 30000));
    config.setCutDepth(getNumericInput<double>("Enter cut depth per pass (" + config.getUnitsString() + "): ", 
                                             0.01, isMetric ? 20.0 : 0.8));
    config.setPassCount(getNumericInput<int>("Enter number of passes: ", 
                                           1, 100));
    config.setSafeHeight(getNumericInput<double>("Enter safe travel height (" + config.getUnitsString() + "): ", 
                                               0.1, isMetric ? 50.0 : 2.0));
    
    std::cout << "\nConfiguration complete!" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string configFile = "nwss-cnc.cfg";
    
    // Check for custom config file path
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            configFile = argv[++i];
        }
    }
    
    CNConfig config;
    
    // Check if this is the first run
    bool firstRun = CNConfig::isFirstRun(configFile);
    
    if (firstRun) {
        std::cout << "No configuration file found. Starting setup wizard..." << std::endl;
        runConfigWizard(config);
        
        // Save the configuration
        if (config.saveToFile(configFile)) {
            std::cout << "Configuration saved to: " << configFile << std::endl;
        } else {
            std::cerr << "Error: Failed to save configuration." << std::endl;
            return 1;
        }
    } else {
        // Load existing configuration
        if (!config.loadFromFile(configFile)) {
            std::cerr << "Error: Failed to load configuration from: " << configFile << std::endl;
            return 1;
        }
        
        std::cout << "Configuration loaded from: " << configFile << std::endl;
        
        // Ask if the user wants to modify the configuration
        if (getYesNoInput("Would you like to modify the configuration?", false)) {
            runConfigWizard(config);
            
            // Save the updated configuration
            if (config.saveToFile(configFile)) {
                std::cout << "Configuration updated and saved to: " << configFile << std::endl;
            } else {
                std::cerr << "Error: Failed to save configuration." << std::endl;
                return 1;
            }
        }
    }
    
    // Display the current configuration
    std::cout << "\n====================================" << std::endl;
    std::cout << "Current Configuration" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Machine:" << std::endl;
    std::cout << "  Bed Size: " << config.getBedWidth() << " x " << config.getBedHeight() 
              << " " << config.getUnitsString() << std::endl;
    std::cout << "Material:" << std::endl;
    std::cout << "  Size: " << config.getMaterialWidth() << " x " << config.getMaterialHeight() 
              << " x " << config.getMaterialThickness() << " " << config.getUnitsString() << std::endl;
    std::cout << "Cutting:" << std::endl;
    std::cout << "  Feed Rate: " << config.getFeedRate() << " " << config.getUnitsString() << "/min" << std::endl;
    std::cout << "  Plunge Rate: " << config.getPlungeRate() << " " << config.getUnitsString() << "/min" << std::endl;
    std::cout << "  Spindle Speed: " << config.getSpindleSpeed() << " RPM" << std::endl;
    std::cout << "  Cut Depth: " << config.getCutDepth() << " " << config.getUnitsString() 
              << " x " << config.getPassCount() << " passes" << std::endl;
    std::cout << "  Safe Height: " << config.getSafeHeight() << " " << config.getUnitsString() << std::endl;
    
    return 0;
}