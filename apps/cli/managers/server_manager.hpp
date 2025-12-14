#pragma once

#include <boost/program_options.hpp>

namespace SnowOwl::Cli::Managers {

class ServerManager {
public:
    static int startServer(const boost::program_options::variables_map& vm);
};

}