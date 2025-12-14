#pragma once

#include <string>

#include "modules/config/device_profile.hpp"

namespace SnowOwl::Edge::Config {

class DeviceConfig {
public:
    static DeviceProfile loadFromFile(const std::string& path);
};

}
