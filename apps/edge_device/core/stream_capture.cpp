#include <chrono>
#include <iostream>
#include <thread>

#include "core/stream_capture.hpp"

namespace SnowOwl::Edge::Core {

namespace {

bool isCameraUri(const std::string& uri) {
    return uri.rfind("camera://", 0) == 0;
}

}

StreamCapture::StreamCapture()
    : lastReconnectAttempt_(std::chrono::steady_clock::now() - reconnectCooldown_) 
{
    gst_init(nullptr, nullptr);
}

StreamCapture::~StreamCapture() {
    stop();
}

void StreamCapture::configure(const CaptureSourceConfig& config) {
    bool restart = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
        
        if (config_.mode == CaptureMode::Camera && config_.cameraIndex < 0) {
            config_.cameraIndex = 0;
        }

        if (!config_.primaryUri.empty() && isCameraUri(config_.primaryUri) && config_.mode != CaptureMode::Camera) {
            try {
                config_.cameraIndex = std::stoi(config_.primaryUri.substr(9));
                config_.mode = CaptureMode::Camera;
            } catch (...) {
                // ignore invalid camera index
            }
        }

        restart = running_.load();
    }

    if (restart) {
        stop();
        
        if (!start()) {
            std::cerr << "StreamCapture: failed to restart after reconfiguration" << std::endl;
        }
    }
}

bool StreamCapture::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_.load()) {
        return true;
    }

    if (!initializeGstPipeline()) {
        return false;
    }

    shouldRun_ = true;
    running_ = true;
    thread_ = std::thread(&StreamCapture::captureLoop, this);
    return true;
}

void StreamCapture::stop() {
    shouldRun_ = false;

    if (thread_.joinable()) {
        thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        activeUri_.clear();
        cleanupGstPipeline();
    }

    std::lock_guard<std::mutex> frameLock(frameMutex_);
    frame_.release();
}

cv::Mat StreamCapture::latestFrame() const {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (frame_.empty()) {
        return {};
    }

    return frame_.clone();
}

bool StreamCapture::isRunning() const {
    return running_.load();
}

bool StreamCapture::initializeGstPipeline() {
    cleanupGstPipeline();

    std::string pipelineStr = buildPipelineString();
    if (pipelineStr.empty()) {
        return false;
    }

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipelineStr.c_str(), &error);
    if (!pipeline_) {
        if (error) {
            g_error_free(error);
        }
        return false;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "appsink");
    if (!appsink_) {
        cleanupGstPipeline();
        return false;
    }

    GstAppSinkCallbacks callbacks = { nullptr, nullptr, nullptr };
    callbacks.new_sample = onNewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_), &callbacks, this, nullptr);
    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink_), FALSE);

    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));

    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        cleanupGstPipeline();
        return false;
    }

    return true;
}

void StreamCapture::cleanupGstPipeline() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        
        if (bus_) {
            gst_object_unref(bus_);
            bus_ = nullptr;
        }
        
        if (appsink_) {
            gst_object_unref(appsink_);
            appsink_ = nullptr;
        }
        
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

GstFlowReturn StreamCapture::onNewSample(GstAppSink* appsink, gpointer userData) {
    StreamCapture* capture = static_cast<StreamCapture*>(userData);
    
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_ERROR;
    }

    cv::Mat frame = capture->gstSampleToMat(sample);
    
    {
        std::lock_guard<std::mutex> lock(capture->frameMutex_);
        capture->frame_ = frame.clone();
    }
    
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

cv::Mat StreamCapture::gstSampleToMat(GstSample* sample) {
    if (!sample) {
        return cv::Mat();
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return cv::Mat();
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return cv::Mat();
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    if (!caps) {
        gst_buffer_unmap(buffer, &map);
        return cv::Mat();
    }

    GstStructure* structure = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);
    
    const char* format = gst_structure_get_string(structure, "format");
    
    cv::Mat frame;
    if (format && strcmp(format, "RGB") == 0) {
        frame = cv::Mat(height, width, CV_8UC3, map.data).clone();
    } else if (format && strcmp(format, "I420") == 0) {
        frame = cv::Mat(height * 3/2, width, CV_8UC1, map.data).clone();
        cv::cvtColor(frame, frame, cv::COLOR_YUV2BGR_I420);
    } else {
        frame = cv::Mat(height, width, CV_8UC3, map.data).clone();
    }

    gst_buffer_unmap(buffer, &map);
    return frame;
}

std::string StreamCapture::buildPipelineString() {
    std::string pipeline;

    switch (config_.mode) {
        case CaptureMode::Camera: {
            pipeline = "v4l2src device=/dev/video" + std::to_string(config_.cameraIndex) + 
                       " ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
            activeUri_ = "camera://" + std::to_string(config_.cameraIndex);
            break;
        }
        case CaptureMode::Network: {
            if (!config_.primaryUri.empty()) {
                if (config_.primaryUri.find("rtsp://") == 0) {
                    pipeline = "rtspsrc location=" + config_.primaryUri + " latency=0 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                } else if (config_.primaryUri.find("rtmp://") == 0) {
                    pipeline = "rtmpsrc location=" + config_.primaryUri + " ! flvdemux ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                } else {
                    pipeline = "souphttpsrc location=" + config_.primaryUri + " ! decodebin ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                }
                activeUri_ = config_.primaryUri;
            } else if (!config_.fallbackUri.empty()) {
                if (config_.fallbackUri.find("rtsp://") == 0) {
                    pipeline = "rtspsrc location=" + config_.fallbackUri + " latency=0 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                } else if (config_.fallbackUri.find("rtmp://") == 0) {
                    pipeline = "rtmpsrc location=" + config_.fallbackUri + " ! flvdemux ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                } else {
                    pipeline = "souphttpsrc location=" + config_.fallbackUri + " ! decodebin ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                }
                activeUri_ = config_.fallbackUri;
            }
            break;
        }
        case CaptureMode::File: {
            if (!config_.primaryUri.empty()) {
                pipeline = "filesrc location=" + config_.primaryUri + " ! decodebin ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                activeUri_ = config_.primaryUri;
            } else if (!config_.fallbackUri.empty()) {
                pipeline = "filesrc location=" + config_.fallbackUri + " ! decodebin ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink";
                activeUri_ = config_.fallbackUri;
            }
            break;
        }
    }

    return pipeline;
}

void StreamCapture::captureLoop() {
    const auto interval = std::chrono::milliseconds(5);

    while (shouldRun_.load()) {
        if (bus_) {
            GstMessage* msg = gst_bus_timed_pop(bus_, 0);
            
            if (msg) {
                switch (GST_MESSAGE_TYPE(msg)) {
                    case GST_MESSAGE_ERROR: {
                        GError* err = nullptr;
                        gchar* debug_info = nullptr;
                        gst_message_parse_error(msg, &err, &debug_info);

                        g_error_free(err);
                        g_free(debug_info);
                        break;
                    }
                    case GST_MESSAGE_EOS:
                
                        break;
                    default:
                        break;
                }
                gst_message_unref(msg);
            }
        }
        
        std::this_thread::sleep_for(interval);
    }

    running_ = false;
}

}