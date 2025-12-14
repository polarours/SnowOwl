#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "core/cli_options.hpp"
#include "commands/server_commands.hpp"
#include "commands/edge_commands.hpp"
#include "commands/client_commands.hpp"
#include "commands/device_commands.hpp"
#include "commands/config_commands.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    try {
        po::options_description mainDesc = getMainOptions();
        po::options_description serverDesc = getServerOptions();
        po::options_description edgeDesc = getEdgeOptions();
        po::options_description clientDesc = getClientOptions();
        po::options_description deviceDesc = getDeviceOptions();
        po::options_description configDesc = getConfigOptions();
        
        po::options_description allOptions;
        allOptions.add(mainDesc);
        allOptions.add(serverDesc);
        allOptions.add(edgeDesc);
        allOptions.add(clientDesc);
        allOptions.add(deviceDesc);
        allOptions.add(configDesc);
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, allOptions), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << "SnowOwl - Unified Command Line Interface" << std::endl;
            std::cout << allOptions << std::endl;
            return 0;
        }
        
        if (vm.count("version")) {
            std::cout << "SnowOwl CLI v0.1.0" << std::endl;
            return 0;
        }
        
        if (vm.count("server")) {
            return SnowOwl::Cli::Commands::executeServerCommand(vm);
        }
        
        if (vm.count("edge")) {
            return SnowOwl::Cli::Commands::executeEdgeCommand(vm);
        }
        
        if (vm.count("client")) {
            return SnowOwl::Cli::Commands::executeClientCommand(vm);
        }
        
        if (vm.count("device")) {
            return SnowOwl::Cli::Commands::executeDeviceCommand(vm);
        }
        
        if (vm.count("config")) {
            return SnowOwl::Cli::Commands::executeConfigCommand(vm);
        }
        
        std::cout << "SnowOwl - Unified Command Line Interface" << std::endl;
        std::cout << mainDesc << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
