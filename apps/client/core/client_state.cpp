#include <algorithm>

#include "client_state.hpp"

namespace SnowOwl::Client::Core {

ClientState::ClientState() : serverUrl_("http://localhost:8081"), streamingDeviceId_(""), clientMode_("web") {}

void ClientState::setServerUrl(const std::string& url) {
    serverUrl_ = url;
}

std::string ClientState::getServerUrl() const {
    return serverUrl_;
}

void ClientState::addDevice(const DeviceInfo& device) {
    auto it = std::find_if(devices_.begin(), devices_.end(), 
                          [&device](const DeviceInfo& d) { return d.id == device.id; });
    
    if (it == devices_.end()) {
        devices_.push_back(device);
    } else {
        *it = device;
    }
}

void ClientState::removeDevice(const std::string& deviceId) {
    auto it = std::find_if(devices_.begin(), devices_.end(), 
                          [&deviceId](const DeviceInfo& d) { return d.id == deviceId; });
    
    if (it != devices_.end()) {
        if (streamingDeviceId_ == deviceId) {
            stopStreaming();
        }
        devices_.erase(it);
    }
}

void ClientState::updateDevice(const DeviceInfo& device) {
    auto it = std::find_if(devices_.begin(), devices_.end(), 
                          [&device](const DeviceInfo& d) { return d.id == device.id; });
    
    if (it != devices_.end()) {
        *it = device;
    } else {
        devices_.push_back(device);
    }
}

DeviceInfo* ClientState::getDevice(const std::string& deviceId) {
    auto it = std::find_if(devices_.begin(), devices_.end(), 
                          [&deviceId](const DeviceInfo& d) { return d.id == deviceId; });
    
    if (it != devices_.end()) {
        return &(*it);
    }
    return nullptr;
}

const std::vector<DeviceInfo>& ClientState::getDevices() const {
    return devices_;
}

void ClientState::setStreamingDevice(const std::string& deviceId) {
    streamingDeviceId_ = deviceId;
}

std::string ClientState::getStreamingDevice() const {
    return streamingDeviceId_;
}

void ClientState::stopStreaming() {
    streamingDeviceId_.clear();
}

bool ClientState::isStreaming() const {
    return !streamingDeviceId_.empty();
}

void ClientState::setClientMode(const std::string& mode) {
    clientMode_ = mode;
}

std::string ClientState::getClientMode() const {
    return clientMode_;
}

}
