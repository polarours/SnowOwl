#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "managers/server_manager.hpp"

namespace SnowOwl::Cli::Commands {

namespace po = boost::program_options;

int executeServerCommand(const po::variables_map& vm) {
    return SnowOwl::Cli::Managers::ServerManager::startServer(vm);
}

}
