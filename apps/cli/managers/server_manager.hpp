#pragma once

#include <boost/program_options.hpp>

#include "libs/utils/resource_tracker.hpp"
#include "libs/utils/system_probe.hpp"
#include "libs/utils/health_monitor.hpp"

namespace SnowOwl::Cli::Managers {

using SnowOwl::Utils::Monitoring::ResourceTracker;
using SnowOwl::Utils::Monitoring::SystemProbe;    

class ServerManager {
public:
    static int startServer(const boost::program_options::variables_map& vm);

private:
    ResourceTracker resourceTracker_;
    SystemProbe systemProbe_;

};

}