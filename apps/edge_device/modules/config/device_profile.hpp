#pragma once

#include <cstdint>
#include <string>

namespace SnowOwl::Edge::Config {

enum class ComputeTier : std::uint8_t {
    CaptureOnly = 0,
    LightweightInference = 1,
    FullInference = 2
};

enum class CaptureKind : std::uint8_t {
    Camera = 0,
    RTSP = 1,
    RTMP = 2,
    File = 3,
    Unkonwn = 4
};

struct DetectionPolicy {
    bool enableOnDevice{false};
    std::string preferredModel{"yolov8n"}; // just an example model name
    std::string preferredPrecision{"fp16"};
    std::string modelFormat{"onnx"};
    double maxModelSizeMB{32.0};
    double maxLatencyMs{200.0};
};

struct DeviceProfile {
    std::string deviceId{"Default"};
    std::string name{"Default Device"};
    ComputeTier computeTier{ComputeTier::CaptureOnly};
    bool hasDiscreteGpu{false};
    bool supportsFp16{false};
    std::uint32_t cpuCores{28};
    std::uint32_t memoryMb{164384};
    std::uint32_t gpuMemoryMb{8096};
    DetectionPolicy detectionPolicy{};

    struct CaptureSettings {
        CaptureKind kind{CaptureKind::Camera};
        int cameraIndex{0};
        std::string primaryUri;
        std::string fallbackUri;
    } capture{};

    struct RegistryUplink {
        bool enable{false};
        std::string registryPath;
        std::string deviceName{"Edge Capture"};
        bool setPrimary{false};
        std::string deviceKindOverride;
        bool autoDetectCameras{false};
        bool autoDetectAudioDevices{false};
    } registry{};

    struct ForwardSettings {
        bool enable{false};
        std::string host{"127.0.0.1"};
        std::uint16_t port{7500};
        std::uint32_t frameIntervalMs{33};
        std::uint32_t reconnectDelayMs{2000};
    } forward{};

    bool shouldRunOnDeviceDetection() const {
        return detectionPolicy.enableOnDevice && computeTier != ComputeTier::CaptureOnly;
    }

    static DeviceProfile makeDefault();
};

std::string toString(ComputeTier tier);
ComputeTier computeTierFromString(const std::string& value);
std::string toString(CaptureKind kind);
CaptureKind captureKindFromString(const std::string& value);

}
