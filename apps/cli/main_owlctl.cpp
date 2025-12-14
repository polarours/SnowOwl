#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <curl/curl.h>

#include "core/cli_manager.hpp"

namespace po = boost::program_options;
using SnowOwl::Cli::Core::CliManager;

int main(int argc, char* argv[]) {
    po::options_description desc("Owl Control Tool (owlctl)");
    desc.add_options()
        ("help,h", "Show help message")
        ("version,v", "Show version")
        ("server,s", po::value<std::string>()->default_value("http://localhost:8081"), "Server URL")
        
        ("list-devices", "List all registered devices")
        ("server-status", "Get server status")
        ("update-config", po::value<std::vector<std::string>>()->multitoken(), "Update server configuration (key value)")
        ("get-config", po::value<std::string>(), "Get configuration value by key")
        ("list-config", "List all configuration")
        ("reset-config", "Reset configuration to defaults")
        
        ("start-stream", po::value<std::string>(), "Start stream for device ID")
        ("stop-stream", po::value<std::string>(), "Stop stream for device ID")
        
        ("register-device", po::value<std::vector<std::string>>()->multitoken(), "Register edge device (device_id name uri [kind])")
        ("update-device", po::value<std::vector<std::string>>()->multitoken(), "Update edge device (device_id name uri)")
        ("delete-device", po::value<std::string>(), "Delete edge device by ID")
        ("device-info", po::value<std::string>(), "Get device information by ID");

    po::variables_map vm;
    
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing command line: " << e.what() << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }
    
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    
    if (vm.count("version")) {
        std::cout << "Owl Control Tool (owlctl) v0.1.0" << std::endl;
        return 0;
    }
    
    std::string serverUrl = vm["server"].as<std::string>();
    CliManager manager(serverUrl);
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    int result = 0;
    
    if (vm.count("list-devices")) {
        if (!manager.listDevices()) {
            result = 1;
        }
    } else if (vm.count("server-status")) {
        if (!manager.getServerStatus()) {
            result = 1;
        }
    } else if (vm.count("list-config")) {
        if (!manager.listConfig()) {
            result = 1;
        }
    } else if (vm.count("get-config")) {
        std::string key = vm["get-config"].as<std::string>();
        if (!manager.getConfigValue(key)) {
            result = 1;
        }
    } else if (vm.count("update-config")) {
        auto configParams = vm["update-config"].as<std::vector<std::string>>();
        if (configParams.size() >= 2) {
            std::string key = configParams[0];
            std::string value = configParams[1];
            if (!manager.setConfigValue(key, value)) {
                result = 1;
            }
        } else {
            std::cerr << "update-config requires key and value parameters" << std::endl;
            result = 1;
        }
    } else if (vm.count("reset-config")) {
        if (!manager.resetConfig()) {
            result = 1;
        }
    }
    
    else if (vm.count("start-stream")) {
        std::string deviceId = vm["start-stream"].as<std::string>();
        if (!manager.startStream(deviceId)) {
            result = 1;
        }
    } else if (vm.count("stop-stream")) {
        std::string deviceId = vm["stop-stream"].as<std::string>();
        if (!manager.stopStream(deviceId)) {
            result = 1;
        }
    }
    
    else if (vm.count("register-device")) {
        auto deviceParams = vm["register-device"].as<std::vector<std::string>>();
        if (deviceParams.size() >= 3) {
            std::string deviceId = deviceParams[0];
            std::string name = deviceParams[1];
            std::string uri = deviceParams[2];
            std::string kind = "camera";
            
            if (deviceParams.size() >= 4) {
                kind = deviceParams[3];
            }
            
            if (!manager.registerDevice(deviceId, name, uri, kind)) {
                result = 1;
            }
        } else {
            std::cerr << "register-device requires device_id, name, and uri parameters" << std::endl;
            result = 1;
        }
    } else if (vm.count("update-device")) {
        auto deviceParams = vm["update-device"].as<std::vector<std::string>>();
        if (deviceParams.size() >= 3) {
            std::string deviceId = deviceParams[0];
            std::string name = deviceParams[1];
            std::string uri = deviceParams[2];
            if (!manager.updateDevice(deviceId, name, uri)) {
                result = 1;
            }
        } else {
            std::cerr << "update-device requires device_id, name, and uri parameters" << std::endl;
            result = 1;
        }
    } else if (vm.count("delete-device")) {
        std::string deviceId = vm["delete-device"].as<std::string>();
        if (!manager.deleteDevice(deviceId)) {
            result = 1;
        }
    } else if (vm.count("device-info")) {
        std::string deviceId = vm["device-info"].as<std::string>();
        if (!manager.getDeviceInfo(deviceId)) {
            result = 1;
        }
    } else {
        std::cout << desc << std::endl;
    }
    
    curl_global_cleanup();
    
    return result;
}
