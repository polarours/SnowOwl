#include <iostream>
#include <boost/program_options.hpp>

#include "config_manager.hpp"

namespace SnowOwl::Cli::Managers {

namespace po = boost::program_options;    

int ConfigManager::startConfig(const boost::program_options::variables_map& vm) {
    std::cout << "Executing config command:" << std::endl;
    
    if (vm.count("list")) {
        std::cout << "  Listing configuration..." << std::endl;
    }
    
    if (vm.count("set")) {
        auto keyValue = vm["set"].as<std::vector<std::string>>();
        if (keyValue.size() >= 2) {
            std::cout << "  Setting configuration: " << keyValue[0] << " = " << keyValue[1] << std::endl;
        } else {
            std::cerr << "  Error: 'set' requires key and value parameters" << std::endl;
            return 1;
        }
    }
    
    if (vm.count("get")) {
        std::string key = vm["get"].as<std::string>();
        std::cout << "  Getting configuration for key: " << key << std::endl;
    }
    
    if (vm.count("reset")) {
        std::cout << "  Resetting configuration..." << std::endl;
    }
    
    return 0;
}

}
