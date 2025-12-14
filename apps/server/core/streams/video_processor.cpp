#include <algorithm>
#include <iostream>

#include <opencv2/imgproc.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>

#include "video_processor.hpp"
#include "modules/detection/detector.hpp"
#include "modules/detection/unified_detector.hpp"
#include "modules/network/network_server.hpp"

namespace SnowOwl::Server::Core {

using SnowOwl::Detection::DetectionResult;
using SnowOwl::Detection::DetectionType;
using ServerDetector = SnowOwl::Server::Modules::Detection::IDetector;

namespace {

template <typename DetectorT, typename ContainerT>
void addDetector(ContainerT& storage, std::map<DetectionType, ServerDetector*>& index) {
    auto detector = std::make_unique<DetectorT>();
    ServerDetector* raw = detector.get();
    index[raw->type()] = raw;
    storage.push_back(std::move(detector));
}

}

VideoProcessor::VideoProcessor() {
    ensureDetectors();
}

VideoProcessor::~VideoProcessor() = default;

std::vector<DetectionResult> VideoProcessor::processFrame(const cv::Mat& frame) {
    ensureDetectors();

    std::vector<DetectionResult> results;

    if (frame.empty()) {
        return results;
    }

    try {
        for (auto& detector : detectors_) {
            if (!detector->enabled()) {
                continue;
            }
            detector->process(frame, results);
        }
        
        if (networkServer_ && !results.empty()) {
            networkServer_->broadcastEvents(results);
        }
    } catch (const cv::Exception& e) {
        std::cerr << "VideoProcessor OpenCV error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "VideoProcessor error: " << e.what() << std::endl;
    }

    return results;
}

std::vector<DetectionResult> VideoProcessor::processSample(GstSample* sample) {

    ensureDetectors();

    std::vector<DetectionResult> results;

    if (!sample) {
        return results;
    }

    try {
        cv::Mat frame = sampleToMat(sample);
        
        if (frame.empty()) {
            return results;
        }

        for (auto& detector : detectors_) {
            if (!detector->enabled()) {
                continue;
            }
            detector->process(frame, results);
        }
        
        if (networkServer_ && !results.empty()) {
            networkServer_->broadcastEvents(results);
        }
    } catch (const std::exception& e) {
        std::cerr << "VideoProcessor error: " << e.what() << std::endl;
    }

    return results;
}

cv::Mat VideoProcessor::sampleToMat(GstSample* sample) {
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

void VideoProcessor::setIntrusionDetection(bool enabled) {
    setDetectionEnabled(DetectionType::Intrusion, enabled);
}

void VideoProcessor::setFireDetection(bool enabled) {
    setDetectionEnabled(DetectionType::Fire, enabled);
}

void VideoProcessor::setMotionDetection(bool enabled) {
    setDetectionEnabled(DetectionType::Motion, enabled);
}

void VideoProcessor::setGasLeakDetection(bool enabled) {
    setDetectionEnabled(DetectionType::GasLeak, enabled);
}

void VideoProcessor::setEquipmentDetection(bool enabled) {
    setDetectionEnabled(DetectionType::EquipmentFailure, enabled);
}

void VideoProcessor::setFaceRecognition(bool enabled) {
    setDetectionEnabled(DetectionType::FaceRecognition, enabled);
}

void VideoProcessor::setPipelineInspection(bool enabled) {
    setDetectionEnabled(DetectionType::EquipmentFailure, enabled);
}

void VideoProcessor::setDetectionEnabled(DetectionType type, bool enabled) {

    ensureDetectors();
    if (auto* detector = findDetector(type)) {
        detector->setEnabled(enabled);
    }
}

bool VideoProcessor::isDetectionEnabled(DetectionType type) const {
    const auto* detector = findDetector(type);
    return detector ? detector->enabled() : false;
}

bool VideoProcessor::isAnyDetectionEnabled() const {
    // Check if any detection type is enabled
    return isDetectionEnabled(DetectionType::Motion) ||
           isDetectionEnabled(DetectionType::Intrusion) ||
           isDetectionEnabled(DetectionType::Fire) ||
           isDetectionEnabled(DetectionType::GasLeak) ||
           isDetectionEnabled(DetectionType::EquipmentFailure) ||
           isDetectionEnabled(DetectionType::FaceRecognition);
}

void VideoProcessor::applyConfiguration(const Config::ConfigManager& configManager) {
    if (configManager.has("detection.motion.enabled")) {
        bool enabled = configManager.get("detection.motion.enabled").get<bool>();
        setMotionDetection(enabled);
    }
    
    if (configManager.has("detection.intrusion.enabled")) {
        bool enabled = configManager.get("detection.intrusion.enabled").get<bool>();
        setIntrusionDetection(enabled);
    }
    
    if (configManager.has("detection.fire.enabled")) {
        bool enabled = configManager.get("detection.fire.enabled").get<bool>();
        setFireDetection(enabled);
    }
    
    if (configManager.has("detection.gas_leak.enabled")) {
        bool enabled = configManager.get("detection.gas_leak.enabled").get<bool>();
        setGasLeakDetection(enabled);
    }
    
    if (configManager.has("detection.equipment.enabled")) {
        bool enabled = configManager.get("detection.equipment.enabled").get<bool>();
        setEquipmentDetection(enabled);
    }
    
    if (configManager.has("detection.face_recognition.enabled")) {
        bool enabled = configManager.get("detection.face_recognition.enabled").get<bool>();
        setFaceRecognition(enabled);
    }
}

ServerDetector* VideoProcessor::findDetector(DetectionType type) {
    auto it = detectorIndex_.find(type);
    if (it == detectorIndex_.end()) {
        return nullptr;
    }
    return it->second;
}

const ServerDetector* VideoProcessor::findDetector(DetectionType type) const {
    auto it = detectorIndex_.find(type);
    if (it == detectorIndex_.end()) {
        return nullptr;
    }
    return it->second;
}

void VideoProcessor::ensureDetectors() {
    if (detectorsInitialized_) {
        return;
    }

    detectorsInitialized_ = true;
    detectors_.clear();
    detectorIndex_.clear();

    // Use our new unified detector instead of the multiple specialized detectors
    addDetector<SnowOwl::Server::Modules::Detection::UnifiedDetector>(detectors_, detectorIndex_);

    // Enable the detector by default
    if (auto* detector = findDetector(DetectionType::EquipmentFailure)) {
        detector->setEnabled(true);
    }
}

void VideoProcessor::drawDetections(cv::Mat& frame, const std::vector<DetectionResult>& detections) {
    for (const auto& detection : detections) {
        cv::Scalar color(0, 255, 0);
        
        if (detection.type == DetectionType::Fire) {
            color = cv::Scalar(0, 0, 255);
        } else if (detection.type == DetectionType::Intrusion) {
            color = cv::Scalar(0, 255, 255);
        } else if (detection.type == DetectionType::EquipmentFailure) {
            color = cv::Scalar(255, 0, 0);
        }

        cv::rectangle(frame, detection.boundingBox, color, 2);
        
        std::string label = detection.description;
        if (label.empty()) {
            label = SnowOwl::Detection::detectionTypeToString(detection.type);
        }
        
        int baseline = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        
        cv::rectangle(frame, 
            cv::Point(detection.boundingBox.x, detection.boundingBox.y - labelSize.height - 5),
            cv::Point(detection.boundingBox.x + labelSize.width, detection.boundingBox.y),
            color, -1);
            
        cv::putText(frame, label,
            cv::Point(detection.boundingBox.x, detection.boundingBox.y - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }
}

}