#pragma once

#include <string>
#include <vector>
#include <map>

namespace SnowOwl::Client::Core {

struct DeviceInfo {
    std::string id;
    std::string name;
    std::string uri;
    std::string kind;
    bool isOnline;
};

class ClientState {
public:
    ClientState();
    
    void setServerUrl(const std::string& url);
    std::string getServerUrl() const;
    
    void addDevice(const DeviceInfo& device);
    void removeDevice(const std::string& deviceId);
    void updateDevice(const DeviceInfo& device);
    DeviceInfo* getDevice(const std::string& deviceId);
    const std::vector<DeviceInfo>& getDevices() const;
    
    void setStreamingDevice(const std::string& deviceId);
    std::string getStreamingDevice() const;
    void stopStreaming();
    bool isStreaming() const;
    
    void setClientMode(const std::string& mode);
    std::string getClientMode() const;
    
private:
    std::string serverUrl_;
    std::vector<DeviceInfo> devices_;
    std::string streamingDeviceId_;
    std::string clientMode_;
};

}
