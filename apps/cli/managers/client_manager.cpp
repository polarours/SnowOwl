#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "client_manager.hpp"
#include "core/client_state.hpp"
#include "web/web_client.hpp"
#include "flutter/flutter_client.hpp"

namespace SnowOwl::Cli::Managers {

namespace po = boost::program_options;
using namespace SnowOwl::Client::Core;
using namespace SnowOwl::Client::Web;
using namespace SnowOwl::Client::Flutter;


enum class ClientMode {
    Web,
    Flutter,
    Unknown
};

int runWebClient(const std::vector<std::string>& args) {
    ClientState state;
    
    std::string url = "http://localhost:8081";
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "--url") {
            url = args[i + 1];
            break;
        }
    }
    
    state.setServerUrl(url);
    
    WebClient webClient(state);
    return webClient.launch() ? 0 : 1;
}

int runFlutterClient(const std::vector<std::string>& args) {
    ClientState state;
    
    std::string device;
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "--device") {
            device = args[i + 1];
            break;
        }
    }
    
    FlutterClient flutterClient(state);
    return flutterClient.launch(device);
}

void showHelp() {
    std::cout << "SnowOwl Client Options:\n";
    std::cout << "  -h [ --help ]         Show help information\n";
    std::cout << "  start                 Start client (default action)\n";
    std::cout << "  --web                 Run client in Web mode\n";
    std::cout << "  --flutter             Run client in Flutter mode\n";
    std::cout << "  --url arg             Specify URL for Web client\n";
    std::cout << "  --device arg          Specify device for Flutter client\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  snowowl client start --web --url=\"http://127.0.0.1:8081\"\n";
    std::cout << "  snowowl client start --flutter --device linux\n";
}

int ClientManager::startClient(const po::variables_map& vm) {
    
    std::vector<std::string> passthrough_args;
    
    ClientMode mode = ClientMode::Unknown;
    if (vm.count("web")) {
        mode = ClientMode::Web;
    } else if (vm.count("flutter")) {
        mode = ClientMode::Flutter;
    } else {
        mode = ClientMode::Web;
    }

    if (vm.count("url")) {
        passthrough_args.push_back("--url");
        passthrough_args.push_back(vm["url"].as<std::string>());
    }
    
    if (vm.count("device")) {
        passthrough_args.push_back("--device");
        passthrough_args.push_back(vm["device"].as<std::string>());
    }

    switch (mode) {
        case ClientMode::Web:
            return runWebClient(passthrough_args);
        case ClientMode::Flutter:
            return runFlutterClient(passthrough_args);
        default:
            std::cerr << "Unknown client mode" << std::endl;
            return 1;
    }
}

}