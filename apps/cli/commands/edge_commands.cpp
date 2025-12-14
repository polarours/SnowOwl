#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "managers/edge_manager.hpp"

namespace SnowOwl::Cli::Commands {

namespace po = boost::program_options;

int executeEdgeCommand(const po::variables_map& vm) {
    return SnowOwl::Cli::Managers::EdgeManager::startEdge(vm);
}

}
