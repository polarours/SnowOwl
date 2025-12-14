#include "cli_manager.hpp"
#include <iostream>
#include <curl/curl.h>

namespace SnowOwl::Cli::Core {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

CliManager::CliManager(const std::string& serverUrl) : serverUrl_(serverUrl) {}

bool CliManager::makeHttpRequest(const std::string& endpoint, const std::string& method, const std::string& postData, std::string& response, long& responseCode) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    std::string url = serverUrl_ + endpoint;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    } else if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    }
    
    struct curl_slist* headers = nullptr;
    if (!postData.empty()) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return true;
}

bool CliManager::listDevices() {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices", "GET", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        try {
            auto jsonResponse = json::parse(response);
            std::cout << "Devices:" << std::endl;
            std::cout << jsonResponse.dump(4) << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::registerDevice(const std::string& deviceId, const std::string& name, const std::string& uri, const std::string& kind) {
    json postData;
    postData["device_id"] = deviceId;
    postData["name"] = name;
    postData["uri"] = uri;
    postData["kind"] = kind;
    
    std::string postDataStr = postData.dump();
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices", "POST", postDataStr, response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 201) {
        std::cout << "Device registered successfully: " << deviceId << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::updateDevice(const std::string& deviceId, const std::string& name, const std::string& uri) {
    json putData;
    putData["name"] = name;
    putData["uri"] = uri;
    
    std::string putDataStr = putData.dump();
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices/" + deviceId, "PUT", putDataStr, response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        std::cout << "Device updated successfully: " << deviceId << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::deleteDevice(const std::string& deviceId) {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices/" + deviceId, "DELETE", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 204) {
        std::cout << "Device deleted successfully: " << deviceId << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::getDeviceInfo(const std::string& deviceId) {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices/" + deviceId, "GET", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        try {
            auto jsonResponse = json::parse(response);
            std::cout << "Device Info:" << std::endl;
            std::cout << jsonResponse.dump(4) << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::listConfig() {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/config", "GET", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        try {
            auto jsonResponse = json::parse(response);
            std::cout << "Configuration:" << std::endl;
            std::cout << jsonResponse.dump(4) << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::getConfigValue(const std::string& key) {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/config/" + key, "GET", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        try {
            auto jsonResponse = json::parse(response);
            std::cout << "Configuration value for " << key << ":" << std::endl;
            std::cout << jsonResponse.dump(4) << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::setConfigValue(const std::string& key, const std::string& value) {
    json patchData;
    patchData[key] = value;
    
    std::string patchDataStr = patchData.dump();
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/config", "PATCH", patchDataStr, response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        std::cout << "Configuration updated successfully" << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::resetConfig() {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/config/reset", "POST", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        std::cout << "Configuration reset successfully" << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::getServerStatus() {
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/status", "GET", "", response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        try {
            auto jsonResponse = json::parse(response);
            
            std::cout << "● snowowl-server.service - SnowOwl Surveillance Server" << std::endl;
            
            bool active = jsonResponse.value("active", false);
            std::string status = jsonResponse.value("status", "unknown");
            
            if (active) {
                std::cout << "   Loaded: loaded (/etc/systemd/system/snowowl-server.service; enabled; vendor preset: enabled)" << std::endl;
                std::cout << "   Active: active (running) since ";
                
                if (jsonResponse.contains("timestamp")) {
                    time_t timestamp = jsonResponse["timestamp"];
                    char buffer[100];
                    strftime(buffer, sizeof(buffer), "%a %Y-%m-%d %H:%M:%S %Z", localtime(&timestamp));
                    std::cout << buffer;
                }
                std::cout << std::endl;
            } else {
                std::cout << "   Active: inactive (dead)" << std::endl;
            }
            
            if (jsonResponse.contains("process")) {
                auto process = jsonResponse["process"];
                if (process.contains("pid")) {
                    std::cout << "     Docs: man:snowowl-server(8)" << std::endl;
                    std::cout << "  Process: " << process["pid"].get<int>() << " (snowowl-server)" << std::endl;
                }
            }
            
            if (jsonResponse.contains("listening_ports")) {
                std::cout << "   Listen: ";
                auto ports = jsonResponse["listening_ports"];
                for (size_t i = 0; i < ports.size(); ++i) {
                    if (i > 0) std::cout << "           ";
                    std::cout << "*:" << ports[i].get<int>() << " (Stream)" << std::endl;
                }
            }
            
            if (jsonResponse.contains("components")) {
                std::cout << "Components:" << std::endl;
                auto components = jsonResponse["components"];
                for (auto& [key, value] : components.items()) {
                    std::cout << "   ● " << key << ": " << value.get<std::string>() << std::endl;
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::startStream(const std::string& deviceId) {
    json postData;
    postData["action"] = "start_stream";
    
    std::string postDataStr = postData.dump();
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices/" + deviceId + "/stream/start", "POST", postDataStr, response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        std::cout << "Stream started successfully for device " << deviceId << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

bool CliManager::stopStream(const std::string& deviceId) {
    json postData;
    postData["action"] = "stop_stream";
    
    std::string postDataStr = postData.dump();
    std::string response;
    long responseCode;
    
    if (!makeHttpRequest("/api/v1/devices/" + deviceId + "/stream/stop", "POST", postDataStr, response, responseCode)) {
        std::cerr << "Failed to make HTTP request" << std::endl;
        return false;
    }
    
    if (responseCode == 200) {
        std::cout << "Stream stopped successfully for device " << deviceId << std::endl;
        return true;
    } else {
        std::cerr << "HTTP Error " << responseCode << ": " << response << std::endl;
        return false;
    }
}

}