#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

#include "modules/discovery/network_scanner.hpp"

namespace SnowOwl::Server::Modules::Discovery {

struct LocalDevice {
    std::string deviceId;
    std::string name;
    std::string manufacturer;
    std::string model;
    std::vector<std::string> supportedFormats;
    int width;
    int height;
};

class DeviceDiscovery {
public:
    DeviceDiscovery();
    ~DeviceDiscovery();

    std::vector<DiscoveredDevice> discoverNetworkDevices(const std::string& networkRange = "192.168.1.0/24");
    std::vector<LocalDevice> discoverLocalDevices();
    
    void setNetworkDiscoveryCallback(std::function<void(const DiscoveredDevice&)> callback);
    void setLocalDiscoveryCallback(std::function<void(const LocalDevice&)> callback);

private:
    std::unique_ptr<NetworkScanner> networkScanner_;
    std::function<void(const DiscoveredDevice&)> networkDiscoveryCallback_;
    std::function<void(const LocalDevice&)> localDiscoveryCallback_;
    
    void onNetworkDeviceFound(const DiscoveredDevice& device);
};

}