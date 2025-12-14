#pragma once

#include <opencv2/core.hpp>
#include <string>

namespace SnowOwl::Detection {

enum class DetectionType {
    Motion,
    Intrusion,
    Fire,
    GasLeak,
    EquipmentFailure,
    FaceRecognition 
};

struct DetectionResult {
    DetectionType type;
    cv::Rect boundingBox;
    float confidence;
    std::string description;
};

inline std::string detectionTypeToString(DetectionType type) {
    switch (type) {
        case DetectionType::Motion:
            return "motion";
        case DetectionType::Intrusion:
            return "intrusion";
        case DetectionType::Fire:
            return "fire";
        case DetectionType::GasLeak:
            return "gas_leak";
        case DetectionType::EquipmentFailure:
            return "equipment";
        case DetectionType::FaceRecognition:
            return "face_recognition";
    }
    return "unknown";
}

}