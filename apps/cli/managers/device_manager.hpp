#pragma once

#include <boost/program_options.hpp>

namespace SnowOwl::Cli::Managers {

class DeviceManager {
public:
    static int startDevice(const boost::program_options::variables_map& vm);
};

}