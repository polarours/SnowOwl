#include <snow_owl/plugin_interface.h>
#include <iostream>
#include <string>

namespace SnowOwl {
namespace Plugins {

class IndustrialSafetyPlugin : public ServerPluginInterface {
public:
    IndustrialSafetyPlugin() : enabled_(false) {}

    // Plugin metadata
    std::string getName() const override {
        return "industrial_safety_monitoring";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "Industrial safety monitoring plugin for oil and gas applications";
    }
    
    std::string getType() const override {
        return "server";
    }

    // Lifecycle management
    bool initialize() override {
        std::cout << "Industrial Safety Plugin initialized" << std::endl;
        enabled_ = true;
        return true;
    }
    
    void shutdown() override {
        std::cout << "Industrial Safety Plugin shutdown" << std::endl;
        enabled_ = false;
    }

    // Plugin state
    bool isEnabled() const override {
        return enabled_;
    }
    
    void setEnabled(bool enabled) override {
        enabled_ = enabled;
        if (enabled_) {
            std::cout << "Industrial Safety Plugin enabled" << std::endl;
        } else {
            std::cout << "Industrial Safety Plugin disabled" << std::endl;
        }
    }

private:
    bool enabled_;
};

// Factory function to create the plugin instance
extern "C" PluginInterface* createPlugin() {
    return new IndustrialSafetyPlugin();
}

// Function to destroy the plugin instance
extern "C" void destroyPlugin(PluginInterface* plugin) {
    delete plugin;
}

} // namespace Plugins
} // namespace SnowOwl