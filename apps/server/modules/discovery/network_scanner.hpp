#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace SnowOwl::Server::Modules::Discovery {

struct DiscoveredDevice {
    std::string ipAddress;
    std::string macAddress;
    std::string modelName;
    std::string manufacturer;
    std::vector<std::string> supportedProtocols;
    std::string rtspUrl;
    std::string httpAdminUrl;
};

class NetworkScanner {
public:
    NetworkScanner();
    ~NetworkScanner();

    std::vector<DiscoveredDevice> scanNetwork(const std::string& networkRange = "192.168.1.0/24");
    
    std::optional<DiscoveredDevice> probeDevice(const std::string& ipAddress);
    
    void setDiscoveryCallback(std::function<void(const DiscoveredDevice&)> callback);

private:
    std::function<void(const DiscoveredDevice&)> discoveryCallback_;
    
    std::vector<DiscoveredDevice> discoverViaNmap(const std::string& networkRange);
    std::vector<DiscoveredDevice> discoverViaUpnp();
    std::vector<DiscoveredDevice> discoverViaBroadcast();
    std::vector<DiscoveredDevice> discoverViaKnownPorts();
    
    std::vector<DiscoveredDevice> parseNmapXmlOutput(const std::string& xmlOutput);
    
    bool probeRtsp(const std::string& ipAddress, DiscoveredDevice& device);
    bool probeHttp(const std::string& ipAddress, DiscoveredDevice& device);
    bool probeOnvif(const std::string& ipAddress, DiscoveredDevice& device);
};

}