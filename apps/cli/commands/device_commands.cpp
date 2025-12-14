#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "managers/device_manager.hpp"

namespace SnowOwl::Cli::Commands {

namespace po = boost::program_options;
using json = nlohmann::json;

int executeDeviceCommand(const po::variables_map& vm) {
    return SnowOwl::Cli::Managers::DeviceManager::startDevice(vm);
}

}