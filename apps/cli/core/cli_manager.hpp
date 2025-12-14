#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace SnowOwl::Cli::Core {

using json = nlohmann::json;

class CliManager {
public:
    CliManager(const std::string& serverUrl);
    
    bool listDevices();
    bool registerDevice(const std::string& deviceId, const std::string& name, const std::string& uri, const std::string& kind = "camera");
    bool updateDevice(const std::string& deviceId, const std::string& name, const std::string& uri);
    bool deleteDevice(const std::string& deviceId);
    bool getDeviceInfo(const std::string& deviceId);
    
    bool listConfig();
    bool getConfigValue(const std::string& key);
    bool setConfigValue(const std::string& key, const std::string& value);
    bool resetConfig();
    
    bool getServerStatus();
    
    bool startStream(const std::string& deviceId);
    bool stopStream(const std::string& deviceId);
    
private:
    std::string serverUrl_;
    
    bool makeHttpRequest(const std::string& endpoint, const std::string& method, const std::string& postData, std::string& response, long& responseCode);
};

}