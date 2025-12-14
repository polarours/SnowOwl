#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "managers/client_manager.hpp"

namespace SnowOwl::Cli::Commands {

namespace po = boost::program_options;

int executeClientCommand(const po::variables_map& vm) {
    return SnowOwl::Cli::Managers::ClientManager::startClient(vm);
}

}
