#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "device_manager.hpp"

namespace SnowOwl::Cli::Managers {

int DeviceManager::startDevice(const boost::program_options::variables_map& vm) {
    std::cout << "Executing device command:" << std::endl;
    
    if (vm.count("list")) {
        std::cout << "  Listing all devices..." << std::endl;
    }
    
    if (vm.count("register")) {
        std::cout << "  Registering new device..." << std::endl;
        if (vm.count("device-id")) {
            std::cout << "    Device ID: " << vm["device-id"].as<std::string>() << std::endl;
        }
        if (vm.count("name")) {
            std::cout << "    Name: " << vm["name"].as<std::string>() << std::endl;
        }
        if (vm.count("uri")) {
            std::cout << "    URI: " << vm["uri"].as<std::string>() << std::endl;
        }
        if (vm.count("kind")) {
            std::cout << "    Kind: " << vm["kind"].as<std::string>() << std::endl;
        }
    }
    
    if (vm.count("update")) {
        std::string deviceId = vm["update"].as<std::string>();
        std::cout << "  Updating device: " << deviceId << std::endl;
    }
    
    if (vm.count("delete")) {
        std::string deviceId = vm["delete"].as<std::string>();
        std::cout << "  Deleting device: " << deviceId << std::endl;
    }
    
    if (vm.count("info")) {
        std::string deviceId = vm["info"].as<std::string>();
        std::cout << "  Showing info for device: " << deviceId << std::endl;
    }
    
    return 0;
}

}
