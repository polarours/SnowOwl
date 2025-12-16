#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <system_error>
#include <utility>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <nlohmann/json.hpp>
#include <opencv4/opencv2/opencv.hpp>

#include "device_controller.hpp"
#include "libs/database/device_registry.hpp"
#include "libs/utils/app_paths.hpp"
#include "modules/config/device_config.hpp"

namespace {

std::string resolveHostName() {
#if defined(_WIN32)
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameA(buffer, &size) != 0 && size > 0) {
        return std::string(buffer, size);
    }
    return "windows";
#elif defined(__APPLE__)
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer) - 1) == 0) {
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
    }
    return "macos";
#elif defined(__linux__)
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer) - 1) == 0) {
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
    }
    return "linux";
#endif
}

}

namespace SnowOwl::Edge::Core {

using SnowOwl::Utils::Monitoring::ResourceSnapshot;
using SnowOwl::Utils::Monitoring::SystemProbe;
using SnowOwl::Utils::Monitoring::HealthStatus;  
using SnowOwl::Utils::Monitoring::HealthThresholds; 
    
DeviceController::DeviceController()
    : profile_(Config::DeviceProfile::makeDefault()) 
    , forwarder_(std::make_shared<StreamForwarder>())
{
    resourceTracker_.start(std::chrono::milliseconds(1000));
    applyProfile();
}

DeviceController::~DeviceController() {
    stopCapture();
    resourceTracker_.stop();
}

bool DeviceController::loadProfile(const std::string& path) {
    profile_ = Config::DeviceConfig::loadFromFile(path);
    applyProfile();
    return true;
}

void DeviceController::setProfile(const Config::DeviceProfile& profile) {
    profile_ = profile;
    applyProfile();
}

bool DeviceController::shouldRunLocalDetection() const {
    return profile_.shouldRunOnDeviceDetection();
}

std::string DeviceController::recommendedModel() const {
    if (!shouldRunLocalDetection()) {
        return "none";
    }
    return profile_.detectionPolicy.preferredModel;
}

CaptureSourceConfig DeviceController::buildCaptureConfig() const {
    CaptureSourceConfig config;

    using Config::CaptureKind;

    switch (profile_.capture.kind) {
        case CaptureKind::Camera:
            config.mode = CaptureMode::Camera;
            config.cameraIndex = profile_.capture.cameraIndex;
            break;
        case CaptureKind::RTSP:
            config.mode = CaptureMode::Network;
            config.primaryUri = profile_.capture.primaryUri;
            config.fallbackUri = profile_.capture.fallbackUri;
            break;
        case CaptureKind::RTMP:
            config.mode = CaptureMode::Network;
            config.primaryUri = profile_.capture.primaryUri;
            config.fallbackUri = profile_.capture.fallbackUri;
            break;
        case CaptureKind::File:
            config.mode = CaptureMode::File;
            config.primaryUri = profile_.capture.primaryUri;
            config.fallbackUri = profile_.capture.fallbackUri;
            break;
    }

    if (config.mode == CaptureMode::Camera && config.cameraIndex < 0) {
        config.cameraIndex = 0;
    }

    return config;
}

ForwarderConfig DeviceController::buildForwarderConfig() const {
    ForwarderConfig config;
    const auto& forward = profile_.forward;

    config.enabled = forward.enable;
    config.host = forward.host;
    config.port = forward.port;
    config.frameInterval = std::chrono::milliseconds(forward.frameIntervalMs);
    config.reconnectDelay = std::chrono::milliseconds(forward.reconnectDelayMs);
    config.deviceId = profile_.deviceId;
    config.deviceName = profile_.name;

    if (config.frameInterval.count() <= 0) {
        config.frameInterval = std::chrono::milliseconds(100);
    }
    if (config.reconnectDelay.count() <= 0) {
        config.reconnectDelay = std::chrono::milliseconds(2000);
    }

    return config;
}

void DeviceController::applyProfile() {
    systemInfo_ = SystemProbe::collect();

    if (systemInfo_.physicalCores > 0 && systemInfo_.physicalCores != profile_.cpuCores) {
        profile_.cpuCores = systemInfo_.physicalCores;
    }

    if (systemInfo_.memoryTotalMb > 0 && systemInfo_.memoryTotalMb != profile_.memoryMb) {
        const auto clamped = static_cast<std::uint32_t>(std::min<std::uint64_t>(
            systemInfo_.memoryTotalMb, std::numeric_limits<std::uint32_t>::max()));
        profile_.memoryMb = clamped;
    }

    const bool detectedDiscrete = systemInfo_.hasNvidiaGpu || systemInfo_.hasAmdGpu;
    profile_.hasDiscreteGpu = detectedDiscrete;
    profile_.supportsFp16 = profile_.supportsFp16 || detectedDiscrete || systemInfo_.hasIntelGpu;

    captureConfig_ = buildCaptureConfig();
    capture_.configure(captureConfig_);

    if (forwarder_->isRunning()) {
        forwarder_->stop();
    }

    forwarderConfig_ = buildForwarderConfig();
    forwarder_->configure(forwarderConfig_);
    HealthThresholds thresholds;
    if (profile_.computeTier == Config::ComputeTier::FullInference) {
        thresholds.maxCpuPercent = 95.0;
        thresholds.maxMemoryPercent = 95.0;
    } else if (profile_.computeTier == Config::ComputeTier::CaptureOnly) {
        thresholds.maxCpuPercent = 75.0;
        thresholds.maxMemoryPercent = 75.0;
    }
    healthMonitor_.setThresholds(thresholds);

    encoderChoice_ = encoderSelector_.select(profile_);
    powerPolicy_ = Utils::PowerPolicy::fromProfile(profile_);
    powerManager_.applyPolicy(powerPolicy_);

    if (profile_.registry.autoDetectCameras) {
        autoDetectAndRegisterCameras();
    }

    refreshOperationalState();
}

bool DeviceController::registerVideoSource() {
    const auto& uplink = profile_.registry;
    if (!uplink.enable) {
        return false;
    }

    if (tryRegisterViaDatabase()) {
        return true;
    }

    if (tryRegisterViaApi()) {
        return true;
    }


    return false;
}

bool DeviceController::startCapture() {
    const bool started = capture_.start();
    if (!started) {
        return false;
    }

    if (forwarderConfig_.enabled) {
        if (!forwarder_->start(&capture_)) {

        }
    }

    refreshOperationalState();
    return true;
}

void DeviceController::stopCapture() {
    if (forwarder_->isRunning()) {
        forwarder_->stop();
    }
    capture_.stop();

    refreshOperationalState();
}

ResourceSnapshot DeviceController::latestResourceSnapshot() const {
    return resourceTracker_.latestSnapshot();
}

HealthStatus DeviceController::healthStatus() const {
    std::lock_guard<std::mutex> lock(healthMutex_);
    return healthStatus_;
}

void DeviceController::refreshOperationalState() {
    const auto snapshot = resourceTracker_.sampleNow();
    const auto status = healthMonitor_.evaluate(snapshot);

    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        healthStatus_ = status;
    }

    powerManager_.onHealthUpdate(status);
}

std::vector<int> DeviceController::enumerateCameras() const {
    std::vector<int> availableCameras;
    const int maxCamerasToCheck = 10;
    int consecutiveFailures = 0;

    for (int i = 0; i < maxCamerasToCheck; ++i) {
#if defined(__linux__)
        cv::VideoCapture cap(i);
        if (!cap.isOpened()) {
            cap.open(i);
        }
#else
        cv::VideoCapture cap(i);
#endif

        if (cap.isOpened()) {
            availableCameras.push_back(i);
            consecutiveFailures = 0;
            cap.release();
        } else {
            ++consecutiveFailures;
            const int failureThreshold = availableCameras.empty() ? 3 : 1;
            if (consecutiveFailures >= failureThreshold) {
                break;
            }
        }
    }

    return availableCameras;
}

void DeviceController::autoDetectAndRegisterCameras()
{
    if (!profile_.registry.enable) {
        return;
    }

    std::string registryPath = profile_.registry.registryPath;
    if (registryPath.empty()) {
        registryPath = "postgresql://localhost/snowowl_dev";
    }

    if (registryPath.empty()) {
        return;
    }

    SnowOwl::Config::DeviceRegistry registry;
    if (!registry.open(registryPath)) {

        return;
    }

    const auto availableCameras = enumerateCameras();
    if (availableCameras.empty()) {

        return;
    }



    for (int cameraIndex : availableCameras) {
        SnowOwl::Config::DeviceRecord record;
        record.name = "Auto Detected Camera: " + std::to_string(cameraIndex);
        record.kind = SnowOwl::Config::DeviceKind::Camera;
        record.enabled = true;
        record.isPrimary = false;
        record.uri = "camera://" + std::to_string(cameraIndex);

        nlohmann::json metadata = nlohmann::json::object();
        metadata["origin"] = "edge";
        metadata["edge_device"] = {
            {"id", profile_.deviceId},
            {"name", profile_.name},
            {"host", resolveHostName()},
            {"camera_index", cameraIndex},
            {"compute_tier", Config::toString(profile_.computeTier)},
            {"supports_fp16", profile_.supportsFp16},
            {"has_discrete_gpu", profile_.hasDiscreteGpu}
        };
        metadata["camera_id"] = cameraIndex;
        metadata["auto_detected"] = true;
        record.metadata = metadata.dump();

        if (auto existing = registry.findByUri(record.uri)) {
            record.id = existing->id;
        }

        auto stored = registry.upsertDevice(record);
        if (stored.id > 0) {

        } else {

        }
    }
}

bool DeviceController::startAudioCapture(const AudioConfig& config)
{

    if (audioPrivacyMode_.load()) {
        return false;
    }
    
    return audioProcessor_.startCapture(config);
}

void DeviceController::stopAudioCapture()
{
    audioProcessor_.stopCapture();
}

bool DeviceController::startAudioPlayback(const AudioConfig& config)
{
    return audioProcessor_.startPlayback(config);
}

void DeviceController::stopAudioPlayback()
{
    audioProcessor_.stopPlayback();
}

bool DeviceController::isAudioCaptureRunning() const
{
    return audioProcessor_.isCapturing();
}

bool DeviceController::isAudioPlaybackRunning() const
{
    return audioProcessor_.isPlaying();
}

std::vector<std::string> DeviceController::enumerateAudioDevices() const
{
    return audioProcessor_.enumerateAudioDevices();
}

void DeviceController::setAudioNoiseReductionLevel(float level)
{
    audioProcessor_.setNoiseReductionLevel(level);
}

void DeviceController::setAudioEchoCancellation(bool enabled)
{
    audioProcessor_.setEchoCancellation(enabled);
}

bool DeviceController::startIntercom(const std::string& targetDevice)
{
    return audioProcessor_.startIntercom(targetDevice);
}

void DeviceController::stopIntercom()
{
    audioProcessor_.stopIntercom();
}

void DeviceController::setSoundEventCallback(SoundEventCallback callback)
{
    audioProcessor_.setSoundEventCallback(callback);
}

void DeviceController::enableSoundEventDetection(bool enable)
{
    audioProcessor_.enableSoundEventDetection(enable);
}

void DeviceController::setAudioTriggeredRecording(bool enable)
{
    audioProcessor_.setAudioTriggeredRecording(enable);
}

bool DeviceController::isSoundEventDetectionEnabled() const
{
    return audioProcessor_.isSoundEventDetectionEnabled();
}

bool DeviceController::isAudioTriggeredRecordingEnabled() const
{
    return audioProcessor_.isAudioTriggeredRecordingEnabled();
}

void DeviceController::setAudioLowPowerMode(bool enable)
{
    audioProcessor_.setLowPowerMode(enable);
}

bool DeviceController::isAudioLowPowerModeEnabled() const
{
    return audioProcessor_.isLowPowerModeEnabled();
}

void DeviceController::enableAudioEncryption(bool enable)
{
    audioProcessor_.enableEncryption(enable);
}

bool DeviceController::isAudioEncryptionEnabled() const
{
    return audioProcessor_.isEncryptionEnabled();
}

void DeviceController::setAudioEncryptionKey(const std::string& key)
{
    audioProcessor_.setEncryptionKey(key);
}

void DeviceController::setAudioPrivacyMode(bool enable)
{
    audioPrivacyMode_.store(enable);
    
    if (enable && audioProcessor_.isCapturing()) {
        audioProcessor_.stopCapture();
    }
    
    audioProcessor_.setPrivacyMode(enable);
}

bool DeviceController::isAudioPrivacyModeEnabled() const
{
    return audioPrivacyMode_.load();
}

void DeviceController::setAudioSchedule(const std::string& schedule)
{
    std::lock_guard<std::mutex> lock(scheduleMutex_);
    audioSchedule_ = schedule;
}

std::string DeviceController::getAudioSchedule() const
{
    std::lock_guard<std::mutex> lock(scheduleMutex_);
    return audioSchedule_;
}

bool DeviceController::isDatabaseConnected() const
{
    return databaseConnected_.load();
}

void DeviceController::setApiEndpoint(const std::string& apiEndpoint)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    apiEndpoint_ = apiEndpoint;
}

bool DeviceController::registerVideoSourceViaApi() const
{

    return false;
}

bool DeviceController::tryRegisterViaDatabase()
{
    const auto& uplink = profile_.registry;
    std::string registryPath = uplink.registryPath;
    
    if (registryPath.empty()) {
        registryPath = "postgresql://snowowl_dev:SnowOwl_Dev!@localhost/snowowl_dev";
    }

    if (registryPath.empty()) {
        return false;
    }

    SnowOwl::Config::DeviceRegistry registry;
    if (!registry.open(registryPath)) {

        databaseConnected_ = false;
        return false;
    }
    
    databaseConnected_ = true;
    using SnowOwl::Config::DeviceKind;
    using SnowOwl::Config::deviceKindFromString;

    auto resolveKind = [&]() {
        if (!uplink.deviceKindOverride.empty()) {
            const auto overrideKind = deviceKindFromString(uplink.deviceKindOverride);
            if (overrideKind != DeviceKind::Unknown) {
                return overrideKind;
            }

        }

        switch (profile_.capture.kind) {
            case Config::CaptureKind::Camera:
                return DeviceKind::Camera;
            case Config::CaptureKind::RTSP:
                return DeviceKind::RTSP;
            case Config::CaptureKind::RTMP:
                return DeviceKind::RTMP;
            case Config::CaptureKind::File:
                return DeviceKind::File;
        }
        return DeviceKind::Unknown;
    };

    DeviceKind deviceKind = resolveKind();
    if (deviceKind == DeviceKind::Unknown) {

        deviceKind = DeviceKind::Camera;
    }

    SnowOwl::Config::DeviceRecord record;
    record.name = uplink.deviceName.empty() ? profile_.name : uplink.deviceName;
    record.kind = deviceKind;
    record.enabled = true;
    record.isPrimary = uplink.setPrimary;

    nlohmann::json metadata = nlohmann::json::object();

    if (deviceKind == DeviceKind::Camera) {
        const int cameraIndex = (captureConfig_.mode == CaptureMode::Camera)
                                    ? captureConfig_.cameraIndex
                                    : profile_.capture.cameraIndex;
        const int normalizedIndex = cameraIndex < 0 ? 0 : cameraIndex;
        record.uri = "camera://" + std::to_string(normalizedIndex);
        metadata["camera_id"] = normalizedIndex;
    } else {
        const std::string primary = !captureConfig_.primaryUri.empty()
                                        ? captureConfig_.primaryUri
                                        : profile_.capture.primaryUri;
        const std::string fallback = !captureConfig_.fallbackUri.empty()
                                         ? captureConfig_.fallbackUri
                                         : profile_.capture.fallbackUri;

        record.uri = primary.empty() ? fallback : primary;

        if (!primary.empty()) {
            metadata["primary_uri"] = primary;
            metadata["stream_uri"] = primary;
        }
        if (!fallback.empty()) {
            metadata["fallback_uri"] = fallback;
            metadata["secondary_uri"] = fallback;
        }

        if (deviceKind == DeviceKind::RTSP) {
            metadata["rtsp_uri"] = record.uri;
        } else if (deviceKind == DeviceKind::RTMP) {
            metadata["rtmp_uri"] = record.uri;
        } else if (deviceKind == DeviceKind::File) {
            metadata["file_path"] = record.uri;
        }
    }

    if (record.uri.empty()) {

        return false;
    }

    if (auto existing = registry.findByUri(record.uri)) {
        record.id = existing->id;
    }

    metadata["origin"] = "edge";
    metadata["edge_device"] = {
        {"id", profile_.deviceId},
        {"name", profile_.name},
        {"host", resolveHostName()},
        {"compute_tier", Config::toString(profile_.computeTier)},
        {"supports_fp16", profile_.supportsFp16},
        {"has_discrete_gpu", profile_.hasDiscreteGpu},
        {"capture_kind", Config::toString(profile_.capture.kind)}
    };

    if (captureConfig_.mode == CaptureMode::Camera) {
        metadata["edge_device"]["camera_index"] = (captureConfig_.cameraIndex < 0) ? 0 : captureConfig_.cameraIndex;
    }

    if (forwarderConfig_.enabled) {
        metadata["edge_device"]["forward_enabled"] = true;
        metadata["edge_device"]["forward_host"] = forwarderConfig_.host;
        metadata["edge_device"]["forward_port"] = forwarderConfig_.port;
    } else {
        metadata["edge_device"]["forward_enabled"] = false;
    }

    const auto snapshot = latestResourceSnapshot();
    if (snapshot.valid) {
        nlohmann::json metrics = nlohmann::json::object();
        metrics["cpu_percent"] = snapshot.cpuPercent;
        metrics["memory_percent"] = snapshot.memoryPercent;
        metrics["memory_used_mb"] = snapshot.memoryUsedMb;
        metrics["memory_total_mb"] = snapshot.memoryTotalMb;
        if (!std::isnan(snapshot.temperatureC)) {
            metrics["temperature_c"] = snapshot.temperatureC;
        }
        if (snapshot.gpuPercent >= 0.0) {
            metrics["gpu_percent"] = snapshot.gpuPercent;
        }
        metadata["metrics"] = std::move(metrics);
    }

    metadata["encoder"] = {
        {"kind", Utils::EncoderSelector::toString(encoderChoice_.kind)},
        {"name", encoderChoice_.name},
        {"supports_fp16", encoderChoice_.supportsFp16}
    };

    metadata["power_policy"] = {
        {"mode", Utils::PowerPolicy::toString(powerPolicy_.mode)},
        {"gpu_boost", powerPolicy_.allowGpuBoost},
        {"prefer_low_power_encoders", powerPolicy_.preferLowPowerEncoders}
    };

    metadata["system"] = {
        {"architecture", systemInfo_.architecture},
        {"kernel", systemInfo_.kernel},
        {"cpu_model", systemInfo_.cpuModel},
        {"cpu_vendor", systemInfo_.cpuVendor},
        {"logical_cores", systemInfo_.logicalCores},
        {"physical_cores", systemInfo_.physicalCores},
        {"memory_total_mb", systemInfo_.memoryTotalMb},
        {"has_nvidia_gpu", systemInfo_.hasNvidiaGpu},
        {"has_amd_gpu", systemInfo_.hasAmdGpu},
        {"has_intel_gpu", systemInfo_.hasIntelGpu}
    };

    if (!metadata.empty()) {
        record.metadata = metadata.dump();
    }

    const auto stored = registry.upsertDevice(record);
    if (stored.id <= 0) {

        return false;
    }

    if (uplink.setPrimary) {
        registry.setPrimaryDevice(stored.id);
    }

    refreshOperationalState();
    return true;
}

bool DeviceController::tryRegisterViaApi() const
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    
    if (apiEndpoint_.empty()) {
        return false;
    }

    return true;
}

}
