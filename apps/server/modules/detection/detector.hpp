#pragma once

#include <opencv2/core.hpp>
#include <vector>

#include "detection/detection_types.hpp"

namespace SnowOwl::Server::Modules::Detection {

using SnowOwl::Detection::DetectionResult;
using SnowOwl::Detection::DetectionType;

class IDetector {
public:
    virtual ~IDetector() = default;
    virtual DetectionType type() const = 0;
    virtual bool enabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual void process(const cv::Mat& frame, std::vector<DetectionResult>& outResults) = 0;
};

// Forward declaration of our unified detector
class UnifiedDetector;

}