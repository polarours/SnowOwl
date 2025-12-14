#pragma once

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace SnowOwl::Cli::Commands {

int executeServerCommand(const po::variables_map& vm);

}
