#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "core/stream_capture.hpp"
#include "core/stream_forwarder.hpp"
#include "core/audio_processor.hpp"
#include "modules/config/device_profile.hpp"
#include "utils/health_monitor.hpp"
#include "utils/resource_tracker.hpp"
#include "utils/system_probe.hpp"
#include "modules/utils/encoder_selector.hpp"
#include "modules/utils/power_manager.hpp"
#include "config/device_registry.hpp"

namespace SnowOwl::Edge::Core {

using SnowOwl::Utils::SystemResources::ResourceSnapshot;
using SnowOwl::Utils::SystemResources::HealthStatus;
using SnowOwl::Utils::SystemResources::SystemInfo;
using SnowOwl::Utils::SystemResources::ResourceTracker;
using SnowOwl::Utils::SystemResources::HealthMonitor;
using SnowOwl::Config::DeviceRecord;
using SnowOwl::Config::DeviceKind;

class DeviceController {
public:
    DeviceController();
    ~DeviceController();

    bool loadProfile(const std::string& path);
    void setProfile(const Config::DeviceProfile& profile);

    const Config::DeviceProfile& profile() const { return profile_; }
    const SystemInfo& systemInfo() const { return systemInfo_; }

    bool shouldRunLocalDetection() const; 
    std::string recommendedModel() const;

    bool registerVideoSource();
    bool startCapture();
    void stopCapture();

    bool startAudioCapture(const AudioConfig& config = {});
    void stopAudioCapture();
    bool startAudioPlayback(const AudioConfig& config = {});
    void stopAudioPlayback();
    bool isAudioCaptureRunning() const;
    bool isAudioPlaybackRunning() const;

    std::vector<std::string> enumerateAudioDevices() const;
    void setAudioNoiseReductionLevel(float level);
    void setAudioEchoCancellation(bool enabled);
    bool startIntercom(const std::string& targetDevice);
    void stopIntercom();
    
    void setSoundEventCallback(SoundEventCallback callback);
    void enableSoundEventDetection(bool enable);
    void setAudioTriggeredRecording(bool enable);
    bool isSoundEventDetectionEnabled() const;
    bool isAudioTriggeredRecordingEnabled() const;
    
    void setAudioLowPowerMode(bool enable);
    bool isAudioLowPowerModeEnabled() const;
    void enableAudioEncryption(bool enable);
    bool isAudioEncryptionEnabled() const;
    void setAudioEncryptionKey(const std::string& key);
    
    void setAudioPrivacyMode(bool enable);
    bool isAudioPrivacyModeEnabled() const;
    void setAudioSchedule(const std::string& schedule);
    std::string getAudioSchedule() const;
    
    bool isDatabaseConnected() const;
    void setApiEndpoint(const std::string& apiEndpoint);
    bool registerVideoSourceViaApi() const;

    ResourceSnapshot latestResourceSnapshot() const;
    HealthStatus healthStatus() const;
    const Utils::EncoderChoice& encoderChoice() const { return encoderChoice_; }
    const Utils::PowerPolicy& powerPolicy() const { return powerPolicy_; }

    SnowOwl::Edge::Core::CaptureSourceConfig captureConfig() const { return captureConfig_; }
    bool captureRunning() const { return capture_.isRunning(); }
    bool forwarderRunning() const { return forwarder_->isRunning(); }
    const ForwarderConfig& forwarderConfig() const { return forwarderConfig_; }

private:
    CaptureSourceConfig buildCaptureConfig() const;
    ForwarderConfig buildForwarderConfig() const;
    void applyProfile();
    void refreshOperationalState();
    std::vector<int> enumerateCameras() const;
    void autoDetectAndRegisterCameras();
    bool tryRegisterViaDatabase();
    bool tryRegisterViaApi() const;

    Config::DeviceProfile profile_ {};
    CaptureSourceConfig captureConfig_ {};
    ForwarderConfig forwarderConfig_ {};
    StreamCapture capture_ {};
    std::shared_ptr<StreamForwarder> forwarder_ {};
    AudioProcessor audioProcessor_ {};

    ResourceTracker resourceTracker_ {};
    HealthMonitor healthMonitor_ {};
    mutable std::mutex healthMutex_ {};
    HealthStatus healthStatus_ {};
    SystemInfo systemInfo_ {};

    Utils::EncoderSelector encoderSelector_ {};
    Utils::EncoderChoice encoderChoice_ {};
    Utils::PowerPolicy powerPolicy_ {};
    Utils::PowerManager powerManager_ {};
    
    std::atomic<bool> audioPrivacyMode_ {false};
    std::string audioSchedule_ {};
    mutable std::mutex scheduleMutex_ {};
    
    std::atomic<bool> databaseConnected_ {false};
    std::string apiEndpoint_ {};
    mutable std::mutex apiMutex_ {};
};

}