#include "core/audio_processor.hpp"

#include <iostream>
#include <thread>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace SnowOwl::Edge::Core {

AudioProcessor::AudioProcessor() = default;

AudioProcessor::~AudioProcessor() {
    stopCapture();
    stopPlayback();
    cleanup();
}

bool AudioProcessor::initialize() {
    return true;
}

void AudioProcessor::cleanup() {
    stopCapture();
    stopPlayback();
}

std::string AudioProcessor::buildCapturePipelineString(const AudioConfig& config) const {
    std::ostringstream pipeline;
    
    pipeline << "autoaudiosrc";
    
    if (config.enableEchoCancellation) {
        pipeline << " ! webrtcechoprocessor";
    }
    
    if (config.enableNoiseReduction) {
        pipeline << " ! webrtcdenoise";
    }
    
    pipeline << " ! audioconvert ! audioresample";
    
    if (config.codec == "opus") {
        pipeline << " ! opusenc bitrate=" << config.bitrate;
        pipeline << " ! rtpopuspay";
    } else {
        pipeline << " ! vorbisenc";
        pipeline << " ! rtpvorbispay";
    }
    
    pipeline << " ! appsink name=audio_sink";
    
    return pipeline.str();
}

std::string AudioProcessor::buildSecureCapturePipelineString(const AudioConfig& config) const {
    std::ostringstream pipeline;
    
    pipeline << "autoaudiosrc";
    
    if (config.enableEchoCancellation) {
        pipeline << " ! webrtcechoprocessor";
    }
    
    if (config.enableNoiseReduction) {
        pipeline << " ! webrtcdenoise";
    }
    
    pipeline << " ! audioconvert ! audioresample";
    
    if (config.codec == "opus") {
        pipeline << " ! opusenc bitrate=" << config.bitrate;
        if (config.enableEncryption && !config.encryptionKey.empty()) {
            pipeline << " ! srtpenc key=" << config.encryptionKey;
        }
        pipeline << " ! rtpopuspay";
    } else {
        pipeline << " ! vorbisenc";
        if (config.enableEncryption && !config.encryptionKey.empty()) {
            pipeline << " ! srtpenc key=" << config.encryptionKey;
        }
        pipeline << " ! rtpvorbispay";
    }
    
    pipeline << " ! appsink name=audio_sink";
    
    return pipeline.str();
}

std::string AudioProcessor::buildPlaybackPipelineString(const AudioConfig& config) const {
    std::ostringstream pipeline;
    
    pipeline << "appsrc name=audio_src ! application/x-rtp";
    
    if (config.codec == "opus") {
        pipeline << " ! rtpopusdepay ! opusdec";
    } else {
        pipeline << " ! rtpvorbisdepay ! vorbisdec";
    }
    
    pipeline << " ! audioconvert";
    
    if (config.enableEchoCancellation) {
        pipeline << " ! webrtcechoprocessor";
    }
    
    pipeline << " ! autoaudiosink";
    
    return pipeline.str();
}

std::string AudioProcessor::buildSecurePlaybackPipelineString(const AudioConfig& config) const {
    std::ostringstream pipeline;
    
    pipeline << "appsrc name=audio_src ! application/x-rtp";
    
    if (config.enableEncryption && !config.encryptionKey.empty()) {
        pipeline << " ! srtpdec key=" << config.encryptionKey;
    }
    
    if (config.codec == "opus") {
        pipeline << " ! rtpopusdepay ! opusdec";
    } else {
        pipeline << " ! rtpvorbisdepay ! vorbisdec";
    }
    
    pipeline << " ! audioconvert";
    
    if (config.enableEchoCancellation) {
        pipeline << " ! webrtcechoprocessor";
    }
    
    pipeline << " ! autoaudiosink";
    
    return pipeline.str();
}

bool AudioProcessor::startCapture(const AudioConfig& config) {
    std::lock_guard<std::mutex> lock(captureMutex_);
    if (capturing_.load()) {
        return true;
    }

    currentCaptureConfig_ = config;
    
    GError* error = nullptr;
    std::string pipelineStr;
    
    if (config.enableEncryption && !config.encryptionKey.empty()) {
        pipelineStr = buildSecureCapturePipelineString(config);
    } else {
        pipelineStr = buildCapturePipelineString(config);
    }
    
    capturePipeline_ = gst_parse_launch(pipelineStr.c_str(), &error);

    if (!capturePipeline_) {
        std::cerr << "AudioProcessor: failed to create capture pipeline: " 
                  << (error ? error->message : "unknown error") << std::endl;
        if (error) {
            g_error_free(error);
        }
        return false;
    }

    GstElement* sink = gst_bin_get_by_name(GST_BIN(capturePipeline_), "audio_sink");
    if (!sink) {
        std::cerr << "AudioProcessor: failed to get sink from capture pipeline" << std::endl;
        cleanup();
        return false;
    }
    captureSink_ = sink;

    captureBus_ = gst_pipeline_get_bus(GST_PIPELINE(capturePipeline_));

    GstStateChangeReturn ret = gst_element_set_state(capturePipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "AudioProcessor: failed to start capture pipeline" << std::endl;
        cleanup();
        return false;
    }

    shouldRunCapture_ = true;
    capturing_ = true;
    captureThread_ = std::thread(&AudioProcessor::captureLoop, this);
    return true;
}

void AudioProcessor::stopCapture() {
    shouldRunCapture_ = false;

    if (captureThread_.joinable()) {
        captureThread_.join();
    }

    std::lock_guard<std::mutex> lock(captureMutex_);
    capturing_ = false;

    if (capturePipeline_) {
        gst_element_set_state(capturePipeline_, GST_STATE_NULL);
        
        if (captureBus_) {
            gst_object_unref(captureBus_);
            captureBus_ = nullptr;
        }
        
        if (captureSink_) {
            gst_object_unref(captureSink_);
            captureSink_ = nullptr;
        }
        
        gst_object_unref(capturePipeline_);
        capturePipeline_ = nullptr;
    }
}

void AudioProcessor::setStreamForwarder(StreamForwarder* forwarder) {
    forwarder_ = forwarder;
}

bool AudioProcessor::startPlayback(const AudioConfig& config) {
    std::lock_guard<std::mutex> lock(playbackMutex_);
    if (playing_.load()) {
        return true;
    }

    currentPlaybackConfig_ = config;
    
    GError* error = nullptr;
    std::string pipelineStr;
    
    if (config.enableEncryption && !config.encryptionKey.empty()) {
        pipelineStr = buildSecurePlaybackPipelineString(config);
    } else {
        pipelineStr = buildPlaybackPipelineString(config);
    }
    
    playbackPipeline_ = gst_parse_launch(pipelineStr.c_str(), &error);
    if (!playbackPipeline_) {
        std::cerr << "AudioProcessor: failed to create playback pipeline: " 
                  << (error ? error->message : "unknown error") << std::endl;
        if (error) {
            g_error_free(error);
        }
        return false;
    }

    GstElement* src = gst_bin_get_by_name(GST_BIN(playbackPipeline_), "audio_src");
    if (!src) {
        std::cerr << "AudioProcessor: failed to get src from playback pipeline" << std::endl;
        cleanup();
        return false;
    }
    playbackSrc_ = src;

    playbackBus_ = gst_pipeline_get_bus(GST_PIPELINE(playbackPipeline_));

    GstStateChangeReturn ret = gst_element_set_state(playbackPipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "AudioProcessor: failed to start playback pipeline" << std::endl;
        cleanup();
        return false;
    }

    shouldRunPlayback_ = true;
    playing_ = true;
    playbackThread_ = std::thread(&AudioProcessor::playbackLoop, this);
    return true;
}

void AudioProcessor::stopPlayback() {
    shouldRunPlayback_ = false;

    if (playbackThread_.joinable()) {
        playbackThread_.join();
    }

    std::lock_guard<std::mutex> lock(playbackMutex_);
    playing_ = false;

    if (playbackPipeline_) {
        gst_element_set_state(playbackPipeline_, GST_STATE_NULL);
        
        if (playbackBus_) {
            gst_object_unref(playbackBus_);
            playbackBus_ = nullptr;
        }
        
        if (playbackSrc_) {
            gst_object_unref(playbackSrc_);
            playbackSrc_ = nullptr;
        }
        
        gst_object_unref(playbackPipeline_);
        playbackPipeline_ = nullptr;
    }
}

void AudioProcessor::setPlaybackVolume(double volume) {
    std::lock_guard<std::mutex> lock(playbackMutex_);
    volume_ = volume;
}

double AudioProcessor::getPlaybackVolume() const {
    std::lock_guard<std::mutex> lock(playbackMutex_);
    return volume_;
}

bool AudioProcessor::isCapturing() const {
    return capturing_.load();
}

bool AudioProcessor::isPlaying() const {
    return playing_.load();
}

bool AudioProcessor::sendAudioData(const void* data, size_t size) {
    if (!playbackSrc_ || !playing_.load()) {
        return false;
    }

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    if (!buffer) {
        return false;
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        memcpy(map.data, data, size);
        gst_buffer_unmap(buffer, &map);
        
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(playbackSrc_), buffer);
        return ret == GST_FLOW_OK;
    }

    gst_buffer_unref(buffer);
    return false;
}

std::vector<std::string> AudioProcessor::enumerateAudioDevices() const {
    std::vector<std::string> devices;
    
    return devices;
}

void AudioProcessor::setNoiseReductionLevel(float level) {
    noiseReductionLevel_ = level;
}

void AudioProcessor::setEchoCancellation(bool enabled) {
    echoCancellationEnabled_ = enabled;
}

bool AudioProcessor::startIntercom(const std::string& targetDevice) {
    intercomMode_.store(true);

    AudioConfig config;
    config.enableEchoCancellation = true;
    config.enableNoiseReduction = true;
    config.codec = "opus";
    config.bitrate = 48000;
    config.channels = 2;
    config.sampleRate = 128000;

    bool captureStarted = startCapture(config);
    bool playbackStarted = startPlayback(config);

    if (!captureStarted || !playbackStarted) {
        if (!captureStarted) {
            stopPlayback();
        }
        if (!playbackStarted) {
            stopCapture();
        }
        intercomMode_.store(false);
        return false;
    }

    return true;
}

void AudioProcessor::stopIntercom() {
    stopCapture();
    stopPlayback();

    intercomMode_.store(false);
}

void AudioProcessor::setSoundEventCallback(SoundEventCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    soundEventCallback_ = callback;
}

void AudioProcessor::enableSoundEventDetection(bool enable) {
    soundEventDetectionEnabled_ = enable;
}

void AudioProcessor::setAudioTriggeredRecording(bool enable) {
    audioTriggeredRecordingEnabled_ = enable;
}

bool AudioProcessor::isSoundEventDetectionEnabled() const {
    return soundEventDetectionEnabled_.load();
}

bool AudioProcessor::isAudioTriggeredRecordingEnabled() const {
    return audioTriggeredRecordingEnabled_.load();
}

void AudioProcessor::setLowPowerMode(bool enable) {
    lowPowerModeEnabled_ = enable;
}

bool AudioProcessor::isLowPowerModeEnabled() const {
    return lowPowerModeEnabled_.load();
}

void AudioProcessor::setPrivacyMode(bool enable) {
    privacyModeEnabled_ = enable;
    
    if (enable && capturing_.load()) {
        stopCapture();
    }
}

bool AudioProcessor::isPrivacyModeEnabled() const {
    return privacyModeEnabled_.load();
}

void AudioProcessor::enableEncryption(bool enable) {
    encryptionEnabled_ = enable;
}

bool AudioProcessor::isEncryptionEnabled() const {
    return encryptionEnabled_.load();
}

void AudioProcessor::setEncryptionKey(const std::string& key) {
    encryptionKey_ = key;
}

void AudioProcessor::analyzeAudioBuffer(GstBuffer* buffer) {
    if (!soundEventDetectionEnabled_.load() && !audioTriggeredRecordingEnabled_.load()) {
        return;
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        const size_t sampleCount = map.size / sizeof(float);
        std::vector<float> audioData(reinterpret_cast<const float*>(map.data), 
                                     reinterpret_cast<const float*>(map.data) + sampleCount);
        
        if (soundEventDetectionEnabled_.load()) {
            detectSoundEvent(audioData);
        }
        
        if (audioTriggeredRecordingEnabled_.load()) {

        }
        
        gst_buffer_unmap(buffer, &map);
    }
}

bool AudioProcessor::detectSoundEvent(const std::vector<float>& audioData) {
    if (audioData.empty()) {
        return false;
    }

    double energy = 0.0;
    for (const auto& sample : audioData) {
        energy += sample * sample;
    }
    energy /= audioData.size();

    const double threshold = 0.01;
    
    if (energy > threshold) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (soundEventCallback_) {
            SoundEvent event;
            event.type = SoundEventType::Custom;
            event.timestamp = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0;
            event.confidence = std::min(1.0, energy / (threshold * 2));
            event.description = "High energy audio detected";
            
            soundEventCallback_(event);
            return true;
        }
    }
    
    return false;
}

void AudioProcessor::captureLoop() {
    const auto interval = std::chrono::milliseconds(10);

    while (shouldRunCapture_.load()) {
        if (captureBus_) {
            GstMessage* msg = gst_bus_timed_pop_filtered(captureBus_, GST_CLOCK_TIME_NONE, 
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (msg) {
                switch (GST_MESSAGE_TYPE(msg)) {
                    case GST_MESSAGE_ERROR: {
                        GError* err = nullptr;
                        gchar* debug_info = nullptr;
                        gst_message_parse_error(msg, &err, &debug_info);
                        std::cerr << "AudioProcessor: capture error: " << err->message << std::endl;
                        g_error_free(err);
                        g_free(debug_info);
                        break;
                    }
                    case GST_MESSAGE_EOS:
                        std::cerr << "AudioProcessor: capture end of stream" << std::endl;
                        break;
                    default:
                        break;
                }
                gst_message_unref(msg);
            }
        }

        if (intercomMode_.load() && forwarder_) {
            // get audio buffer from GStreamer and forward it
            // TODO: implement buffer retrieval and forwarding
        }

        std::this_thread::sleep_for(interval);
    }

    capturing_ = false;
}

void AudioProcessor::playbackLoop() {
    const auto interval = std::chrono::milliseconds(10);

    while (shouldRunPlayback_.load()) {
        if (playbackBus_) {
            GstMessage* msg = gst_bus_timed_pop_filtered(playbackBus_, GST_CLOCK_TIME_NONE, 
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (msg) {
                switch (GST_MESSAGE_TYPE(msg)) {
                    case GST_MESSAGE_ERROR: {
                        GError* err = nullptr;
                        gchar* debug_info = nullptr;
                        gst_message_parse_error(msg, &err, &debug_info);
                        std::cerr << "AudioProcessor: playback error: " << err->message << std::endl;
                        g_error_free(err);
                        g_free(debug_info);
                        break;
                    }
                    case GST_MESSAGE_EOS:
                        std::cerr << "AudioProcessor: playback end of stream" << std::endl;
                        break;
                    default:
                        break;
                }
                gst_message_unref(msg);
            }
        }

        std::this_thread::sleep_for(interval);
    }

    playing_ = false;
}

bool AudioProcessor::isIntercomMode() const {
    return intercomMode_.load();
}


}