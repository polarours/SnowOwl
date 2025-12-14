#include "plugin_manager.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace SnowOwl {

PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    return instance;
}

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    shutdownPlugins();
}

bool PluginManager::loadPlugins(const std::string& pluginDirectory) {
    pluginDirectory_ = pluginDirectory;
    
    if (!fs::exists(pluginDirectory)) {
        std::cerr << "Plugin directory does not exist: " << pluginDirectory << std::endl;
        return false;
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(pluginDirectory)) {
            if (entry.is_directory()) {
                std::string pluginPath = entry.path().string();
                loadPlugin(pluginPath);
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading plugins: " << e.what() << std::endl;
        return false;
    }
}

bool PluginManager::loadPlugin(const std::string& pluginPath) {
    // For now, we'll just log that we found a plugin directory
    // Actual plugin loading implementation would go here
    std::cout << "Found plugin directory: " << pluginPath << std::endl;
    
    // Look for manifest.json
    std::string manifestPath = pluginPath + "/manifest.json";
    if (!fs::exists(manifestPath)) {
        std::cerr << "Plugin manifest not found: " << manifestPath << std::endl;
        return false;
    }
    
    try {
        std::ifstream manifestFile(manifestPath);
        json manifest;
        manifestFile >> manifest;
        
        std::string pluginName = manifest.value("name", "unknown");
        std::string pluginType = manifest.value("type", "unknown");
        std::string pluginVersion = manifest.value("version", "0.0.0");
        
        std::cout << "Loaded plugin manifest - Name: " << pluginName 
                  << ", Type: " << pluginType 
                  << ", Version: " << pluginVersion << std::endl;
                  
        // In a real implementation, we would load the actual plugin library here
        // For now, we'll just store basic information about the plugin
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing plugin manifest: " << e.what() << std::endl;
        return false;
    }
}

void PluginManager::unloadPlugins() {
    shutdownPlugins();
    plugins_.clear();
}

std::vector<std::shared_ptr<PluginInterface>> PluginManager::getAllPlugins() const {
    std::vector<std::shared_ptr<PluginInterface>> result;
    for (const auto& pair : plugins_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<PluginInterface>> PluginManager::getPluginsByType(const std::string& type) const {
    std::vector<std::shared_ptr<PluginInterface>> result;
    for (const auto& pair : plugins_) {
        if (pair.second->getType() == type) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::shared_ptr<PluginInterface> PluginManager::getPluginByName(const std::string& name) const {
    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second;
    }
    return nullptr;
}

bool PluginManager::initializePlugins() {
    bool allSuccess = true;
    for (const auto& pair : plugins_) {
        if (!pair.second->initialize()) {
            std::cerr << "Failed to initialize plugin: " << pair.first << std::endl;
            allSuccess = false;
        }
    }
    return allSuccess;
}

void PluginManager::shutdownPlugins() {
    for (const auto& pair : plugins_) {
        pair.second->shutdown();
    }
}

} // namespace SnowOwl