#pragma once

#include <string>
#include <vector>

void storeCommandLineArguments(int argc, char* argv[]);

std::vector<std::string> getCommandLineArguments();

std::string serverHostName();