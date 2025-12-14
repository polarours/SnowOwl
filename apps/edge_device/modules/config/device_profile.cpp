#include "modules/config/device_profile.hpp"

#include <algorithm>
#include <cctype>

namespace SnowOwl::Edge::Config {

namespace {

std::string normalize(const std::string& value) {
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

}

DeviceProfile DeviceProfile::makeDefault() {
    DeviceProfile profile;
    profile.deviceId = "edge-device";
    profile.name = "Generic Edge Device";
    profile.computeTier = ComputeTier::CaptureOnly;
    profile.hasDiscreteGpu = false;
    profile.supportsFp16 = false;
    profile.cpuCores = 28;
    profile.memoryMb = 164384;
    profile.gpuMemoryMb = 8096;
    profile.detectionPolicy.enableOnDevice = false;
    profile.detectionPolicy.preferredModel = "yolov8n";
    profile.detectionPolicy.preferredPrecision = "fp16";
    profile.detectionPolicy.modelFormat = "onnx";
    profile.detectionPolicy.maxModelSizeMB = 32.0;
    profile.detectionPolicy.maxLatencyMs = 200.0;
    profile.capture.kind = CaptureKind::Camera;
    profile.capture.cameraIndex = 0;
    profile.capture.primaryUri.clear();
    profile.capture.fallbackUri.clear();
    profile.registry.enable = false;
    profile.registry.registryPath.clear();
    profile.registry.deviceName = "Edge Capture";
    profile.registry.setPrimary = false;
    profile.registry.deviceKindOverride.clear();
    profile.registry.autoDetectCameras = false;
    profile.forward.enable = false;
    profile.forward.host = "127.0.0.1";
    profile.forward.port = 7500;
    profile.forward.frameIntervalMs = 33;
    profile.forward.reconnectDelayMs = 2000;
    return profile;
}

std::string toString(ComputeTier tier) {
    switch (tier) {
        case ComputeTier::CaptureOnly:
            return "capture_only";
        case ComputeTier::LightweightInference:
            return "lightweight_inference";
        case ComputeTier::FullInference:
            return "full_inference";
    }
    return "capture_only";
}

ComputeTier computeTierFromString(const std::string& value) {
    const std::string normalized = normalize(value);
    if (normalized == "lightweight" || normalized == "lightweight_inference") {
        return ComputeTier::LightweightInference;
    }
    if (normalized == "full" || normalized == "full_inference") {
        return ComputeTier::FullInference;
    }
    return ComputeTier::CaptureOnly;
}

std::string toString(CaptureKind kind) {
    switch (kind) {
        case CaptureKind::Camera:
            return "camera";
        case CaptureKind::RTSP:
            return "rtsp";
        case CaptureKind::RTMP:
            return "rtmp";
        case CaptureKind::File:
            return "file";
    }
    return "camera";
}

CaptureKind captureKindFromString(const std::string& value) {
    const std::string normalized = normalize(value);
    if (normalized == "rtmp") {
        return CaptureKind::RTMP;
    }
    if (normalized == "file" || normalized == "video") {
        return CaptureKind::File;
    }
    if (normalized == "rtsp" || normalized == "network" || normalized == "stream") {
        return CaptureKind::RTSP;
    }
    return CaptureKind::Camera;
}

}
