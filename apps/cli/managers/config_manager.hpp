#pragma once

#include <boost/program_options.hpp>

namespace SnowOwl::Cli::Managers {

class ConfigManager {
public:
    static int startConfig(const boost::program_options::variables_map& vm);
};

}