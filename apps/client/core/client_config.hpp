#pragma once

#include <string>
#include <map>

namespace SnowOwl::Client::Core {

class ClientConfig {
public:
    ClientConfig();
    
    bool load(const std::string& configFile);
    
    bool save(const std::string& configFile);
    
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    
    void set(const std::string& key, const std::string& value);
    
    void remove(const std::string& key);
    
    void clear();
    
private:
    std::map<std::string, std::string> configMap_;
    std::string configPath_;
};

}
