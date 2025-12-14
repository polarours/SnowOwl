#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

namespace SnowOwl::Edge::Core {

struct AudioConfig {
    int sampleRate = 48000;
    int channels = 2;
    std::string codec = "opus";
    int bitrate = 128000;
    bool enableNoiseReduction = false;
    bool enableEchoCancellation = false;
    bool enableEncryption = false;
    std::string encryptionKey;
};

enum class SoundEventType {
    GlassBreak,
    Shout,
    Scream,
    Gunshot,
    Custom
};

struct SoundEvent {
    SoundEventType type;
    double timestamp;
    double confidence;
    std::string description;
};

using SoundEventCallback = std::function<void(const SoundEvent&)>;

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();

    bool initialize();
    void cleanup();

    bool startCapture(const AudioConfig& config = {});
    void stopCapture();

    bool startPlayback(const AudioConfig& config = {});
    void stopPlayback();

    void setPlaybackVolume(double volume);
    double getPlaybackVolume() const;

    bool isCapturing() const;
    bool isPlaying() const;

    bool sendAudioData(const void* data, size_t size);
    
    std::vector<std::string> enumerateAudioDevices() const;
    void setNoiseReductionLevel(float level);
    void setEchoCancellation(bool enabled);
    bool startIntercom(const std::string& targetDevice);
    void stopIntercom();

    void setSoundEventCallback(SoundEventCallback callback);
    void enableSoundEventDetection(bool enable);
    void setAudioTriggeredRecording(bool enable);
    bool isSoundEventDetectionEnabled() const;
    bool isAudioTriggeredRecordingEnabled() const;
    
    void setLowPowerMode(bool enable);
    bool isLowPowerModeEnabled() const;
    void enableEncryption(bool enable);
    bool isEncryptionEnabled() const;
    void setEncryptionKey(const std::string& key);

    void setPrivacyMode(bool enable);
    bool isPrivacyModeEnabled() const;

private:
    void captureLoop();
    void playbackLoop();
    void analyzeAudioBuffer(GstBuffer* buffer);
    std::string buildCapturePipelineString(const AudioConfig& config) const;
    std::string buildPlaybackPipelineString(const AudioConfig& config) const;
    bool detectSoundEvent(const std::vector<float>& audioData);

    std::string buildSecureCapturePipelineString(const AudioConfig& config) const;
    std::string buildSecurePlaybackPipelineString(const AudioConfig& config) const;

    GstElement* capturePipeline_ = nullptr;
    GstElement* captureSink_ = nullptr;
    GstBus* captureBus_ = nullptr;

    GstElement* playbackPipeline_ = nullptr;
    GstElement* playbackSrc_ = nullptr;
    GstBus* playbackBus_ = nullptr;

    std::thread captureThread_;
    std::thread playbackThread_;

    mutable std::mutex captureMutex_;
    mutable std::mutex playbackMutex_;

    std::atomic<bool> capturing_{false};
    std::atomic<bool> playing_{false};
    std::atomic<bool> shouldRunCapture_{false};
    std::atomic<bool> shouldRunPlayback_{false};

    double volume_ = 1.0;
    float noiseReductionLevel_ = 0.0f;
    bool echoCancellationEnabled_ = false;

    std::atomic<bool> soundEventDetectionEnabled_{false};
    std::atomic<bool> audioTriggeredRecordingEnabled_{false};
    SoundEventCallback soundEventCallback_;
    mutable std::mutex callbackMutex_;

    std::atomic<bool> lowPowerModeEnabled_{false};
    std::atomic<bool> encryptionEnabled_{false};
    std::string encryptionKey_;

    std::atomic<bool> privacyModeEnabled_{false};
    
    AudioConfig currentCaptureConfig_;
    AudioConfig currentPlaybackConfig_;
};

}
