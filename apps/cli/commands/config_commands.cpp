#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "managers/config_manager.hpp"

namespace SnowOwl::Cli::Commands {

namespace po = boost::program_options;
using json = nlohmann::json;

int executeConfigCommand(const po::variables_map& vm) {
    return SnowOwl::Cli::Managers::ConfigManager::startConfig(vm);
}

}