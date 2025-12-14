#pragma once

#include <boost/program_options.hpp>

#include "commands/server_commands.hpp"
#include "commands/edge_commands.hpp"
#include "commands/client_commands.hpp"
#include "commands/device_commands.hpp"
#include "commands/config_commands.hpp"

namespace po = boost::program_options;

po::options_description getMainOptions();

po::options_description getServerOptions();

po::options_description getEdgeOptions();

po::options_description getClientOptions();

po::options_description getDeviceOptions();

po::options_description getConfigOptions();