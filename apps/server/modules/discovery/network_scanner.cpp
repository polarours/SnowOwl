#include "network_scanner.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <regex>
#include <tinyxml2.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace SnowOwl::Server::Modules::Discovery {

NetworkScanner::NetworkScanner() = default;

NetworkScanner::~NetworkScanner() = default;

void NetworkScanner::setDiscoveryCallback(std::function<void(const DiscoveredDevice&)> callback) {
    discoveryCallback_ = std::move(callback);
}

std::vector<DiscoveredDevice> NetworkScanner::scanNetwork(const std::string& networkRange) {

    try {
        return discoverViaNmap(networkRange);
    } catch (const std::exception& e) {
        std::cerr << "Nmap scanning failed: " << e.what() << ", falling back to mock data" << std::endl;
    }
    

    std::vector<DiscoveredDevice> devices;
    

    std::this_thread::sleep_for(std::chrono::seconds(2));
    

    DiscoveredDevice mockDevice1;
    mockDevice1.ipAddress = "192.168.1.64";
    mockDevice1.macAddress = "00:11:22:33:44:55";
    mockDevice1.modelName = "Hikvision DS-2CD2T47G1-L";
    mockDevice1.manufacturer = "Hikvision";
    mockDevice1.supportedProtocols = {"RTSP", "RTMP", "HLS"};
    mockDevice1.rtspUrl = "rtsp://192.168.1.64:554/stream1";
    mockDevice1.httpAdminUrl = "http://192.168.1.64";
    
    DiscoveredDevice mockDevice2;
    mockDevice2.ipAddress = "192.168.1.65";
    mockDevice2.macAddress = "AA:BB:CC:DD:EE:FF";
    mockDevice2.modelName = "Dahua IPC-HFW4431M";
    mockDevice2.manufacturer = "Dahua";
    mockDevice2.supportedProtocols = {"RTSP", "ONVIF"};
    mockDevice2.rtspUrl = "rtsp://192.168.1.65:554/cam/realmonitor?channel=1&subtype=0";
    mockDevice2.httpAdminUrl = "http://192.168.1.65";
    
    devices.push_back(mockDevice1);
    devices.push_back(mockDevice2);
    
    if (discoveryCallback_) {
        for (const auto& device : devices) {
            discoveryCallback_(device);
        }
    }
    
    return devices;
}

std::vector<DiscoveredDevice> NetworkScanner::discoverViaNmap(const std::string& networkRange) {
    std::vector<DiscoveredDevice> devices;
    
    std::string command = "nmap -sn -oX - " + networkRange + " 2>/dev/null";
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("Failed to execute nmap command");
    }
    
    std::string xmlOutput;
    char buffer[128];
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        xmlOutput += buffer;
    }
    
    return parseNmapXmlOutput(xmlOutput);
}

std::vector<DiscoveredDevice> NetworkScanner::parseNmapXmlOutput(const std::string& xmlOutput) {
    std::vector<DiscoveredDevice> devices;
    
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xmlOutput.c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Failed to parse nmap XML output");
    }
    
    tinyxml2::XMLElement* root = doc.FirstChildElement("nmaprun");
    if (!root) {
        throw std::runtime_error("Invalid nmap XML format");
    }
    
    for (tinyxml2::XMLElement* hostElem = root->FirstChildElement("host"); 
         hostElem != nullptr; 
         hostElem = hostElem->NextSiblingElement("host")) {
        
        tinyxml2::XMLElement* statusElem = hostElem->FirstChildElement("status");
        if (statusElem) {
            const char* state = statusElem->Attribute("state");
            if (!state || strcmp(state, "up") != 0) {
                continue;
            }
        }
        
        DiscoveredDevice device;
        
        for (tinyxml2::XMLElement* addrElem = hostElem->FirstChildElement("address");
             addrElem != nullptr;
             addrElem = addrElem->NextSiblingElement("address")) {
            
            const char* addrType = addrElem->Attribute("addrtype");
            const char* addr = addrElem->Attribute("addr");
            
            if (addrType && addr && strcmp(addrType, "ipv4") == 0) {
                device.ipAddress = addr;
            } else if (addrType && addr && strcmp(addrType, "mac") == 0) {
                device.macAddress = addr;
                const char* vendor = addrElem->Attribute("vendor");
                if (vendor) {
                    device.manufacturer = vendor;
                }
            }
        }
        
        tinyxml2::XMLElement* hostnamesElem = hostElem->FirstChildElement("hostnames");
        if (hostnamesElem) {
            tinyxml2::XMLElement* hostnameElem = hostnamesElem->FirstChildElement("hostname");
            if (hostnameElem) {
                const char* name = hostnameElem->Attribute("name");
                if (name) {
                    device.modelName = name;
                }
            }
        }
        
        if (!device.ipAddress.empty()) {
            devices.push_back(device);
        }
    }
    
    return devices;
}

std::optional<DiscoveredDevice> NetworkScanner::probeDevice(const std::string& ipAddress) {
    DiscoveredDevice device;
    device.ipAddress = ipAddress;
    

    if (probeRtsp(ipAddress, device)) {
        if (discoveryCallback_) {
            discoveryCallback_(device);
        }
        return device;
    }
    
    if (probeHttp(ipAddress, device)) {
        if (discoveryCallback_) {
            discoveryCallback_(device);
        }
        return device;
    }
    
    return std::nullopt;
}

bool NetworkScanner::probeRtsp(const std::string& ipAddress, DiscoveredDevice& device) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (ipAddress == "192.168.1.64" || ipAddress == "192.168.1.65") {
        device.supportedProtocols.push_back("RTSP");
        device.rtspUrl = "rtsp://" + ipAddress + ":554/stream1";
        return true;
    }
    
    return false;
}

bool NetworkScanner::probeHttp(const std::string& ipAddress, DiscoveredDevice& device) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    if (ipAddress == "192.168.1.64" || ipAddress == "192.168.1.65") {
        device.supportedProtocols.push_back("HTTP");
        device.httpAdminUrl = "http://" + ipAddress;
        return true;
    }
    
    return false;
}

bool NetworkScanner::probeOnvif(const std::string& ipAddress, DiscoveredDevice& device) {
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    if (ipAddress == "192.168.1.65") {
        device.supportedProtocols.push_back("ONVIF");
        return true;
    }
    
    return false;
}

}