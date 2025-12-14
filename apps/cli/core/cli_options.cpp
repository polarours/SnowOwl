#include "cli_options.hpp"

po::options_description getMainOptions() {
    po::options_description desc("SnowOwl CLI");
    desc.add_options()
        ("help,h", "Show help message")
        ("version,v", "Show version")
        ("server", "Run server component")
        ("edge", "Run edge device component")
        ("client", "Run client component")
        ("device", po::value<std::string>(), "Device management commands")
        ("config", po::value<std::string>(), "Configuration management commands");
    return desc;
}

po::options_description getServerOptions() {
    po::options_description desc("Server Options");
    desc.add_options()
        ("port", po::value<int>()->default_value(7500), "Server port")
        ("enable-rtmp", po::value<bool>()->default_value(false), "Enable RTMP output")
        ("rtmp-url", po::value<std::string>(), "RTMP server URL")
        ("enable-hls", po::value<bool>()->default_value(false), "Enable HLS output")
        ("hls-playlist", po::value<std::string>(), "HLS playlist URL")
        ("ingest-port", po::value<int>()->default_value(7500), "TCP port for ingesting streams")
        ("http-port", po::value<int>()->default_value(8081), "HTTP port for REST API")
        ("db-path", po::value<std::string>(), "Database path for device registry")
        ("connect-database", "Connect to database")
        ("daemon", "Run server as daemon (background process)")
        ("pid-file", po::value<std::string>(), "Write PID to file when running as daemon");
    return desc;
}

po::options_description getEdgeOptions() {
    po::options_description desc("Edge Options");
    desc.add_options()
        ("config", po::value<std::string>(), "Path to edge device configuration file")
        ("server-url", po::value<std::string>(), "Server URL")
        ("device-id", po::value<std::string>(), "Device ID")
        ("device-uri", po::value<std::string>(), "Device URI (camera address, etc.)")
        ("daemon", "Run edge as daemon (background process)")
        ("pid-file", po::value<std::string>(), "Write PID to file when running as daemon");
    return desc;
}

po::options_description getClientOptions() {
    po::options_description desc("Client Options");
    desc.add_options()
        ("start", "Start client")
        ("web", "Start web client")
        ("flutter", "Start Flutter client")
        ("qt", "Start Qt client")
        ("url", po::value<std::string>(), "Server URL")
        ("device", po::value<std::string>(), "Device identifier");
    return desc;
}

po::options_description getDeviceOptions() {
    po::options_description desc("Device Management Options");
    desc.add_options()
        ("list", "List all devices")
        ("register", "Register new device")
        ("update", po::value<std::string>(), "Update device")
        ("delete", po::value<std::string>(), "Delete device")
        ("info", po::value<std::string>(), "Show device information")
        ("device-id", po::value<std::string>(), "Device ID")
        ("name", po::value<std::string>(), "Device name")
        ("uri", po::value<std::string>(), "Device URI")
        ("kind", po::value<std::string>(), "Device kind");
    return desc;
}

po::options_description getConfigOptions() {
    po::options_description desc("Configuration Options");
    desc.add_options()
        ("list", "List configuration")
        ("set", po::value<std::vector<std::string>>()->multitoken(), "Set configuration key-value pair")
        ("get", po::value<std::string>(), "Get configuration value")
        ("reset", "Reset configuration");
    return desc;
}