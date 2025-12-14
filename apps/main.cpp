#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "cli/managers/server_manager.hpp"
#include "cli/managers/edge_manager.hpp"
#include "cli/managers/client_manager.hpp"

using SnowOwl::Cli::Managers::ServerManager;
using SnowOwl::Cli::Managers::EdgeManager;
using SnowOwl::Cli::Managers::ClientManager;

namespace po = boost::program_options;

void show_help() {
    const std::string help_text = R"(
Owl Unified Command Line Interface

Usage: owl [OPTIONS] [SUBCOMMAND] [ACTION]

Subcommands:
  server     Server component operations
  edge       Edge device component operations
  client     Client component operations

Actions:
  start      Start the component
  (other actions may be added in the future)

Global Options:
  -h [ --help ]         Show this help message
  -v [ --version ]      Print version information

Server Options:
  Run 'owl server start --help' for server-specific options
  Or use 'owlctl' to manage a running server

Edge Device Options:
  Run 'owl edge start --help' for edge-specific options
  Or use 'owlctl' to manage edge devices

Client Options:
  Run 'owl client start --help' for client-specific options

Management Tool:
  Use 'owlctl' to manage a running server:
    --list-devices        List all registered devices
    --server-status       Get server status
    --update-config       Update server configuration
    --get-config          Get configuration value by key
    --list-config         List all configuration
    --reset-config        Reset configuration to defaults
    --start-stream        Start stream for device
    --stop-stream         Stop stream for device
    --register-device     Register edge device
    --update-device       Update edge device
    --delete-device       Delete edge device
    --device-info         Get device information

Examples:
  owl server start --enable-rtmp --rtmp-url "rtmp://127.0.0.1:1935/live/stream" --ingest-port 7500 --http-port 8081
  owl edge start
  owl client start --web --url="http://127.0.0.1:8081"
  owlctl --list-devices
  owlctl --server-status
)";
    std::cout << help_text << std::endl;
}

void show_version() {
    std::cout << "Owl Version 0.1.0" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        po::options_description global("Global options");
        global.add_options()
            ("help,h", "Show help message")
            ("version,v", "Show version");

        po::parsed_options parsed = po::command_line_parser(argc, argv)
            .options(global)
            .allow_unregistered()
            .run();

        po::variables_map vm;
        po::store(parsed, vm);
        po::notify(vm);

        if (vm.count("help")) {
            show_help();
            return 0;
        }

        if (vm.count("version")) {
            show_version();
            return 0;
        }

        std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
        
        if (opts.empty()) {
            show_help();
            return 0;
        }

        const std::string& subcommand = opts[0];
        
        std::vector<char*> sub_args;
        sub_args.push_back(argv[0]);
        
        sub_args.push_back(const_cast<char*>(subcommand.c_str()));
        
        for (size_t i = 1; i < opts.size(); ++i) {
            sub_args.push_back(const_cast<char*>(opts[i].c_str()));
        }
        
        sub_args.push_back(nullptr);

        if (subcommand == "server") {
            if (opts.size() > 1 && opts[1] == "start") {
                po::options_description server_desc("Owl Server Options");
                server_desc.add_options()
                    ("help,h", "Show help message")
                    ("version,v", "Show version")
                    ("config,c", po::value<std::string>(), "Path to configuration file")
                    ("dry-run", "Load configuration, print effective stream outputs, then exit")
                    ("enable-rtmp", po::value<bool>()->default_value(false)->implicit_value(true), "Enable RTMP output")
                    ("enable-tcp", po::value<bool>()->default_value(true)->implicit_value(true), "Enable legacy TCP broadcast output")
                    ("enable-hls", po::value<bool>()->default_value(false)->implicit_value(true), "Enable HLS output")
                    ("enable-rtsp", po::value<bool>()->default_value(false)->implicit_value(true), "Enable RTSP output stream")
                    ("enable-webrtc", po::value<bool>()->default_value(false)->implicit_value(true), "Enable WebRTC output stream")
                    ("rtmp-url", po::value<std::string>(), "RTMP server URL")
                    ("rtsp-url", po::value<std::string>(), "RTSP server URL")
                    ("rtmp-mount", po::value<std::string>(), "RTMP mount path (e.g. /snowowl/main)")
                    ("rtsp-mount", po::value<std::string>(), "RTSP mount path (e.g. /snowowl/main)")
                    ("ingest-port", po::value<int>()->default_value(7500), "TCP port for ingesting streams")
                    ("http-port", po::value<int>()->default_value(8081), "HTTP port for REST API")
                    ("listen-port", po::value<int>()->default_value(7000), "TCP port for accepting client connections")
                    ("config-db", po::value<std::string>()->default_value("postgresql://snowowl_dev@localhost/snowowl_dev"), "Database connection string")
                    ("connect-database", "Connect to database")
                    ("db-host", po::value<std::string>()->default_value("localhost"), "Database host for connection")
                    ("db-port", po::value<int>()->default_value(5432), "Database port for connection")
                    ("db-name", po::value<std::string>()->default_value("snowowl_dev"), "Database name for connection")
                    ("db-user", po::value<std::string>()->default_value("snowowl_dev"), "Database user for connection")
                    ("db-password", po::value<std::string>(), "Database password for connection")
                    ("list-sources", "List registered devices")
                    ("list-sources-json", "List registered devices in JSON format")
                    ("list-devices", "List registered devices")
                    ("remove-device", po::value<int>(), "Remove a device by ID")
                    ("set-primary", po::value<int>(), "Set a device as primary")
                    ("set-device-name", po::value<std::string>(), "Set device name (requires --device-id)")
                    ("device-id", po::value<int>(), "Device ID for operations that require it")
                    ("forward-stream", po::value<std::string>(), "Forward stream from specific device")
                    ("use-stream-receiver", "Use stream receiver instead of local capture")
                    ("daemon", "Run server as daemon (background process)")
                    ("pid-file", po::value<std::string>(), "Write PID to file when running as daemon")
                    ("discover-devices", "Discover devices on the network")
                    ("discover-network-range", po::value<std::string>()->default_value("192.168.1.0/24"), "Network range for device discovery")
                    ("register-device", "Register a new device")
                    ("source-type", po::value<std::string>(), "Source type for device registration (camera, rtsp, rtmp, file)")
                    ("source-id", po::value<int>(), "Source ID to use")
                    ("device-name", po::value<std::string>()->default_value("Unnamed Device"), "Device name")
                    ("camera-id", po::value<int>()->default_value(0), "Camera ID for camera sources")
                    ("source-uri", po::value<std::string>(), "URI for network sources")
                    ("fallback-uri", po::value<std::string>(), "Fallback URI for network sources")
                    ("id", po::value<int>(), "Custom device ID")
                    ("enable-rest", po::value<bool>()->default_value(true), "Enable REST API");

                po::variables_map server_vm;
                po::store(po::command_line_parser(sub_args.size() - 1, sub_args.data())
                          .options(server_desc)
                          .run(), 
                          server_vm);
                po::notify(server_vm);

                if (server_vm.count("help")) {
                    std::cout << "Owl Server\n";
                    std::cout << server_desc << std::endl;
                    return 0;
                }

                if (server_vm.count("version")) {
                    show_version();
                    return 0;
                }

                return ServerManager::startServer(server_vm);
            } else {
                std::cout << "Usage: owl server start [options]" << std::endl;
                return 1;
            }
        }
        else if (subcommand == "edge") {
            if (opts.size() > 1 && opts[1] == "start") {
                po::options_description edge_desc("Owl Edge Device Options");
                edge_desc.add_options()
                    ("help,h", "Show help information")
                    ("config,c", po::value<std::string>(), "Path to configuration file")
                    ("list-devices", "List registered devices and exit")
                    ("list-sources-json", "List registered devices in JSON format and exit")
                    ("remove-device", po::value<int>(), "Remove a device by ID from the database and exit")
                    ("set-primary", po::value<int>(), "Set a device as primary by ID and exit")
                    ("connect-database", "Connect to the database")
                    ("db-host", po::value<std::string>()->default_value("localhost"), "Database host for connection")
                    ("db-port", po::value<int>()->default_value(5432), "Database port for connection")
                    ("db-name", po::value<std::string>()->default_value("snowowl_dev"), "Database name for connection")
                    ("db-user", po::value<std::string>()->default_value("snowowl_dev"), "Database user for connection")
                    ("db-password", po::value<std::string>(), "Database password for connection")
                    ("db-path", po::value<std::string>()->default_value("postgresql://snowowl_dev@localhost/snowowl_dev"), "Database path for device registry");

                po::variables_map edge_vm;
                po::store(po::command_line_parser(sub_args.size() - 1, sub_args.data())
                          .options(edge_desc)
                          .run(), 
                          edge_vm);
                po::notify(edge_vm);

                if (edge_vm.count("help")) {
                    std::cout << "Owl Edge Device\n";
                    std::cout << edge_desc << std::endl;
                    return 0;
                }

                return EdgeManager::startEdge(edge_vm);
            } else {
                std::cout << "Usage: owl edge start [options]" << std::endl;
                return 1;
            }
        }
        else if (subcommand == "client") {
            if (opts.size() > 1 && opts[1] == "start") {
                po::options_description client_desc("Owl Client Options");
                client_desc.add_options()
                    ("help,h", "Show help information")
                    ("web", "Run client in Web mode")
                    ("flutter", "Run client in Flutter mode")
                    ("url", po::value<std::string>(), "Specify URL for Web client")
                    ("device", po::value<std::string>(), "Specify device for Flutter client");

                po::variables_map client_vm;
                po::store(po::command_line_parser(sub_args.size() - 1, sub_args.data())
                          .options(client_desc)
                          .run(), 
                          client_vm);
                po::notify(client_vm);

                if (client_vm.count("help")) {
                    std::cout << "Owl Client\n";
                    std::cout << client_desc << std::endl;
                    return 0;
                }

                return ClientManager::startClient(client_vm);
            } else {
                std::cout << "Usage: owl client start [options]" << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << "Unknown subcommand: " << subcommand << std::endl;
            show_help();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}