#include "server_manager.hpp"
#include <atomic>
#include <array>
#include <vector>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <boost/program_options.hpp>
#include <gst/gst.h>
#include <nlohmann/json.hpp>

#include "../../../libs/config/device_registry.hpp"
#include "../../../libs/config/config_manager.hpp"
#include "plugin/plugin_manager.hpp"
#include "core/streams/stream_dispatcher.hpp"
#include "core/streams/video_capture_manager.hpp"
#include "core/streams/video_processor.hpp"
#include "modules/network/network_server.hpp"
#include "modules/ingest/stream_receiver.hpp"
#include "modules/api/rest/rest_server.hpp"
#include "../../../libs/utils/resource_tracker.hpp"
#include "../../../libs/utils/system_probe.hpp"
#include "../../../libs/utils/health_monitor.hpp"
#ifdef HAVE_GRPC
#include "modules/api/grpc/grpc_server.hpp"
#endif
#include "modules/api/unified/api_server.hpp"
#include "modules/discovery/device_discovery.hpp"

namespace SnowOwl::Cli::Managers {

namespace po = boost::program_options;

namespace {

std::string serverHostName() {
#if defined(_WIN32)
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameA(buffer, &size) != 0 && size > 0) {
        return std::string(buffer, size);
    }
    return "windows";
#elif defined(__APPLE__)
    char buffer[256];
    std::size_t size = sizeof(buffer);
    if (sysctlbyname("kern.hostname", buffer, &size, nullptr, 0) == 0 && size > 0) {
        return std::string(buffer, size - 1);
    }
    return "macos";
#elif defined(__linux__)
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer) - 1) == 0) {
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
    }
    return "linux";
#else
    return "unknown";
#endif
}

nlohmann::json makeServerOriginMetadata() {
    nlohmann::json metadata = nlohmann::json::object();
    metadata["origin"] = "server";
    metadata["server"] = {
        {"host", serverHostName()}
    };
    metadata["stream_outputs"] = {
        {"tcp", {{"enabled", true}}},
        {"rtmp", {{"enabled", false}, {"url", std::string{}}, {"stream_key", std::string{}}}},
        {"hls", {{"enabled", false}, {"playlist", std::string{}}, {"segment_path", std::string{}}}},
        {"rtsp", {{"enabled", false}, {"url", std::string{}}, {"stream_key", std::string{}}}},
        {"webrtc", {{"enabled", false}}}
    };
    return metadata;
}

void ensureStreamOutputsMetadata(nlohmann::json& metadata) {
    if (!metadata.is_object()) {
        metadata = makeServerOriginMetadata();
        return;
    }

    const auto bootstrap = makeServerOriginMetadata();
    const auto outputs = bootstrap.at("stream_outputs");

    if (!metadata.contains("stream_outputs") || !metadata["stream_outputs"].is_object()) {
        metadata["stream_outputs"] = outputs;
    } else {
        auto& current = metadata["stream_outputs"];
        for (auto& [key, value] : outputs.items()) {
            if (!current.contains(key) || !current[key].is_object()) {
                current[key] = value;
            } else if (key == "rtmp") {
                if (!current[key].contains("url")) {
                    current[key]["url"] = std::string{};
                }
                if (!current[key].contains("stream_key")) {
                    current[key]["stream_key"] = std::string{};
                }
            } else if (key == "hls") {
                if (!current[key].contains("playlist")) {
                    current[key]["playlist"] = std::string{};
                }
                if (!current[key].contains("segment_path")) {
                    current[key]["segment_path"] = std::string{};
                }
            }

            if (!current[key].contains("enabled")) {
                current[key]["enabled"] = value.value("enabled", false);
            }
        }
    }

    const char* rtmpEnv = std::getenv("ARCTICOWL_RTMP_OUTPUT_URL");
    const char* hlsEnv = std::getenv("ARCTICOWL_HLS_BASE_URL");

    if (metadata.contains("stream_outputs")) {
        auto& outputsRef = metadata["stream_outputs"];

        if (rtmpEnv && outputsRef.contains("rtmp")) {
            outputsRef["rtmp"]["url"] = std::string(rtmpEnv);
            outputsRef["rtmp"]["enabled"] = true;
        }

        if (outputsRef.contains("rtmp") && outputsRef.contains("hls")) {
            std::string streamKey = outputsRef["rtmp"].value("stream_key", std::string{});
            if (streamKey.empty()) {
                const auto rtmpUrl = outputsRef["rtmp"].value("url", std::string{});
                const auto lastSlash = rtmpUrl.find_last_of('/');
                if (lastSlash != std::string::npos && lastSlash + 1 < rtmpUrl.size()) {
                    streamKey = rtmpUrl.substr(lastSlash + 1);
                    outputsRef["rtmp"]["stream_key"] = streamKey;
                }
            }

            if (!streamKey.empty() && hlsEnv && *hlsEnv != '\0') {
                std::string playlist = hlsEnv;
                if (!playlist.empty() && playlist.back() != '/') {
                    playlist.push_back('/');
                }
                playlist += streamKey;
                playlist += ".m3u8";

                outputsRef["hls"]["playlist"] = playlist;
                outputsRef["hls"]["enabled"] = true;
            }
        }
    }
}

std::string resolveConfigDbPath(const std::string& argument, const char* executable) {
    if (!argument.empty() && argument != "postgresql://snowowl_dev@localhost/snowowl_dev") {
        return argument;
    }

    SnowOwl::Config::ConfigManager configMgr;
    if (configMgr.load()) {
        std::string defaultConnName = configMgr.getDefaultDatabaseConnectionName();
        if (!defaultConnName.empty()) {
            auto defaultConn = configMgr.getDefaultDatabaseConnection();
            return defaultConn.toConnectionString();
        }
        
        auto connections = configMgr.getAllDatabaseConnections();
        if (!connections.empty()) {
            return connections.begin()->second.toConnectionString();
        }
    }
    
    std::string postgresConnectionString = "postgresql://snowowl_dev@localhost/snowowl_dev";
    return postgresConnectionString;
}

std::atomic<bool> g_running{true};

void handleSignal(int) {
    g_running = false;
}

using SnowOwl::Server::Core::CaptureSourceKind;

struct SourceRouting {
    CaptureSourceKind sourceKind{CaptureSourceKind::Camera};
    int cameraId{0};
    std::string primaryUri;
    std::string secondaryUri;
    bool useForwardStream{false};
    std::string forwardDeviceId;
};

SourceRouting deriveSourceConfig(const SnowOwl::Config::DeviceRecord& device) {
    SourceRouting routing;

    using SnowOwl::Config::DeviceKind;

    nlohmann::json metadata = nlohmann::json::object();
    if (!device.metadata.empty()) {
        const auto parsed = nlohmann::json::parse(device.metadata, nullptr, false);
        if (!parsed.is_discarded() && parsed.is_object()) {
            metadata = parsed;
        }
    }

    auto detectForwardId = [](const std::string& uri) -> std::optional<std::string> {
        constexpr std::string_view prefix{"forward://"};
        if (uri.compare(0, prefix.size(), prefix) == 0) {
            std::string id = uri.substr(prefix.size());
            if (!id.empty()) {
                return id;
            }
        }
        return std::nullopt;
    };

    auto markForwardUri = [&](const std::string& uri) {
        if (routing.useForwardStream) {
            return;
        }
        if (const auto id = detectForwardId(uri)) {
            routing.useForwardStream = true;
            routing.forwardDeviceId = *id;
        }
    };

    switch (device.kind) {
        case DeviceKind::Camera: {
            routing.sourceKind = CaptureSourceKind::Camera;
            int cameraId = metadata.value("camera_id", 0);

            if (cameraId < 0 && device.uri.rfind("camera://", 0) == 0) {
                try {
                    cameraId = std::stoi(device.uri.substr(9));
                } catch (...) {
                    cameraId = 0;
                }
            }

            routing.cameraId = cameraId < 0 ? 0 : cameraId;
            break;
        }
        case DeviceKind::RTSP:
            routing.sourceKind = CaptureSourceKind::NetworkStream;
            routing.primaryUri = device.uri;
            break;
        case DeviceKind::RTMP:
            routing.sourceKind = CaptureSourceKind::RTMPStream;
            routing.primaryUri = device.uri;
            break;
        case DeviceKind::File:
            routing.sourceKind = CaptureSourceKind::File;
            routing.primaryUri = device.uri;
            break;
        default:
            routing.sourceKind = CaptureSourceKind::Camera;
            routing.cameraId = 0;
            break;
    }

    markForwardUri(device.uri);

    if (metadata.is_object()) {
        if (routing.sourceKind == CaptureSourceKind::Camera) {
            if (metadata.contains("camera_id")) {
                const int cameraId = metadata.value("camera_id", routing.cameraId);
                routing.cameraId = cameraId < 0 ? 0 : cameraId;
            }
        } else {
            const std::string metaPrimary = metadata.value("primary_uri", std::string{});
            if (!metaPrimary.empty()) {
                routing.primaryUri = metaPrimary;
            }

            const std::string metaStream = metadata.value("stream_uri", std::string{});
            if (!metaStream.empty()) {
                routing.primaryUri = metaStream;
            }

            auto setSecondary = [&](const std::string& value) {
                if (routing.secondaryUri.empty() && !value.empty() && value != routing.primaryUri) {
                    routing.secondaryUri = value;
                }
            };

            setSecondary(metadata.value("secondary_uri", std::string{}));
            setSecondary(metadata.value("fallback_uri", std::string{}));
            setSecondary(metadata.value("rtmp_uri", std::string{}));
            setSecondary(metadata.value("rtsp_uri", std::string{}));
        }

        if (!routing.useForwardStream && metadata.contains("edge_device") && metadata["edge_device"].is_object()) {
            const auto& edge = metadata["edge_device"];
            if (edge.value("forward_enabled", false)) {
                routing.useForwardStream = true;
                routing.forwardDeviceId = edge.value("id", routing.forwardDeviceId);
            }
        }
    }

    if (routing.sourceKind != CaptureSourceKind::Camera && routing.primaryUri.empty()) {
        routing.primaryUri = routing.secondaryUri;
        routing.secondaryUri.clear();
    }

    if (!routing.useForwardStream) {
        markForwardUri(routing.primaryUri);
        markForwardUri(routing.secondaryUri);
    }

    return routing;
}

SnowOwl::Server::Core::StreamTargetProfile deriveStreamProfile(const SnowOwl::Config::DeviceRecord& device) {
    SnowOwl::Server::Core::StreamTargetProfile profile;

    if (device.metadata.empty()) {
        profile.tcp.enabled = true;
        return profile;
    }

    const auto metadata = nlohmann::json::parse(device.metadata, nullptr, false);
    if (metadata.is_discarded() || !metadata.is_object()) {
        profile.tcp.enabled = true;
        return profile;
    }

    auto parseConfig = [](const nlohmann::json& node, SnowOwl::Server::Core::StreamOutputConfig& target) {
        target.parameters.clear();

        if (node.is_boolean()) {
            target.enabled = node.get<bool>();
            return;
        }

        if (!node.is_object()) {
            target.enabled = false;
            return;
        }

        target.enabled = node.value("enabled", true);
        for (const auto& [key, value] : node.items()) {
            if (key == "enabled") {
                continue;
            }

            if (value.is_string()) {
                target.parameters[key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                target.parameters[key] = std::to_string(value.get<long long>());
            } else if (value.is_number_float()) {
                target.parameters[key] = std::to_string(value.get<double>());
            } else if (value.is_boolean()) {
                target.parameters[key] = value.get<bool>() ? "true" : "false";
            } else {
                target.parameters[key] = value.dump();
            }
        }
    };

    const auto outputsIt = metadata.find("stream_outputs");
    if (outputsIt == metadata.end() || !outputsIt->is_object()) {
        profile.tcp.enabled = true;
        return profile;
    }

    const auto& outputs = *outputsIt;
    if (const auto it = outputs.find("tcp"); it != outputs.end()) {
        parseConfig(it.value(), profile.tcp);
    }
    if (const auto it = outputs.find("rtmp"); it != outputs.end()) {
        parseConfig(it.value(), profile.rtmp);
        if (profile.rtmp.enabled && !profile.rtmp.parameters.count("stream_key")) {
            if (const auto urlIt = profile.rtmp.parameters.find("url"); urlIt != profile.rtmp.parameters.end()) {
                const auto& url = urlIt->second;
                const auto lastSlash = url.find_last_of('/');
                if (lastSlash != std::string::npos && lastSlash + 1 < url.size()) {
                    profile.rtmp.parameters["stream_key"] = url.substr(lastSlash + 1);
                }
            }
        }
    }
    if (const auto it = outputs.find("rtsp"); it != outputs.end()) {
        parseConfig(it.value(), profile.rtsp);
    }
    if (const auto it = outputs.find("hls"); it != outputs.end()) {
        parseConfig(it.value(), profile.hls);
    }
    if (const auto it = outputs.find("webrtc"); it != outputs.end()) {
        parseConfig(it.value(), profile.webrtc);
    }

    if (!SnowOwl::Server::Core::hasAnyEnabled(profile)) {
        profile.tcp.enabled = true;
    }

    return profile;
}

void printStreamProfile(const SnowOwl::Server::Core::StreamTargetProfile& profile) {
    auto printEntry = [](const char* name, const SnowOwl::Server::Core::StreamOutputConfig& cfg) {
        std::cout << "  - " << std::setw(6) << std::left << name << " : "
                  << (cfg.enabled ? "enabled" : "disabled");
        if (!cfg.parameters.empty()) {
            std::cout << " (";
            bool first = true;
            for (const auto& [key, value] : cfg.parameters) {
                if (!first) {
                    std::cout << ", ";
                }
                first = false;
                std::cout << key << '=' << value;
            }
            std::cout << ')';
        }
        std::cout << std::endl;
    };

    const auto previousFlags = std::cout.flags();
    const auto previousFill = std::cout.fill();
    std::cout << std::left;

    std::cout << "Effective Stream Outputs:" << std::endl;
    printEntry("tcp", profile.tcp);
    printEntry("rtmp", profile.rtmp);
    printEntry("rtsp", profile.rtsp);
    printEntry("hls", profile.hls);
    printEntry("webrtc", profile.webrtc);

    std::cout.flags(previousFlags);
    std::cout.fill(previousFill);
}

}

int ServerManager::startServer(const po::variables_map& vm) {
    gst_init(nullptr, nullptr);

    namespace fs = std::filesystem;
    using SnowOwl::Utils::SystemResources::SystemProbe;
    using SnowOwl::Utils::SystemResources::ResourceTracker;
    using SnowOwl::Config::DeviceKind;
    using SnowOwl::Config::DeviceRecord;
    using SnowOwl::Config::DeviceRegistry;
    using SnowOwl::Config::deviceKindFromString;
    using SnowOwl::Config::toString;
    using SnowOwl::Server::Core::CaptureSourceKind;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif

    const bool dryRun = vm.count("dry-run") > 0;
    if (dryRun) {
        std::cout << "Dry run requested. Effective output configuration will be printed." << std::endl;
    }

    const auto listenPort = static_cast<std::uint16_t>(vm.count("listen-port") ? vm["listen-port"].as<int>() : 7000);
    const auto httpPort = static_cast<std::uint16_t>(vm.count("http-port") ? vm["http-port"].as<int>() : 8081);
    const auto ingestPort = static_cast<std::uint16_t>(vm.count("ingest-port") ? vm["ingest-port"].as<int>() : 7500);
    const bool enableRest = vm.count("enable-rest") ? vm["enable-rest"].as<bool>() : true;
    const bool enableWebsocket = vm.count("enable-websocket") ? vm["enable-websocket"].as<bool>() : true;
    const std::string configDbArg = vm.count("config-db")
        ? vm["config-db"].as<std::string>()
        : "postgresql://snowowl_dev@localhost/snowowl_dev";
    const fs::path dbPath = resolveConfigDbPath(configDbArg, nullptr);

    if (vm.count("connect-database")) {
        std::string host = vm.count("db-host") ? vm["db-host"].as<std::string>() : "localhost";
        int port = vm.count("db-port") ? vm["db-port"].as<int>() : 5432;
        std::string dbName = vm.count("db-name") ? vm["db-name"].as<std::string>() : "snowowl_dev";
        std::string user = vm.count("db-user") ? vm["db-user"].as<std::string>() : "snowowl_dev";
        std::string password = vm.count("db-password") ? vm["db-password"].as<std::string>() : "";

        std::string connectionString = "postgresql://" + user;
        if (!password.empty()) {
            connectionString += ":" + password;
        }
        connectionString += "@" + host + ":" + std::to_string(port) + "/" + dbName;

        std::cout << "==========================================\n";
        std::cout << "  🦉 SnowOwl Server - Database Connection  \n";
        std::cout << "==========================================\n";
        std::cout << "  Host:     " << host << std::endl;
        std::cout << "  Port:     " << port << std::endl;
        std::cout << "  Database: " << dbName << std::endl;
        std::cout << "  User:     " << user << std::endl;
        if (!password.empty()) {
            std::cout << "  Password: ***" << std::endl;
        } else {
            std::cout << "  Password: (not provided)" << std::endl;
        }
        std::cout << "==========================================\n";

        DeviceRegistry testRegistry;
        if (testRegistry.open(connectionString)) {
            std::cout << "\n✅ Database connection successful!" << std::endl;
            try {
                const auto devices = testRegistry.listDevices();
                std::cout << "✅ Database schema is accessible (" << devices.size() << " devices found)" << std::endl;
                std::cout << "\nConnection string for future use:" << std::endl;
                std::cout << "  --config-db \"" << connectionString << "\"" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "\n⚠️  Connection successful but unable to query devices: " << e.what() << std::endl;
            }
        } else {
            std::cout << "\n❌ Database connection failed!" << std::endl;
            return 1;
        }

        return 0;
    }

    if (!dbPath.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(dbPath.parent_path(), ec);
        if (ec) {
            std::cerr << "Failed to create configuration directory: " << ec.message() << std::endl;
            return 1;
        }
    }

    DeviceRegistry registry;
    if (!registry.open(dbPath.string())) {
        std::cerr << "❌ Failed to open device registry: " << dbPath << std::endl;
        return 1;
    }

    std::cout << "✅ Connected to database: " << dbPath << std::endl;
    std::cout << "🦉 Note: Edge devices can access device registry information through the same database connection" << std::endl;

    std::cout << "🔌 Initializing plugin system..." << std::endl;
    SnowOwl::PluginManager& pluginManager = SnowOwl::PluginManager::getInstance();
    std::string pluginDir = "/usr/local/lib/snowowl/plugins";
    if (const char* envDir = std::getenv("ARCTICOWL_PLUGIN_DIR")) {
        pluginDir = envDir;
    }
    if (pluginManager.loadPlugins(pluginDir)) {
        std::cout << "✅ Plugin system initialized successfully" << std::endl;
        pluginManager.initializePlugins();
    } else {
        std::cout << "⚠️  Failed to initialize plugin system" << std::endl;
    }

    if (vm.count("list-devices") || vm.count("list-sources")) {
        const auto devices = registry.listDevices();
        std::cout << "==========================================\n";
        std::cout << "  🦉 Registered devices (" << devices.size() << ")\n";
        std::cout << "==========================================\n";
        if (devices.empty()) {
            std::cout << "  <none>\n";
        } else {
            const auto previousFlags = std::cout.flags();
            std::cout << std::left
                      << "  " << std::setw(5) << "ID"
                      << std::setw(16) << "Kind"
                      << std::setw(12) << "Enabled"
                      << std::setw(12) << "Primary"
                      << "Name -> URI" << std::endl;
            std::cout << "  " << std::string(60, '-') << std::endl;
            for (const auto& device : devices) {
                std::cout << "  " << std::setw(5) << device.id
                          << std::setw(16) << toString(device.kind)
                          << std::setw(12) << (device.enabled ? "yes" : "no")
                          << std::setw(12) << (device.isPrimary ? "yes" : "no")
                          << device.name << " -> " << device.uri << std::endl;
            }
            std::cout.flags(previousFlags);
        }
        std::cout << "==========================================\n";
        return 0;
    }

    if (vm.count("list-sources-json")) {
        const auto devices = registry.listDevices();
        nlohmann::json result = nlohmann::json::array();
        for (const auto& device : devices) {
            nlohmann::json item;
            item["id"] = device.id;
            item["name"] = device.name;
            item["kind"] = toString(device.kind);
            item["uri"] = device.uri;
            item["enabled"] = device.enabled;
            item["is_primary"] = device.isPrimary;

            if (!device.metadata.empty()) {
                auto metadata = nlohmann::json::parse(device.metadata, nullptr, false);
                if (!metadata.is_discarded()) {
                    item["metadata"] = metadata;
                }
            }

            result.push_back(item);
        }

        std::cout << result.dump(2) << std::endl;
        return 0;
    }

    if (vm.count("discover-devices")) {
        std::string networkRange = vm.count("discover-network-range")
            ? vm["discover-network-range"].as<std::string>()
            : "192.168.1.0/24";

        std::cout << "==========================================\n";
        std::cout << "  🦉 Device Discovery  \n";
        std::cout << "==========================================\n";

        SnowOwl::Server::Modules::Discovery::DeviceDiscovery deviceDiscovery;

        std::cout << "Scanning network range: " << networkRange << "...\n";
        auto networkDevices = deviceDiscovery.discoverNetworkDevices(networkRange);
        std::cout << "Found " << networkDevices.size() << " network devices:\n";
        for (const auto& device : networkDevices) {
            std::cout << "  IP: " << device.ipAddress
                      << ", MAC: " << device.macAddress
                      << ", Model: " << device.modelName
                      << ", Manufacturer: " << device.manufacturer << std::endl;
        }

        std::cout << "Scanning for local devices...\n";
        auto localDevices = deviceDiscovery.discoverLocalDevices();
        std::cout << "Found " << localDevices.size() << " local devices:\n";
        for (const auto& device : localDevices) {
            std::cout << "  Device: " << device.deviceId
                      << ", Name: " << device.name
                      << ", Manufacturer: " << device.manufacturer
                      << ", Model: " << device.model << std::endl;
        }

        const auto registeredDevices = registry.listDevices();
        std::cout << "Found " << registeredDevices.size() << " registered devices in database:\n";
        if (registeredDevices.empty()) {
            std::cout << "  <none>\n";
        } else {
            for (const auto& device : registeredDevices) {
                std::cout << "  ID: " << device.id
                          << ", Name: " << device.name
                          << ", Kind: " << toString(device.kind)
                          << ", URI: " << device.uri << std::endl;
            }
        }

        std::cout << "==========================================\n";
        return 0;
    }

    if (vm.count("remove-device")) {
        const int deviceId = vm["remove-device"].as<int>();
        if (deviceId <= 0) {
            std::cerr << "❌ Error: Device ID must be greater than zero" << std::endl;
            return 1;
        }

        auto device = registry.findById(deviceId);
        if (!device) {
            std::cerr << "❌ Error: Device with ID " << deviceId << " not found" << std::endl;
            return 1;
        }

        if (registry.removeDevice(deviceId)) {
            std::cout << "✅ Successfully removed device:" << std::endl;
            std::cout << "  ID: " << device->id << std::endl;
            std::cout << "  Name: " << device->name << std::endl;
            std::cout << "  Kind: " << toString(device->kind) << std::endl;
            std::cout << "  URI: " << device->uri << std::endl;
        } else {
            std::cerr << "❌ Failed to remove device with ID " << deviceId << std::endl;
            return 1;
        }

        return 0;
    }

    if (vm.count("set-primary")) {
        const int deviceId = vm["set-primary"].as<int>();
        if (deviceId <= 0) {
            std::cerr << "❌ Error: Device ID must be greater than zero" << std::endl;
            return 1;
        }

        auto device = registry.findById(deviceId);
        if (!device) {
            std::cerr << "❌ Error: Device with ID " << deviceId << " not found" << std::endl;
            return 1;
        }

        if (registry.setPrimaryDevice(deviceId)) {
            std::cout << "✅ Successfully set device as primary:" << std::endl;
            std::cout << "  ID: " << device->id << std::endl;
            std::cout << "  Name: " << device->name << std::endl;
            std::cout << "  Kind: " << toString(device->kind) << std::endl;
            std::cout << "  URI: " << device->uri << std::endl;
        } else {
            std::cerr << "❌ Failed to set device with ID " << deviceId << " as primary" << std::endl;
            return 1;
        }

        return 0;
    }

    if (vm.count("set-device-name")) {
        if (!vm.count("device-id")) {
            std::cerr << "❌ Error: --device-id is required when using --set-device-name" << std::endl;
            return 1;
        }

        const int deviceId = vm["device-id"].as<int>();
        const std::string newName = vm["set-device-name"].as<std::string>();
        if (deviceId <= 0) {
            std::cerr << "❌ Error: Device ID must be greater than zero" << std::endl;
            return 1;
        }

        auto device = registry.findById(deviceId);
        if (!device) {
            std::cerr << "❌ Error: Device with ID " << deviceId << " not found" << std::endl;
            return 1;
        }

        device->name = newName;
        const auto updatedDevice = registry.upsertDevice(*device);
        if (updatedDevice.id > 0) {
            std::cout << "✅ Successfully updated device name:" << std::endl;
            std::cout << "  ID: " << updatedDevice.id << std::endl;
            std::cout << "  Name: " << updatedDevice.name << std::endl;
            std::cout << "  Kind: " << toString(updatedDevice.kind) << std::endl;
            std::cout << "  URI: " << updatedDevice.uri << std::endl;
        } else {
            std::cerr << "❌ Failed to update device name for device with ID " << deviceId << std::endl;
            return 1;
        }

        return 0;
    }

    if (vm.count("register-device")) {
        if (!vm.count("source-type")) {
            std::cerr << "❌ Error: --source-type is required for device registration" << std::endl;
            return 1;
        }

        const std::string typeString = vm["source-type"].as<std::string>();
        const DeviceKind kind = deviceKindFromString(typeString);
        if (kind == DeviceKind::Unknown) {
            std::cerr << "❌ Error: Unsupported video source type: " << typeString << std::endl;
            std::cerr << "Supported types: camera, rtsp, rtmp, file" << std::endl;
            return 1;
        }

        DeviceRecord record;
        record.name = vm.count("device-name") ? vm["device-name"].as<std::string>() : "Unnamed Device";
        record.kind = kind;
        record.enabled = true;
        record.isPrimary = vm.count("set-primary") > 0;

        if (vm.count("id")) {
            int customId = vm["id"].as<int>();
            if (customId <= 0) {
                std::cerr << "❌ Error: ID must be greater than zero" << std::endl;
                return 1;
            }
            record.id = customId;
        }

        nlohmann::json metadata = nlohmann::json::object();
        if (kind == DeviceKind::Camera) {
            const int cameraId = vm.count("camera-id") ? vm["camera-id"].as<int>() : 0;
            if (cameraId < 0) {
                std::cerr << "❌ Error: camera-id cannot be negative" << std::endl;
                return 1;
            }
            record.uri = "camera://" + std::to_string(cameraId);
            metadata["camera_id"] = cameraId;
        } else {
            if (!vm.count("source-uri")) {
                std::cerr << "❌ Error: source-uri parameter is required for the selected source type" << std::endl;
                return 1;
            }
            record.uri = vm["source-uri"].as<std::string>();

            if (kind == DeviceKind::RTSP) {
                metadata["rtsp_uri"] = record.uri;
            } else if (kind == DeviceKind::RTMP) {
                metadata["rtmp_uri"] = record.uri;
                if (record.uri.rfind("forward://", 0) == 0) {
                    metadata["forward_device_id"] = record.uri.substr(10);
                }
            } else if (kind == DeviceKind::File) {
                metadata["file_path"] = record.uri;
            }
        }

        if (vm.count("fallback-uri")) {
            const std::string fallback = vm["fallback-uri"].as<std::string>();
            if (!fallback.empty()) {
                metadata["fallback_uri"] = fallback;
                if (fallback.rfind("rtmp://", 0) == 0) {
                    metadata["rtmp_uri"] = fallback;
                } else if (fallback.rfind("rtsp://", 0) == 0) {
                    metadata["rtsp_uri"] = fallback;
                }
            }
        }

        if (!metadata.empty()) {
            record.metadata = metadata.dump();
        }

        const auto result = registry.upsertDevice(record);
        if (result.id > 0) {
            std::cout << "✅ Successfully registered device:" << std::endl;
            std::cout << "  ID: " << result.id << std::endl;
            std::cout << "  Name: " << result.name << std::endl;
            std::cout << "  Kind: " << toString(result.kind) << std::endl;
            std::cout << "  URI: " << result.uri << std::endl;
            std::cout << "  Enabled: " << (result.enabled ? "yes" : "no") << std::endl;
            std::cout << "  Primary: " << (result.isPrimary ? "yes" : "no") << std::endl;
            if (!result.metadata.empty()) {
                std::cout << "  Metadata: " << result.metadata << std::endl;
            }
        } else {
            std::cerr << "❌ Failed to register device" << std::endl;
            std::cerr << "Device record ID was: " << record.id << std::endl;
            std::cerr << "Device record name was: " << record.name << std::endl;
            std::cerr << "Device record URI was: " << record.uri << std::endl;
            return 1;
        }

        return 0;
    }

    if (vm.count("source-id") && vm.count("source-type")) {
        std::cerr << "❌ Error: Specify either --source-id or --source-type, not both" << std::endl;
        return 1;
    }

    std::optional<DeviceRecord> activeDevice;
    if (vm.count("source-id")) {
        const int desiredId = vm["source-id"].as<int>();
        if (desiredId <= 0) {
            std::cerr << "❌ Error: source-id must be greater than zero" << std::endl;
            return 1;
        }

        activeDevice = registry.findById(desiredId);
        if (!activeDevice) {
            std::cerr << "❌ Error: Device with id " << desiredId << " not found" << std::endl;
            return 1;
        }

        if (!activeDevice->enabled) {
            std::cerr << "⚠️  Warning: Selected device is disabled in the registry" << std::endl;
        }
    } else if (vm.count("source-type")) {
        const std::string typeString = vm["source-type"].as<std::string>();
        const DeviceKind kind = deviceKindFromString(typeString);
        if (kind == DeviceKind::Unknown) {
            std::cerr << "❌ Error: Unsupported video source type: " << typeString << std::endl;
            return 1;
        }

        DeviceRecord record;
        record.name = vm.count("device-name") ? vm["device-name"].as<std::string>() : "Primary Source";
        record.kind = kind;
        record.enabled = true;
        record.isPrimary = true;

        nlohmann::json metadata = makeServerOriginMetadata();
        if (kind == DeviceKind::Camera) {
            const int cameraId = vm.count("camera-id") ? vm["camera-id"].as<int>() : 0;
            if (cameraId < 0) {
                std::cerr << "❌ Error: camera-id cannot be negative" << std::endl;
                return 1;
            }
            record.uri = "camera://" + std::to_string(cameraId);
            metadata["camera_id"] = cameraId;
        } else {
            if (!vm.count("source-uri")) {
                std::cerr << "❌ Error: source-uri parameter is required for the selected source type" << std::endl;
                return 1;
            }
            record.uri = vm["source-uri"].as<std::string>();
            metadata["primary_uri"] = record.uri;
            metadata["stream_uri"] = record.uri;

            if (kind == DeviceKind::RTSP) {
                metadata["rtsp_uri"] = record.uri;
            } else if (kind == DeviceKind::RTMP) {
                metadata["rtmp_uri"] = record.uri;
            } else if (kind == DeviceKind::File) {
                metadata["file_path"] = record.uri;
            }

            if (vm.count("fallback-uri")) {
                const std::string fallback = vm["fallback-uri"].as<std::string>();
                if (!fallback.empty()) {
                    metadata["fallback_uri"] = fallback;
                    metadata["secondary_uri"] = fallback;

                    if (fallback.rfind("rtmp://", 0) == 0) {
                        metadata["rtmp_uri"] = fallback;
                    } else if (fallback.rfind("rtsp://", 0) == 0) {
                        metadata["rtsp_uri"] = fallback;
                    }
                }
            }
        }

        if (auto existing = registry.findByUri(record.uri)) {
            record.id = existing->id;
            if (!existing->metadata.empty()) {
                const auto existingMeta = nlohmann::json::parse(existing->metadata, nullptr, false);
                if (!existingMeta.is_discarded() && existingMeta.is_object()) {
                    auto copyIfMissing = [&](const char* key) {
                        if (!metadata.contains(key) && existingMeta.contains(key)) {
                            metadata[key] = existingMeta.at(key);
                        }
                    };

                    if (!vm.count("fallback-uri")) {
                        copyIfMissing("fallback_uri");
                        copyIfMissing("secondary_uri");
                        copyIfMissing("rtmp_uri");
                        copyIfMissing("rtsp_uri");
                    }
                }
            }
        }

        ensureStreamOutputsMetadata(metadata);
        if (!metadata.empty()) {
            record.metadata = metadata.dump();
        }

        activeDevice = registry.upsertDevice(record);
    } else {
        activeDevice = registry.primaryDevice();
        if (!activeDevice) {
            const auto devices = registry.listDevices();
            if (devices.empty()) {
                std::cerr << "❌ Error: No devices registered. Use --register-device to add one." << std::endl;
                return 1;
            }
            activeDevice = devices.front();
        }
    }

    if (!activeDevice) {
        std::cerr << "❌ Error: No usable video source found" << std::endl;
        return 1;
    }

    if (activeDevice->id > 0) {
        registry.setPrimaryDevice(activeDevice->id);
    }

    const auto routing = deriveSourceConfig(*activeDevice);
    auto streamProfile = deriveStreamProfile(*activeDevice);

    if (vm.count("enable-tcp")) {
        streamProfile.tcp.enabled = vm["enable-tcp"].as<bool>();
    }
    if (vm.count("enable-rtmp")) {
        streamProfile.rtmp.enabled = vm["enable-rtmp"].as<bool>();
    }
    if (vm.count("enable-rtsp")) {
        streamProfile.rtsp.enabled = vm["enable-rtsp"].as<bool>();
    }
    if (vm.count("enable-webrtc")) {
        streamProfile.webrtc.enabled = vm["enable-webrtc"].as<bool>();
    }
    if (vm.count("enable-hls") && !vm["enable-hls"].defaulted()) {
        streamProfile.hls.enabled = vm["enable-hls"].as<bool>();
    }

    if (vm.count("rtmp-url")) {
        const std::string url = vm["rtmp-url"].as<std::string>();
        streamProfile.rtmp.parameters["url"] = url;
        streamProfile.rtmp.enabled = !url.empty();
        if (!url.empty()) {
            const auto lastSlash = url.find_last_of('/');
            if (lastSlash != std::string::npos && lastSlash + 1 < url.size()) {
                streamProfile.rtmp.parameters["stream_key"] = url.substr(lastSlash + 1);
            }
        }
    }

    if (vm.count("rtsp-url")) {
        const std::string url = vm["rtsp-url"].as<std::string>();
        streamProfile.rtsp.parameters["url"] = url;
        streamProfile.rtsp.enabled = !url.empty();
        if (!url.empty()) {
            const auto lastSlash = url.find_last_of('/');
            if (lastSlash != std::string::npos && lastSlash + 1 < url.size()) {
                streamProfile.rtsp.parameters["stream_key"] = url.substr(lastSlash + 1);
            }
        }
    }

    if (streamProfile.rtmp.enabled) {
        const auto it = streamProfile.rtmp.parameters.find("url");
        if (it == streamProfile.rtmp.parameters.end() || it->second.empty()) {
            std::cerr << "⚠️  Warning: RTMP output enabled but no URL provided." << std::endl;
        }
    }

    if (streamProfile.rtsp.enabled) {
        const auto it = streamProfile.rtsp.parameters.find("url");
        if (it == streamProfile.rtsp.parameters.end() || it->second.empty()) {
            std::cerr << "⚠️  Warning: RTSP output enabled but no URL provided." << std::endl;
        }
    }

    if (dryRun) {
        printStreamProfile(streamProfile);
        return 0;
    }

    const bool useStreamReceiver = routing.useForwardStream
        || (routing.sourceKind == CaptureSourceKind::Camera && routing.primaryUri.empty());

    SnowOwl::Modules::Ingest::StreamReceiver receiver;
    SnowOwl::Server::Core::VideoCaptureManager captureManager;
    SnowOwl::Server::Core::StreamDispatcher streamDispatcher;
    streamDispatcher.configure(streamProfile);

    bool outputsStarted = false;
    auto ensureOutputs = [&]() -> bool {
        if (outputsStarted) {
            return true;
        }
        printStreamProfile(streamProfile);
        if (!streamDispatcher.startOutputs()) {
            std::cerr << "❌ Error: Failed to initialise stream outputs" << std::endl;
            return false;
        }
        outputsStarted = true;
        return true;
    };

    std::unique_ptr<SnowOwl::Server::Core::VideoProcessor> receiverProcessor;
    if (useStreamReceiver) {
        receiverProcessor = std::make_unique<SnowOwl::Server::Core::VideoProcessor>();
    }

#ifdef HAVE_GRPC
    std::unique_ptr<SnowOwl::Server::Modules::Api::Grpc::GrpcServer> grpcServer;
#endif
    std::unique_ptr<SnowOwl::Server::Modules::Api::Unified::ApiServer> unifiedApiServer;

    SnowOwl::Server::Modules::Network::NetworkServer server(listenPort);

    if (receiverProcessor) {
        receiverProcessor->setNetworkServer(&server);
        receiverProcessor->setStreamProfile(streamProfile);
    }

    std::cout << "===============================================================================\n";
    std::cout << "  Version: 0.1.0                         Status: Starting...\n";
    std::cout << "-------------------------------------------------------------------------------\n";
    std::cout << "  📡 Network Configuration\n";
    std::cout << "     Main Server Port: " << listenPort << "\n";
    std::cout << "     Active Video Source: " << activeDevice->name << " (" << toString(activeDevice->kind) << ")";
    if (routing.sourceKind == CaptureSourceKind::Camera) {
        std::cout << " -> camera://" << routing.cameraId << "\n";
    } else {
        const std::string displayUri = !routing.primaryUri.empty() ? routing.primaryUri : routing.secondaryUri;
        if (!displayUri.empty()) {
            std::cout << " -> " << displayUri << "\n";
        } else {
            std::cout << " -> <no-uri>\n";
        }

        if (!routing.secondaryUri.empty() && routing.secondaryUri != displayUri) {
            std::cout << "        🔄 Fallback URI: " << routing.secondaryUri << "\n";
        }
    }
    std::cout << "\n";

    if (enableRest) {
        // Start unified API server (combines REST and WebSocket)
        unifiedApiServer = std::make_unique<SnowOwl::Server::Modules::Api::Unified::ApiServer>(registry, httpPort);
        if (useStreamReceiver && receiverProcessor) {
            unifiedApiServer->setVideoProcessor(receiverProcessor.get());
        } else if (!useStreamReceiver) {
            unifiedApiServer->setVideoProcessor(&captureManager.getProcessor());
        }
        
        if (!unifiedApiServer->start()) {
            std::cerr << "  ⚠️  Warning: Failed to start Unified API server on port " << httpPort << std::endl;
            unifiedApiServer.reset();
        }
    }
    
#ifdef HAVE_GRPC
        // Start gRPC server on port httpPort + 1000
        grpcServer = std::make_unique<SnowOwl::Server::Modules::Api::Grpc::GrpcServer>(
            "0.0.0.0:" + std::to_string(httpPort + 1000), registry);
        if (!grpcServer->start()) {
            std::cerr << "  ⚠️  Warning: Failed to start gRPC API on port " << (httpPort + 1000) << std::endl;
            grpcServer.reset();
        } else {
            std::cout << "     🌐 gRPC API listening on port " << (httpPort + 1000) << std::endl;
        }
#endif

    if (useStreamReceiver) {
        if (!ensureOutputs()) {
            if (unifiedApiServer) {
                unifiedApiServer->stop();
                unifiedApiServer.reset();
            }
            return 1;
        }

        if (!receiver.start(ingestPort)) {
            std::cerr << "❌ Failed to start edge stream receiver on port " << ingestPort << std::endl;
            if (unifiedApiServer) {
                unifiedApiServer->stop();
                unifiedApiServer.reset();
            }
            if (outputsStarted) {
                streamDispatcher.stopOutputs();
                outputsStarted = false;
            }
            return 1;
        }

        if (!routing.forwardDeviceId.empty()) {
            std::cout << "📡 Using edge stream receiver on port " << ingestPort
                      << " for device " << routing.forwardDeviceId << std::endl;
        } else {
            std::cout << "📡 Using edge stream receiver on port " << ingestPort << std::endl;
        }
    } else {
        if (!ensureOutputs()) {
            if (unifiedApiServer) {
                unifiedApiServer->stop();
                unifiedApiServer.reset();
            }
            return 1;
        }

        SnowOwl::Server::Core::CaptureSourceConfig managerConfig;
        managerConfig.kind = routing.sourceKind;
        managerConfig.cameraId = routing.cameraId;
        managerConfig.primaryUri = routing.primaryUri;
        managerConfig.secondaryUri = routing.secondaryUri;

        SnowOwl::Server::Core::VideoCaptureManager::FrameCallback frameCallback = [&](cv::Mat& frame) {
            if (!frame.empty()) {
                server.broadcastFrame(frame);
                streamDispatcher.onFrame(frame);
            }
        };

        auto detectionCallback = [&](const std::vector<SnowOwl::Detection::DetectionResult>& detections) {
            server.broadcastEvents(detections);
            streamDispatcher.onEvents(detections);
        };

        captureManager.getProcessor().setNetworkServer(&server);
        captureManager.getProcessor().setStreamProfile(streamProfile);

        if (!captureManager.start(managerConfig, frameCallback, detectionCallback)) {
            std::cerr << "❌ Error: Failed to start video capture" << std::endl;
            if (outputsStarted) {
                streamDispatcher.stopOutputs();
                outputsStarted = false;
            }
            if (unifiedApiServer) {
                unifiedApiServer->stop();
                unifiedApiServer.reset();
            }
            return 1;
        }
    }

    if (!server.startNetworkSystem()) {
        std::cerr << "  ❌ Error: Failed to start server" << std::endl;
        if (useStreamReceiver) {
            receiver.stop();
        } else {
            captureManager.stop();
        }
        if (outputsStarted) {
            streamDispatcher.stopOutputs();
            outputsStarted = false;
        }
        if (unifiedApiServer) {
            unifiedApiServer->stop();
            unifiedApiServer.reset();
        }
        return 1;
    }

    std::cout << "-------------------------------------------------------------------------------\n";
    std::cout << "  ✅ Server Status: RUNNING\n";
    std::cout << "     Main Server: http://localhost:" << listenPort << "\n";
    if (enableRest) {
        std::cout << "     REST API: http://localhost:" << httpPort << "/api/v1/\n";
        std::cout << "     WebSocket: ws://localhost:" << (httpPort + 1) << "/\n";
    }
#ifdef HAVE_GRPC
    std::cout << "     gRPC API: grpc://localhost:" << (httpPort + 1000) << "/\n";
#endif
    std::cout << "===============================================================================\n";
    std::cout << "  🚀 SnowOwl Server Started Successfully!\n";
    std::cout << "===============================================================================\n";

    std::string activeForwardDevice;
    while (g_running.load()) {
        if (useStreamReceiver) {
            SnowOwl::Modules::Ingest::ReceivedFrame received;
            if (!receiver.latestFrame(received)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            if (!routing.forwardDeviceId.empty()
                && !received.deviceId.empty()
                && received.deviceId != routing.forwardDeviceId) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            if (activeForwardDevice.empty() && !received.deviceId.empty()) {
                activeForwardDevice = received.deviceId;
                if (!routing.forwardDeviceId.empty() && routing.forwardDeviceId != activeForwardDevice) {
                    std::cout << "📥 StreamReceiver: processing frames from " << activeForwardDevice
                              << " (expected " << routing.forwardDeviceId << ")" << std::endl;
                } else {
                    std::cout << "📥 StreamReceiver: processing frames from " << activeForwardDevice << std::endl;
                }
            }

            cv::Mat frame = received.frame.clone();
            std::vector<SnowOwl::Detection::DetectionResult> detections;
            if (!frame.empty() && receiverProcessor) {
                detections = receiverProcessor->processFrame(frame);
            }

            if (!frame.empty()) {
                server.broadcastFrame(frame);
                streamDispatcher.onFrame(frame);
            }

            if (!detections.empty()) {
                server.broadcastEvents(detections);
            }
            streamDispatcher.onEvents(detections);

            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    server.stopNetworkSystem();
    if (useStreamReceiver) {
        receiver.stop();
    } else {
        captureManager.stop();
    }
    if (outputsStarted) {
        streamDispatcher.stopOutputs();
        outputsStarted = false;
    }

    if (unifiedApiServer) {
        unifiedApiServer->stop();
        unifiedApiServer.reset();
    }
    
#ifdef HAVE_GRPC
    if (grpcServer) {
        grpcServer->stop();
        grpcServer.reset();
    }
#endif

    std::cout << "⏹️  SnowOwl server stopped" << std::endl;
    return 0;
}

}