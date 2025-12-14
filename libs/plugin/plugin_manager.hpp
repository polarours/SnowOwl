#pragma once

#include <snow_owl/plugin_interface.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace SnowOwl {

class PluginManager {
public:
    static PluginManager& getInstance();
    
    // Plugin loading and management
    bool loadPlugins(const std::string& pluginDirectory);
    bool loadPlugin(const std::string& pluginPath);
    void unloadPlugins();
    
    // Plugin access
    std::vector<std::shared_ptr<PluginInterface>> getAllPlugins() const;
    std::vector<std::shared_ptr<PluginInterface>> getPluginsByType(const std::string& type) const;
    std::shared_ptr<PluginInterface> getPluginByName(const std::string& name) const;
    
    // Plugin lifecycle
    bool initializePlugins();
    void shutdownPlugins();
    
private:
    PluginManager();
    ~PluginManager();
    
    std::map<std::string, std::shared_ptr<PluginInterface>> plugins_;
    std::string pluginDirectory_;
};

}