#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "../cli/managers/server_manager.hpp"

int main(int argc, char* argv[]) {
    try {
        boost::program_options::options_description desc("SnowOwl Server Options");
        desc.add_options()
            ("help,h", "Show help message")
            ("version,v", "Show version")
            ("config,c", boost::program_options::value<std::string>(), "Path to configuration file")
            ("enable-rtmp", boost::program_options::value<bool>()->default_value(false), "Enable RTMP output")
            ("rtmp-url", boost::program_options::value<std::string>(), "RTMP server URL")
            ("enable-rtsp", boost::program_options::value<bool>()->default_value(false), "Enable RTSP output")
            ("rtsp-url", boost::program_options::value<std::string>(), "RTSP server URL")
            ("enable-hls", boost::program_options::value<bool>()->default_value(false), "Enable HLS output")
            ("hls-playlist", boost::program_options::value<std::string>(), "HLS playlist URL")
            ("ingest-port", boost::program_options::value<int>()->default_value(7500), "TCP port for ingesting streams")
            ("http-port", boost::program_options::value<int>()->default_value(8081), "HTTP port for REST API")
            ("listen-port", boost::program_options::value<int>()->default_value(7500), "TCP port for accepting connections")
            ("db-path", boost::program_options::value<std::string>(), "Database path for device registry")
            ("config-db", boost::program_options::value<std::string>()->default_value("postgresql://snowowl_dev@localhost/snowowl_dev"), "Database connection string")
            ("connect-database", "Connect to database")
            ("list-sources", "List registered devices")
            ("list-sources-json", "List registered devices in JSON format")
            ("list-devices", "List registered devices")
            ("remove-device", boost::program_options::value<std::string>(), "Remove a device by ID")
            ("set-primary", boost::program_options::value<std::string>(), "Set a device as primary")
            ("set-device-name", boost::program_options::value<std::string>(), "Set device name (requires --device-id)")
            ("device-id", boost::program_options::value<std::string>(), "Device ID for operations that require it")
            ("forward-stream", boost::program_options::value<std::string>(), "Forward stream from specific device")
            ("use-stream-receiver", "Use stream receiver instead of local capture")
            ("daemon", "Run server as daemon (background process)")
            ("pid-file", boost::program_options::value<std::string>(), "Write PID to file when running as daemon")
            ("discover-devices", "Discover devices on the network")
            ("discover-network-range", boost::program_options::value<std::string>()->default_value("192.168.1.0/24"), "Network range for device discovery")
            ("register-device", "Register a new device")
            ("source-type", boost::program_options::value<std::string>(), "Source type for device registration (camera, rtsp, rtmp, file)")
            ("source-id", boost::program_options::value<int>(), "Source ID to use")
            ("device-name", boost::program_options::value<std::string>()->default_value("Unnamed Device"), "Device name")
            ("camera-id", boost::program_options::value<int>()->default_value(0), "Camera ID for camera sources")
            ("source-uri", boost::program_options::value<std::string>(), "URI for network sources")
            ("fallback-uri", boost::program_options::value<std::string>(), "Fallback URI for network sources")
            ("id", boost::program_options::value<int>(), "Custom device ID")
            ("enable-rest", boost::program_options::value<bool>()->default_value(true), "Enable REST API")
            ("enable-websocket", boost::program_options::value<bool>()->default_value(true), "Enable WebSocket API");


        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
        
        if (vm.count("help")) {
            std::cout << "SnowOwl Server\n";
            std::cout << desc << std::endl;
            return 0;
        }
        
        if (vm.count("version")) {
            std::cout << "SnowOwl Server Version 0.1.0" << std::endl;
            return 0;
        }


        return SnowOwl::Cli::Managers::ServerManager::startServer(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}