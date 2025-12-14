#include "modules/config/device_config.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace SnowOwl::Edge::Config {

namespace {

void applyDetectionPolicy(DeviceProfile& profile, const nlohmann::json& node) {
    if (!node.is_object()) {
        return;
    }

    profile.detectionPolicy.enableOnDevice = node.value("enable_on_device", profile.detectionPolicy.enableOnDevice);
    profile.detectionPolicy.preferredModel = node.value("preferred_model", profile.detectionPolicy.preferredModel);
    profile.detectionPolicy.preferredPrecision = node.value("preferred_precision", profile.detectionPolicy.preferredPrecision);
    profile.detectionPolicy.modelFormat = node.value("model_format", profile.detectionPolicy.modelFormat);
    profile.detectionPolicy.maxModelSizeMB = node.value("max_model_size_mb", profile.detectionPolicy.maxModelSizeMB);
    profile.detectionPolicy.maxLatencyMs = node.value("max_latency_ms", profile.detectionPolicy.maxLatencyMs);
}

void applyCaptureSettings(DeviceProfile& profile, const nlohmann::json& node) {
    if (!node.is_object()) {
        return;
    }

    const std::string kindString = node.value("kind", toString(profile.capture.kind));
    profile.capture.kind = captureKindFromString(kindString);
    profile.capture.cameraIndex = node.value("camera_index", profile.capture.cameraIndex);
    profile.capture.primaryUri = node.value("primary_uri", profile.capture.primaryUri);
    profile.capture.fallbackUri = node.value("fallback_uri", profile.capture.fallbackUri);
}

void applyRegistrySettings(DeviceProfile& profile, const nlohmann::json& node) {
    if (!node.is_object()) {
        return;
    }

    profile.registry.enable = node.value("enable", profile.registry.enable);
    profile.registry.registryPath = node.value("registry_path", profile.registry.registryPath);
    profile.registry.deviceName = node.value("device_name", profile.registry.deviceName);
    profile.registry.setPrimary = node.value("set_primary", profile.registry.setPrimary);
    profile.registry.deviceKindOverride = node.value("device_kind", profile.registry.deviceKindOverride);
    profile.registry.autoDetectCameras = node.value("auto_detect_cameras", profile.registry.autoDetectCameras);
}

void applyForwardSettings(DeviceProfile& profile, const nlohmann::json& node) {
    if (!node.is_object()) {
        return;
    }

    profile.forward.enable = node.value("enable", profile.forward.enable);
    profile.forward.host = node.value("host", profile.forward.host);
    profile.forward.port = node.value("port", profile.forward.port);
    profile.forward.frameIntervalMs = node.value("frame_interval_ms", profile.forward.frameIntervalMs);
    profile.forward.reconnectDelayMs = node.value("reconnect_delay_ms", profile.forward.reconnectDelayMs);
}

}

DeviceProfile DeviceConfig::loadFromFile(const std::string& path) {
    DeviceProfile profile = DeviceProfile::makeDefault();

    if (path.empty()) {
        return profile;
    }

    std::ifstream stream(path);
    if (!stream.is_open()) {

        return profile;
    }

    const auto resolveBaseDir = [&]() {
        std::filesystem::path configPath{path};
        std::error_code ec;
        const auto canonical = std::filesystem::weakly_canonical(configPath, ec);
        if (!ec) {
            return canonical.parent_path();
        }

        ec.clear();
        const auto absolutePath = std::filesystem::absolute(configPath, ec);
        if (!ec) {
            return absolutePath.parent_path();
        }

        return std::filesystem::current_path();
    };

    const auto baseDir = resolveBaseDir();

    try {
        const auto json = nlohmann::json::parse(stream, nullptr, true, true);

        profile.deviceId = json.value("device_id", profile.deviceId);
        profile.name = json.value("name", profile.name);
        profile.computeTier = computeTierFromString(json.value("compute_tier", toString(profile.computeTier)));
        profile.hasDiscreteGpu = json.value("has_discrete_gpu", profile.hasDiscreteGpu);
        profile.supportsFp16 = json.value("supports_fp16", profile.supportsFp16);
        profile.cpuCores = json.value("cpu_cores", profile.cpuCores);
        profile.memoryMb = json.value("memory_mb", profile.memoryMb);
        profile.gpuMemoryMb = json.value("gpu_memory_mb", profile.gpuMemoryMb);

        if (json.contains("detection")) {
            applyDetectionPolicy(profile, json.at("detection"));
        }

        if (json.contains("capture")) {
            applyCaptureSettings(profile, json.at("capture"));
        }

        if (json.contains("uplink")) {
            applyRegistrySettings(profile, json.at("uplink"));
        }

        if (json.contains("forward")) {
            applyForwardSettings(profile, json.at("forward"));
        }

        if (!profile.shouldRunOnDeviceDetection()) {
            profile.detectionPolicy.enableOnDevice = false;
        }

        if (profile.registry.enable) {
            std::filesystem::path registryPath = profile.registry.registryPath;
            if (registryPath.empty()) {
                registryPath = baseDir / "devices.db";
            } else if (registryPath.is_relative()) {
                if (profile.registry.registryPath.find("://") == std::string::npos) {
                    registryPath = (baseDir / registryPath).lexically_normal();
                }
            }
            profile.registry.registryPath = registryPath.string();
        }

    } catch (const std::exception& ex) {
        // ...
    }

    return profile;
}

}
