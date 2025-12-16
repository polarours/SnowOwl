#pragma once

#include <memory>
#include <vector>
#include <map>

#include <opencv2/core.hpp>
#include <gst/gst.h>

#include "config/config_manager.hpp"
#include "detection/detection_types.hpp"
#include "modules/detection/detector.hpp"
#include "modules/network/network_server.hpp"
#include "stream_dispatcher.hpp"

namespace SnowOwl::Server::Core {

using SnowOwl::Detection::DetectionResult;
using SnowOwl::Detection::DetectionType;
using ServerDetector = SnowOwl::Server::Modules::Detection::IDetector;

class VideoProcessor {
public:
    VideoProcessor();
    ~VideoProcessor();

    std::vector<DetectionResult> processFrame(const cv::Mat& frame);
    std::vector<DetectionResult> processSample(GstSample* sample);
    
    void setNetworkServer(SnowOwl::Server::Modules::Network::NetworkServer* server) { 
        networkServer_ = server; 
    }

    void setIntrusionDetection(bool enabled);
    void setFireDetection(bool enabled);
    void setMotionDetection(bool enabled);
    void setGasLeakDetection(bool enabled);
    void setEquipmentDetection(bool enabled);
    void setFaceRecognition(bool enabled);
    void setPipelineInspection(bool enabled);

    void setDetectionEnabled(DetectionType type, bool enabled);
    bool isDetectionEnabled(DetectionType type) const;
    bool isAnyDetectionEnabled() const;

    void applyConfiguration(const Config::ConfigManager& configManager);
    
    void setStreamProfile(const StreamTargetProfile& profile) { streamProfile_ = profile; }
    const StreamTargetProfile& getStreamProfile() const { return streamProfile_; }

    static void drawDetections(cv::Mat& frame, const std::vector<DetectionResult>& detections);

private:
    std::vector<std::unique_ptr<ServerDetector>> detectors_;
    std::map<DetectionType, ServerDetector*> detectorIndex_;
    bool detectorsInitialized_ = false;
    SnowOwl::Server::Modules::Network::NetworkServer* networkServer_ = nullptr;
    StreamTargetProfile streamProfile_;

    ServerDetector* findDetector(DetectionType type);
    const ServerDetector* findDetector(DetectionType type) const;
    void ensureDetectors();
    
    cv::Mat sampleToMat(GstSample* sample);
};

}