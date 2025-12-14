#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#include <QMetaObject>

#include "core/streams/video_capture.hpp"
#include "core/streams/video_capture_manager.hpp"

namespace {

std::string trimCopy(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool isCameraUri(const std::string& uri) {
    return uri.rfind("camera://", 0) == 0;
}

bool isRtmpUri(const std::string& uri) {
    return uri.rfind("rtmp://", 0) == 0;
}

GstFlowReturn on_new_sample_from_sink(GstAppSink* appsink, gpointer user_data) {
    SnowOwl::Server::Core::VideoCapture* capture = static_cast<SnowOwl::Server::Core::VideoCapture*>(user_data);
    
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (sample) {
        QMetaObject::invokeMethod(capture, [capture, sample]() {
            emit capture->sampleReady(sample);
        }, Qt::QueuedConnection);
    }
    
    return GST_FLOW_OK;
}

}

namespace SnowOwl::Server::Core {

VideoCapture::VideoCapture(QObject* parent,
                           CaptureSourceKind sourceKind,
                           int camera_id,
                           std::string primary_uri,
                           std::string secondary_uri)
    : QObject(parent)
    , sourceKind_(sourceKind)
    , cameraId_(camera_id)
    , primaryUri_(trimCopy(std::move(primary_uri)))
    , secondaryUri_(trimCopy(std::move(secondary_uri)))
    , pipeline_(nullptr)
    , appsink_(nullptr)
    , bus_(nullptr)
    , isRunning_(false)
    , currentSample_(nullptr)
    , lastReconnectAttempt_(std::chrono::steady_clock::now() - reconnectCooldown_) {
    if (sourceKind_ == CaptureSourceKind::Camera) {
        if (cameraId_ < 0 && isCameraUri(primaryUri_)) {
            try {
                cameraId_ = std::stoi(primaryUri_.substr(9));
            } catch (...) {
                cameraId_ = 0;
            }
        }

        if (cameraId_ < 0) {
            cameraId_ = 0;
        }

        activeUri_ = "camera://" + std::to_string(cameraId_);
    } else if (!primaryUri_.empty()) {
        activeUri_ = primaryUri_;
    } else if (!secondaryUri_.empty()) {
        activeUri_ = secondaryUri_;
    }
    
    gst_init(nullptr, nullptr);
}

VideoCapture::~VideoCapture() {
    if (VideoCaptureManager::getInstance()) {
        VideoCaptureManager::getInstance()->removeVideoCapture(cameraId_);
    }
    
    stopVideoCaptureSystem();
}

bool VideoCapture::startVideoCaptureSystem() {
    if (isRunning_.load()) {
        return true;
    }

    if (!openCapture()) {
        std::cerr << "VideoCapture: failed to open " << describeSource() << std::endl;
        return false;
    }

    if (VideoCaptureManager::getInstance()) {
        VideoCaptureManager::getInstance()->addVideoCapture(cameraId_, this);
    }

    isRunning_ = true;
    captureThread_ = std::thread(&VideoCapture::captureLoop, this);
    return true;
}

bool VideoCapture::stopVideoCaptureSystem() {
    isRunning_ = false;

    if (captureThread_.joinable()) {
        captureThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(captureMutex_);
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
            appsink_ = nullptr;
            bus_ = nullptr;
        }
    }

    activeUri_.clear();
    if (sourceKind_ == CaptureSourceKind::Camera) {
        activeUri_ = "camera://" + std::to_string(cameraId_);
    }

    if (currentSample_) {
        gst_sample_unref(currentSample_);
        currentSample_ = nullptr;
    }
    pendingSamples_.store(0, std::memory_order_relaxed);

    return true;
}

GstSample* VideoCapture::getCurrentSample() {
    try {
        std::lock_guard<std::mutex> lock(sampleMutex_);
        if (!currentSample_) {
            return nullptr;
        }
        return gst_sample_ref(currentSample_);
    } catch (const std::exception& e) {
        std::cerr << "VideoCapture: failed to get sample - " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cerr << "VideoCapture: unexpected error while getting sample" << std::endl;
        return nullptr;
    }
}

bool VideoCapture::isOpened() const {
    std::lock_guard<std::mutex> lock(captureMutex_);
    return pipeline_ && GST_STATE(pipeline_) == GST_STATE_PLAYING;
}

void VideoCapture::updateResolution(const std::string& resolution) {
    std::lock_guard<std::mutex> lock(configMutex_);
    pendingConfig_.resolution = resolution;
    configUpdated_ = true;
}

void VideoCapture::updateFps(int fps) {
    std::lock_guard<std::mutex> lock(configMutex_);
    pendingConfig_.fps = fps;
    configUpdated_ = true;
}

void VideoCapture::updateBitrate(int bitrate_kbps) {
    std::lock_guard<std::mutex> lock(configMutex_);
    pendingConfig_.bitrate_kbps = bitrate_kbps;
    configUpdated_ = true;
}

void VideoCapture::updateConfig(const CaptureConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    pendingConfig_ = config;
    configUpdated_ = true;
}

std::string VideoCapture::buildPipelineString() const {
    std::ostringstream pipeline;
    
    switch (sourceKind_) {
        case CaptureSourceKind::Camera:
            pipeline << "v4l2src device=/dev/video" << cameraId_ << " ! videoconvert ! videoscale ! ";
            {
                std::string resolution = config_.resolution;
                size_t x_pos = resolution.find('x');
                std::string width = "1920";
                std::string height = "1080";
                if (x_pos != std::string::npos) {
                    width = resolution.substr(0, x_pos);
                    height = resolution.substr(x_pos + 1);
                }
                pipeline << "video/x-raw,width=" << width << ",height=" << height 
                         << ",framerate=" << config_.fps << "/1 ! "
                         << "videorate ! video/x-raw,framerate=" << config_.fps << "/1 ! "
                         << "vaapipostproc ! vaapih264enc bitrate=" << config_.bitrate_kbps << " ! "
                         << "h264parse ! appsink name=appsink";
            }
            break;
            
        case CaptureSourceKind::File:
            pipeline << "filesrc location=" << activeUri_ << " ! decodebin ! videoconvert ! ";
            pipeline << "video/x-raw,framerate=" << config_.fps << "/1 ! "
                     << "videorate ! video/x-raw,framerate=" << config_.fps << "/1 ! "
                     << "vaapipostproc ! vaapih264enc bitrate=" << config_.bitrate_kbps << " ! "
                     << "h264parse ! appsink name=appsink";
            break;
            
        case CaptureSourceKind::NetworkStream:
        case CaptureSourceKind::RtmpStream:
            if (isRtmpUri(activeUri_)) {
                pipeline << "rtmpsrc location=" << activeUri_ << " ! flvdemux ! h264parse ! avdec_h264 ! videoconvert ! ";
                pipeline << "video/x-raw,framerate=" << config_.fps << "/1 ! "
                         << "videorate ! video/x-raw,framerate=" << config_.fps << "/1 ! "
                         << "vaapipostproc ! vaapih264enc bitrate=" << config_.bitrate_kbps << " ! "
                         << "h264parse ! appsink name=appsink";
            } else {
                pipeline << "urisourcebin uri=" << activeUri_ << " ! videoconvert ! ";
                pipeline << "video/x-raw,framerate=" << config_.fps << "/1 ! "
                         << "videorate ! video/x-raw,framerate=" << config_.fps << "/1 ! "
                         << "vaapipostproc ! vaapih264enc bitrate=" << config_.bitrate_kbps << " ! "
                         << "h264parse ! appsink name=appsink";
            }
            break;
    }
    
    return pipeline.str();
}

bool VideoCapture::openCapture() {
    std::unique_lock<std::mutex> lock(captureMutex_);

    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        appsink_ = nullptr;
        bus_ = nullptr;
    }

    std::string pipeline_str = buildPipelineString();
    
    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (error) {
        std::cerr << "VideoCapture: failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }
    
    if (!pipeline_) {
        std::cerr << "VideoCapture: failed to create pipeline" << std::endl;
        return false;
    }
    
    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "appsink");
    if (!appsink_) {
        std::cerr << "VideoCapture: failed to get appsink from pipeline" << std::endl;
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        return false;
    }

    g_object_set(appsink_, "emit-signals", TRUE, nullptr);
    GstAppSinkCallbacks callbacks = { nullptr, nullptr, on_new_sample_from_sink };
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_), &callbacks, this, nullptr);
    
    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));

    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "VideoCapture: failed to start pipeline" << std::endl;
        gst_object_unref(appsink_);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        appsink_ = nullptr;
        bus_ = nullptr;
        return false;
    }
    
    pendingSamples_.store(0, std::memory_order_relaxed);
    
    if (currentSample_) {
        gst_sample_unref(currentSample_);
        currentSample_ = nullptr;
    }
    
    return true;
}

bool VideoCapture::openCameraLocked() {
    if (cameraId_ < 0) {
        std::cerr << "VideoCapture: invalid camera id" << std::endl;
        return false;
    }
    
    return true;
}

bool VideoCapture::openUriLocked(const std::string& uri) {
    if (uri.empty()) {
        return false;
    }
    
    return true;
}

void VideoCapture::configureCaptureLocked() {
}

bool VideoCapture::attemptReconnect(const char* reason) {
    if (!isNetworkSource() && !isFileSource()) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if ((now - lastReconnectAttempt_) < reconnectCooldown_) {
        return false;
    }

    lastReconnectAttempt_ = now;

    std::cerr << "VideoCapture: attempting reconnect for " << describeSource()
              << " (reason: " << reason << ")" << std::endl;

    if (openCapture()) {
        std::cout << "VideoCapture: reconnected " << describeSource() << std::endl;
        return true;
    }

    std::cerr << "VideoCapture: reconnect failed for " << describeSource() << std::endl;
    return false;
}

bool VideoCapture::shouldAttemptReconnect(int failureCount) const {
    if (failureCount <= 0) {
        return false;
    }

    if (isNetworkSource()) {
        return failureCount >= networkFailureThreshold_;
    }

    if (isFileSource()) {
        return failureCount >= networkFailureThreshold_;
    }

    return failureCount >= cameraFailureThreshold_;
}

bool VideoCapture::isNetworkSource() const {
    return sourceKind_ == CaptureSourceKind::NetworkStream || sourceKind_ == CaptureSourceKind::RtmpStream;
}

bool VideoCapture::isFileSource() const {
    return sourceKind_ == CaptureSourceKind::File;
}

std::string VideoCapture::describeSource() const {
    std::ostringstream oss;

    if (sourceKind_ == CaptureSourceKind::Camera) {
        oss << "camera #" << (cameraId_ < 0 ? 0 : cameraId_);
        return oss.str();
    }

    const std::string uri = !activeUri_.empty() ? activeUri_
                                                 : (!primaryUri_.empty() ? primaryUri_ : secondaryUri_);

    switch (sourceKind_) {
        case CaptureSourceKind::NetworkStream:
            oss << "network stream";
            break;
        case CaptureSourceKind::RtmpStream:
            oss << "rtmp stream";
            break;
        case CaptureSourceKind::File:
            oss << "file source";
            break;
        case CaptureSourceKind::Camera:
            break;
    }

    if (!uri.empty()) {
        oss << " (" << uri << ")";
    }

    return oss.str();
}

void VideoCapture::applyConfigUpdates() {
    std::lock_guard<std::mutex> lock(configMutex_);
    if (configUpdated_.load()) {
        if (config_ != pendingConfig_) {
            config_ = pendingConfig_;
            
            std::cout << "VideoCapture: Applying new configuration - Resolution: " << config_.resolution 
                      << ", FPS: " << config_.fps << ", Bitrate: " << config_.bitrate_kbps << " kbps" << std::endl;
            
            openCapture();
        }
        configUpdated_ = false;
        lastConfigUpdate_ = std::chrono::steady_clock::now();
    }
}

void VideoCapture::captureLoop() {
    const auto interval = std::chrono::milliseconds(frameIntervalMs_);

    while (isRunning_.load()) {
        {
            auto now = std::chrono::steady_clock::now();
            if (configUpdated_.load() && 
                (now - lastConfigUpdate_) >= configUpdateCooldown_) {
                applyConfigUpdates();
            }
        }

        if (bus_) {
            GstMessage* msg = gst_bus_timed_pop_filtered(bus_, 0, 
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (msg) {
                switch (GST_MESSAGE_TYPE(msg)) {
                    case GST_MESSAGE_ERROR: {
                        GError* err = nullptr;
                        gchar* debug_info = nullptr;
                        gst_message_parse_error(msg, &err, &debug_info);
                        std::cerr << "VideoCapture: error received from bus: " << err->message << std::endl;
                        g_error_free(err);
                        g_free(debug_info);
                        break;
                    }
                    case GST_MESSAGE_EOS:
                        std::cerr << "VideoCapture: end of stream" << std::endl;
                        break;
                    default:
                        break;
                }
                gst_message_unref(msg);
            }
        }

        std::this_thread::sleep_for(interval);
    }
}

}