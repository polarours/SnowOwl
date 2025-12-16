#pragma once

#include <filesystem>
#include <string>

namespace SnowOwl::Utils::Paths {

std::filesystem::path dataRoot();
std::filesystem::path configFile(const std::string& fileName);

}
