#include "client_config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace SnowOwl::Client::Core {

ClientConfig::ClientConfig() 
    : configPath_("") 
{
    
}

bool ClientConfig::load(const std::string& configFile) {
    configPath_ = configFile;
    configMap_.clear();
    
    std::ifstream file(configFile);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            configMap_[key] = value;
        }
    }
    
    file.close();
    return true;
}

bool ClientConfig::save(const std::string& configFile) {
    std::ofstream file(configFile);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : configMap_) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    
    file.close();
    return true;
}

std::string ClientConfig::get(const std::string& key, const std::string& defaultValue) const {
    auto it = configMap_.find(key);
    if (it != configMap_.end()) {
        return it->second;
    }
    return defaultValue;
}

void ClientConfig::set(const std::string& key, const std::string& value) {
    configMap_[key] = value;
}

void ClientConfig::remove(const std::string& key) {
    auto it = configMap_.find(key);
    if (it != configMap_.end()) {
        configMap_.erase(it);
    }
}

void ClientConfig::clear() {
    configMap_.clear();
}

}
