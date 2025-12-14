#pragma once

#include <boost/program_options.hpp>

namespace SnowOwl::Cli::Managers {

class ClientManager {
public:
    static int startClient(const boost::program_options::variables_map& vm);
};

}