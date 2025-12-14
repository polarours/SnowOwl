#pragma once

#include <string>

namespace SnowOwl::Client::Web {

class WebClientLauncher {
public:
    static bool launch(const std::string& url);
};

}