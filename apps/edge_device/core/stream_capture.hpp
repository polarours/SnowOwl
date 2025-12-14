#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>

namespace SnowOwl::Edge::Core {

enum class CaptureMode {
    Camera,
    Network,
    File
};

struct CaptureSourceConfig {
    CaptureMode mode{CaptureMode::Camera}; // Default to Camera
    int cameraIndex{0};
    std::string primaryUri;
    std::string fallbackUri;
};

class StreamCapture {
public:
    StreamCapture();
    ~StreamCapture();

    void configure(const CaptureSourceConfig& config);

    bool start();
    void stop();

    cv::Mat latestFrame() const;
    bool isRunning() const;

private:
    bool initializeGstPipeline();
    void cleanupGstPipeline();
    static GstFlowReturn onNewSample(GstAppSink* appsink, gpointer userData);
    cv::Mat gstSampleToMat(GstSample* sample);
    std::string buildPipelineString();
    void captureLoop();

    mutable std::mutex mutex_;
    mutable std::mutex frameMutex_;
    cv::Mat frame_;
    std::thread thread_;

    CaptureSourceConfig config_;
    std::string activeUri_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shouldRun_{false};

    GstElement* pipeline_ = nullptr;
    GstElement* appsink_ = nullptr;
    GstBus* bus_ = nullptr;
    
    std::chrono::steady_clock::time_point lastReconnectAttempt_;
    const std::chrono::milliseconds reconnectCooldown_{1500};
};

}