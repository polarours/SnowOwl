#pragma once

#include <QObject>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "core/streams/capture_types.hpp"
#include "core/streams/video_capture_manager.hpp"

namespace SnowOwl::Server::Core {

struct CaptureConfig {
    std::string resolution = "1920x1080";
    int fps = 30;
    int bitrate_kbps = 2000;
    
    bool operator!=(const CaptureConfig& other) const {
        return resolution != other.resolution ||
               fps != other.fps ||
               bitrate_kbps != other.bitrate_kbps;
    }
};

class VideoCapture : public QObject
{
    Q_OBJECT

public:
    explicit VideoCapture(QObject* parent = nullptr,
                          CaptureSourceKind sourceKind = CaptureSourceKind::Camera,
                          int camera_id = -1,
                          std::string primary_uri = {},
                          std::string secondary_uri = {});
    ~VideoCapture();

    bool startVideoCaptureSystem();
    bool stopVideoCaptureSystem();

    GstSample* getCurrentSample();

    bool isOpened() const;
    
    void updateResolution(const std::string& resolution);
    void updateFps(int fps);
    void updateBitrate(int bitrate_kbps);
    void updateConfig(const CaptureConfig& config);

private:
    void captureLoop();
    bool openCapture();
    bool openCameraLocked();
    bool openUriLocked(const std::string& uri);
    void configureCaptureLocked();
    bool attemptReconnect(const char* reason);
    bool shouldAttemptReconnect(int failureCount) const;
    bool isNetworkSource() const;
    bool isFileSource() const;
    std::string describeSource() const;
    std::string buildPipelineString() const;
    
    void applyConfigUpdates();

signals:
    void sampleReady(GstSample* sample);

private:
    CaptureSourceKind sourceKind_;
    int cameraId_;
    std::string primaryUri_;
    std::string secondaryUri_;
    std::string activeUri_;
    
    GstElement* pipeline_;
    GstElement* appsink_;
    GstBus* bus_;
    
    std::thread captureThread_;
    std::atomic<bool> isRunning_;
    std::atomic<int> pendingSamples_{0};
    
    GstSample* currentSample_;
    std::mutex sampleMutex_;
    mutable std::mutex captureMutex_;
    
    CaptureConfig config_;
    CaptureConfig pendingConfig_;
    std::mutex configMutex_;
    std::atomic<bool> configUpdated_{false};
    
    std::chrono::steady_clock::time_point lastConfigUpdate_;
    const std::chrono::milliseconds configUpdateCooldown_{100};
    
    const int frameIntervalMs_ = 33;
    const int networkFailureThreshold_ = 15;
    const int cameraFailureThreshold_ = 60;
    std::chrono::steady_clock::time_point lastReconnectAttempt_;
    const std::chrono::milliseconds reconnectCooldown_{1500};
};

}