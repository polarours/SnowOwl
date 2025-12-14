#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "../cli/managers/edge_manager.hpp"

using namespace SnowOwl::Cli::Managers;

int main(int argc, char* argv[]) {
    try {

        boost::program_options::options_description desc("SnowOwl Edge Device Options");
        desc.add_options()
            ("help,h", "Show help information")
            ("config,c", boost::program_options::value<std::string>(), "Path to configuration file")
            ("list-devices", "List registered devices and exit")
            ("list-sources-json", "List registered devices in JSON format and exit")
            ("remove-device", boost::program_options::value<int>(), "Remove a device by ID from the database and exit")
            ("set-primary", boost::program_options::value<int>(), "Set a device as primary by ID and exit")
            ("connect-database", "Connect to the database")
            ("db-host", boost::program_options::value<std::string>()->default_value("localhost"), "Database host for connection")
            ("db-port", boost::program_options::value<int>()->default_value(5432), "Database port for connection")
            ("db-name", boost::program_options::value<std::string>()->default_value("snowowl_dev"), "Database name for connection")
            ("db-user", boost::program_options::value<std::string>()->default_value("snowowl_dev"), "Database user for connection")
            ("db-password", boost::program_options::value<std::string>(), "Database password for connection")
            ("db-path", boost::program_options::value<std::string>()->default_value("postgresql://snowowl_dev@localhost/snowowl_dev"), "Database path for device registry");


        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
        
        if (vm.count("help")) {
            std::cout << "SnowOwl Edge Device\n";
            std::cout << desc << std::endl;
            return 0;
        }

        return EdgeManager::startEdge(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
