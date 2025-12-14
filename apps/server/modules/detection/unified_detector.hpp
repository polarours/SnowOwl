#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <memory>

#include "detector.hpp"

#ifdef HAVE_ONNXRUNTIME
namespace Ort {
class Env;
class Session;
}
#endif

namespace SnowOwl::Server::Modules::Detection {

class UnifiedDetector : public IDetector {
public:
    UnifiedDetector();
    ~UnifiedDetector();

    // Return a general detection type since this detector can detect multiple types
    DetectionType type() const override { return DetectionType::EquipmentFailure; }
    bool enabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }
    void process(const cv::Mat& frame, std::vector<DetectionResult>& outResults) override;

private:
    bool enabled_{false};
    
#ifdef HAVE_ONNXRUNTIME
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
#endif
    
    int inputWidth_ = 640;
    int inputHeight_ = 640;
    float confidenceThreshold_ = 0.5f;
    float nmsThreshold_ = 0.4f;

    std::vector<std::string> classNames_;
    
    void initializeModel();
    cv::Mat preprocessImage(const cv::Mat& image);
    std::vector<DetectionResult> postprocessDetections(
        const std::vector<float>& output, 
        const cv::Size& originalSize);
    std::vector<int> applyNMS(
        const std::vector<cv::Rect>& boxes, 
        const std::vector<float>& scores);
        
    DetectionType mapClassToDetectionType(const std::string& className) const;
};

}