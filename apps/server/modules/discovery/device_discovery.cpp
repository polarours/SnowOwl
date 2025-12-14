#include "device_discovery.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <iterator>

#if defined(_WIN32)
#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#endif

namespace SnowOwl::Server::Modules::Discovery {

DeviceDiscovery::DeviceDiscovery() 
    : networkScanner_(std::make_unique<NetworkScanner>()) {

    networkScanner_->setDiscoveryCallback([this](const DiscoveredDevice& device) {
        onNetworkDeviceFound(device);
    });
}

DeviceDiscovery::~DeviceDiscovery() = default;

void DeviceDiscovery::setNetworkDiscoveryCallback(std::function<void(const DiscoveredDevice&)> callback) {
    networkDiscoveryCallback_ = std::move(callback);
}

void DeviceDiscovery::setLocalDiscoveryCallback(std::function<void(const LocalDevice&)> callback) {
    localDiscoveryCallback_ = std::move(callback);
}

std::vector<DiscoveredDevice> DeviceDiscovery::discoverNetworkDevices(const std::string& networkRange) {
    return networkScanner_->scanNetwork(networkRange);
}

std::vector<LocalDevice> DeviceDiscovery::discoverLocalDevices() {
    std::vector<LocalDevice> devices;
    
#if defined(_WIN32)
    LocalDevice device1;
    device1.deviceId = "webcam_0";
    device1.name = "Integrated Webcam";
    device1.manufacturer = "Generic";
    device1.model = "HD Webcam";
    device1.supportedFormats = {"YUYV", "MJPG"};
    device1.width = 1920;
    device1.height = 1080;
    
    LocalDevice device2;
    device2.deviceId = "webcam_1";
    device2.name = "USB Camera";
    device2.manufacturer = "Logitech";
    device2.model = "C920";
    device2.supportedFormats = {"YUYV", "MJPG", "H264"};
    device2.width = 1920;
    device2.height = 1080;
    
    devices.push_back(device1);
    devices.push_back(device2);
    
    if (localDiscoveryCallback_) {
        for (const auto& device : devices) {
            localDiscoveryCallback_(device);
        }
    }
    
#elif defined(__linux__)
    for (int i = 0; i < 10; ++i) {
        std::string devicePath = "/dev/video" + std::to_string(i);
        
        int fd = open(devicePath.c_str(), O_RDONLY);
        if (fd == -1) {
            continue;
        }
        
        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            LocalDevice device;
            device.deviceId = devicePath;
            device.name = reinterpret_cast<const char*>(cap.card);
            device.manufacturer = "Linux V4L2";
            device.model = reinterpret_cast<const char*>(cap.driver);
            

            device.supportedFormats = {"YUYV", "MJPG", "H264"};
            device.width = 1920;
            device.height = 1080;
            
            devices.push_back(device);
            
            if (localDiscoveryCallback_) {
                localDiscoveryCallback_(device);
            }
        }
        
        close(fd);
    }
#else
    LocalDevice device;
    device.deviceId = "mock_webcam";
    device.name = "Mock Webcam";
    device.manufacturer = "Mock Vendor";
    device.model = "Mock Model";
    device.supportedFormats = {"YUYV"};
    device.width = 640;
    device.height = 480;
    
    devices.push_back(device);
    
    if (localDiscoveryCallback_) {
        localDiscoveryCallback_(device);
    }
#endif
    
    return devices;
}

void DeviceDiscovery::onNetworkDeviceFound(const DiscoveredDevice& device) {
    if (networkDiscoveryCallback_) {
        networkDiscoveryCallback_(device);
    }
}

}