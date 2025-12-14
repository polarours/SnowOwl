#pragma once

#include <boost/program_options.hpp>

namespace SnowOwl::Cli::Managers {

class EdgeManager {
public:
    static int startEdge(const boost::program_options::variables_map& vm);
};

}