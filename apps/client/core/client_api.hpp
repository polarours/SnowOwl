#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace SnowOwl::Client::Core {
    
using json = nlohmann::json;

class ClientAPI {
public:
    ClientAPI(const std::string& serverUrl);
    
    json listDevices();
    bool registerDevice(const std::string& deviceId, const std::string& name, const std::string& uri);
    bool updateDevice(const std::string& deviceId, const std::string& name, const std::string& uri);
    bool deleteDevice(const std::string& deviceId);
    
    bool startStream(const std::string& deviceId);
    bool stopStream(const std::string& deviceId);
    
    json getServerStatus();
    bool updateServerConfig(const std::string& key, const std::string& value);
    
private:
    std::string serverUrl_;
};

}
