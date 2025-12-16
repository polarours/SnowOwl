#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>
#include <boost/program_options.hpp>

#include "edge_manager.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#include "core/device_controller.hpp"
#include "modules/config/device_profile.hpp"
#include "utils/app_paths.hpp"
#include "config/device_registry.hpp"

namespace SnowOwl::Cli::Managers {

namespace {

std::atomic<bool> g_running{true};

void handleSignal(int sig) {
    g_running = false;
}

constexpr const char* kDefaultEdgeProfile = R"JSON({
  "device_id": "edge-device",
  "name": "Generic Edge",
  "compute_tier": "capture_only",
  "cpu_cores": 20,
  "memory_mb": 16384,
  "gpu_memory_mb":8096,
  "has_discrete_gpu": false,
  "supports_fp16": false,
  "detection": {
    "enable_on_device": false,
    "preferred_model": "yolov8n",
    "preferred_precision": "fp16",
    "model_format": "onnx",
    "max_model_size_mb": 32.0,
    "max_latency_ms": 200.0
  },
  "capture": {
    "kind": "camera",
    "camera_index": 0,
    "primary_uri": "",
    "fallback_uri": ""
  },
  "uplink": {
    "enable": true,
    "registry_path": "postgresql://snowowl_dev:SnowOwl_Dev!@localhost/snowowl_dev",
    "device_name": "Edge Camera",
    "set_primary": false,
    "auto_detect_cameras": true
  },
  "forward": {
    "enable": true,
    "host": "127.0.0.1",
    "port": 7500,
    "frame_interval_ms": 100,
    "reconnect_delay_ms": 2000
  }
})JSON";

std::vector<std::filesystem::path> templateCandidates(const std::filesystem::path& exePath) {
    namespace fs = std::filesystem;

    std::vector<fs::path> candidates;
    const fs::path cwdCandidate = fs::current_path() / "config/edge_device_profile.json";
    candidates.push_back(cwdCandidate);

    std::error_code ec;
    const fs::path canonicalExe = fs::weakly_canonical(exePath, ec);
    const fs::path base = ec ? fs::absolute(exePath) : canonicalExe;
    const fs::path exeDir = base.parent_path();

    std::array<fs::path, 4> relativeCandidates{
        exeDir / "config/edge_device_profile.json",
        exeDir / "../config/edge_device_profile.json",
        exeDir / "../../config/edge_device_profile.json",
        exeDir / "../share/SnowOwl/edge_device_profile.json"
    };

    candidates.insert(candidates.end(), relativeCandidates.begin(), relativeCandidates.end());
    return candidates;
}

void ensureProfileExists(const std::filesystem::path& targetPath, const std::vector<std::filesystem::path>& candidates) {
    if (std::filesystem::exists(targetPath)) {
        return;
    }

    for (const auto& candidate : candidates) {
        if (candidate.empty() || !std::filesystem::exists(candidate)) {
            continue;
        }

        std::error_code ec;
        std::filesystem::copy_file(candidate, targetPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            std::cout << "Edge Device: copied default profile to " << targetPath << std::endl;
            return;
        }
    }

    std::ofstream stream(targetPath);
    if (stream.is_open()) {
        stream << kDefaultEdgeProfile;
        stream.close();
        std::cout << "Edge Device: created default profile at " << targetPath << std::endl;
    } else {
        std::cerr << "Edge Device: failed to create profile at " << targetPath << std::endl;
    }
}

std::filesystem::path detectExecutablePath() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length > 0) {
        buffer.resize(length);
        return std::filesystem::path(buffer.begin(), buffer.end());
    }
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(buffer, ec);
        return ec ? std::filesystem::path(buffer) : canonical;
    }
#elif defined(__linux__)
    std::error_code ec;
    auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return exe;
    }
#endif
    return std::filesystem::current_path() / "owl";
}

std::string resolveProfilePath(const boost::program_options::variables_map& vm) {
    if (vm.count("config")) {
        return vm["config"].as<std::string>();
    }

    const auto userPath = SnowOwl::Utils::Paths::configFile("edge_device_profile.json");
    ensureProfileExists(userPath, templateCandidates(detectExecutablePath()));
    return userPath.string();
}

void listDevices(const std::string& dbPath) {
    SnowOwl::Config::DeviceRegistry registry;
    if (!registry.open(dbPath)) {
        std::cerr << "❌ Failed to open device registry: " << dbPath << std::endl;
        return;
    }

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
                      << std::setw(16) << SnowOwl::Config::toString(device.kind)
                      << std::setw(12) << (device.enabled ? "yes" : "no")
                      << std::setw(12) << (device.isPrimary ? "yes" : "no")
                      << device.name << " -> " << device.uri << std::endl;
        }
        std::cout.flags(previousFlags);
    }
    std::cout << "==========================================\n";
}

void listDevicesJson(const std::string& dbPath) {
    SnowOwl::Config::DeviceRegistry registry;
    if (!registry.open(dbPath)) {
        std::cerr << "❌ Failed to open device registry: " << dbPath << std::endl;
        return;
    }

    const auto devices = registry.listDevices();
    nlohmann::json result = nlohmann::json::array();
    
    for (const auto& device : devices) {
        nlohmann::json item;
        item["id"] = device.id;
        item["name"] = device.name;
        item["kind"] = SnowOwl::Config::toString(device.kind);
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
}

bool removeDevice(const std::string& dbPath, int deviceId) {
    if (deviceId <= 0) {
        std::cerr << "❌ Error: Device ID must be greater than zero" << std::endl;
        return false;
    }

    SnowOwl::Config::DeviceRegistry registry;
    if (!registry.open(dbPath)) {
        std::cerr << "❌ Failed to open device registry: " << dbPath << std::endl;
        return false;
    }

    auto device = registry.findById(deviceId);
    if (!device) {
        std::cerr << "❌ Error: Device with ID " << deviceId << " not found" << std::endl;
        return false;
    }

    if (registry.removeDevice(deviceId)) {
        std::cout << "✅ Successfully removed device:" << std::endl;
        std::cout << "  ID: " << device->id << std::endl;
        std::cout << "  Name: " << device->name << std::endl;
        std::cout << "  Kind: " << SnowOwl::Config::toString(device->kind) << std::endl;
        std::cout << "  URI: " << device->uri << std::endl;
        return true;
    } else {
        std::cerr << "❌ Failed to remove device with ID " << deviceId << std::endl;
        return false;
    }
}

bool setPrimaryDevice(const std::string& dbPath, int deviceId) {
    if (deviceId <= 0) {
        std::cerr << "❌ Error: Device ID must be greater than zero" << std::endl;
        return false;
    }

    SnowOwl::Config::DeviceRegistry registry;
    if (!registry.open(dbPath)) {
        std::cerr << "❌ Failed to open device registry: " << dbPath << std::endl;
        return false;
    }

    auto device = registry.findById(deviceId);
    if (!device) {
        std::cerr << "❌ Error: Device with ID " << deviceId << " not found" << std::endl;
        return false;
    }

    if (registry.setPrimaryDevice(deviceId)) {
        std::cout << "✅ Successfully set device as primary:" << std::endl;
        std::cout << "  ID: " << device->id << std::endl;
        std::cout << "  Name: " << device->name << std::endl;
        std::cout << "  Kind: " << SnowOwl::Config::toString(device->kind) << std::endl;
        std::cout << "  URI: " << device->uri << std::endl;
        return true;
    } else {
        std::cerr << "❌ Failed to set device with ID " << deviceId << " as primary" << std::endl;
        return false;
    }
}

}

namespace po = boost::program_options;

int EdgeManager::startEdge(const po::variables_map& vm) {
    namespace po = boost::program_options;
    using SnowOwl::Edge::Core::DeviceController;
    using SnowOwl::Config::DeviceRegistry;

    g_running = true;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif

    std::string dbPath = vm.count("db-path") ? vm["db-path"].as<std::string>() : "postgresql://snowowl_dev@localhost/snowowl_dev";

    {
        SnowOwl::Config::DeviceRegistry registry;
        if (registry.open(dbPath)) {
            std::cout << "✅ Connected to database: " << dbPath << std::endl;
        } else {
            std::cout << "⚠️  Warning: Unable to connect to database: " << dbPath << std::endl;
        }
    }

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
        
        std::cout << "===============================================================================\n";
        std::cout << "  🌊 Edge Device - Database Connection\n";
        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << "  🏠 Host:     " << host << std::endl;
        std::cout << "  🔌 Port:     " << port << std::endl;
        std::cout << "  🗃️ Database: " << dbName << std::endl;
        std::cout << "  👤 User:     " << user << std::endl;
        if (!password.empty()) {
            std::cout << "  🔐 Password: ***" << std::endl;
        } else {
            std::cout << "  🔐 Password: (not provided)" << std::endl;
        }
        std::cout << "-------------------------------------------------------------------------------\n";

        SnowOwl::Config::DeviceRegistry registry;
        if (registry.open(connectionString)) {
            std::cout << "\n✅ Database connection successful!" << std::endl;
            
            try {
                const auto devices = registry.listDevices();
                std::cout << "✅ Database schema is accessible (" << devices.size() << " devices found)" << std::endl;
                std::cout << "\nConnection string for future use:" << std::endl;
                std::cout << "  --db-path \"" << connectionString << "\"" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "\n⚠️  Connection successful but unable to query devices: " << e.what() << std::endl;
            }
        } else {
            std::cout << "\n❌ Database connection failed!" << std::endl;
            return 1;
        }
        
        return 0;
    }

    if (vm.count("list-devices")) {
        listDevices(dbPath);
        return 0;
    }

    if (vm.count("list-sources-json")) {
        listDevicesJson(dbPath);
        return 0;
    }

    if (vm.count("remove-device")) {
        int deviceId = vm["remove-device"].as<int>();
        return removeDevice(dbPath, deviceId) ? 0 : 1;
    }

    if (vm.count("set-primary")) {
        int deviceId = vm["set-primary"].as<int>();
        return setPrimaryDevice(dbPath, deviceId) ? 0 : 1;
    }

    std::unique_ptr<DeviceController> controller = std::make_unique<DeviceController>();

    const std::string configPath = resolveProfilePath(vm);
    if (configPath.empty()) {
        std::cerr << "❌ No configuration profile path could be resolved" << std::endl;
        return 1;
    }

    if (!controller->loadProfile(configPath)) {
        std::cerr << "❌ Failed to load edge device profile: " << configPath << std::endl;
        return 1;
    }

    const auto& profile = controller->profile();
    std::string registryPath = profile.registry.registryPath;
    if (!registryPath.empty()) {
        SnowOwl::Config::DeviceRegistry registry;
        if (registry.open(registryPath)) {
            std::cout << "✅ Auto-connected to database via configuration: " << registryPath << std::endl;
        } else {
            std::cout << "⚠️  Warning: Unable to connect to database via configuration" << std::endl;
        }
    }

    std::cout << "===============================================================================\n";
    std::cout << "  📋 Device Profile Information\n";
    std::cout << "-------------------------------------------------------------------------------\n";
    std::cout << "  🆔 ID: " << profile.deviceId << "\n";
    std::cout << "  📛 Name: " << profile.name << "\n";
    std::cout << "  ⚙️ Compute Tier: " << SnowOwl::Edge::Config::toString(profile.computeTier) << "\n";
    std::cout << "  💻 CPU cores: " << profile.cpuCores << "\n";
    std::cout << "  🧠 Memory (MB): " << profile.memoryMb << "\n";
    std::cout << "  🎮 GPU memory (MB): " << profile.gpuMemoryMb << "\n";
    std::cout << "  🖥️ Discrete GPU: " << (profile.hasDiscreteGpu ? "yes" : "no") << "\n";
    std::cout << "  🔢 Supports FP16: " << (profile.supportsFp16 ? "yes" : "no") << "\n";
    std::cout << "-------------------------------------------------------------------------------\n";

    if (controller->shouldRunLocalDetection()) {
        std::cout << "  🔍 On-device detection: enabled\n";
        std::cout << "  🧠 Preferred model: " << controller->recommendedModel() << "\n";
        std::cout << "  📦 Model format: " << profile.detectionPolicy.modelFormat << "\n";
        std::cout << "  📏 Max model size (MB): " << profile.detectionPolicy.maxModelSizeMB << "\n";
    } else {
        std::cout << "  ❌ On-device detection: disabled (forward-only)\n";
    }
    std::cout << "-------------------------------------------------------------------------------\n";

    if (controller->registerVideoSource()) {
        std::cout << "  🔄 Registry uplink: updated" << std::endl;
    }

    // this function blocks until termination signal
    // transports captured stream to target forwarder,default localhost:7500 via TCP
    std::thread captureThread([&controller]() {
        bool captureStarted = controller->startCapture();
        if (captureStarted) {
            std::cout << "  ▶️  Capture: running (mode="
                      << SnowOwl::Edge::Config::toString(controller->profile().capture.kind)
                      << ")" << std::endl;

            const auto& forwardCfg = controller->forwarderConfig();
            if (forwardCfg.enabled) {
                if (controller->forwarderRunning()) {
                    std::cout << "  📡 Forwarder: streaming to " << forwardCfg.host << ':' << forwardCfg.port << std::endl;
                } else {
                    std::cout << "  ❌ Forwarder: failed to start (check network target)" << std::endl;
                }
            } else {
                std::cout << "  🚫 Forwarder: disabled" << std::endl;
            }
            std::cout << "-------------------------------------------------------------------------------\n";
            std::cout << "  ⌨️  Press Ctrl+C to stop..." << std::endl;
            std::cout << "===============================================================================\n";

            while (g_running.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            controller->stopCapture();
            std::cout << "\n⏹️  Capture stopped." << std::endl;
        } else {
            std::cerr << "❌ Capture: failed to start (check source configuration)" << std::endl;
        }
    });
    
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (captureThread.joinable()) {
        captureThread.join();
    }

    return 0;
}

}
