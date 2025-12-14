#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include "../cli/managers/client_manager.hpp"


using namespace SnowOwl::Cli::Managers;

int main(int argc, char* argv[]) {
    try {
        boost::program_options::options_description desc("SnowOwl Client Options");
        desc.add_options()
            ("help,h", "Show help information")
            ("web", "Run client in Web mode")
            ("flutter", "Run client in Flutter mode")
            ("url", boost::program_options::value<std::string>(), "Specify URL for Web client")
            ("device", boost::program_options::value<std::string>(), "Specify device for Flutter client");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
        
        if (vm.count("help")) {
            std::cout << "SnowOwl Client\n";
            std::cout << desc << std::endl;
            return 0;
        }

        return ClientManager::startClient(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}