#pragma once

#include <filesystem>
#include <string>

namespace SnowOwl::Common::Utils {

std::filesystem::path dataRoot();
std::filesystem::path configFile(const std::string& fileName);

}
