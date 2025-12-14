#include "unified_detector.hpp"

#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv4/opencv2/dnn.hpp>

#ifdef HAVE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace SnowOwl::Server::Modules::Detection {

UnifiedDetector::UnifiedDetector() {
    // COCO dataset class names
    classNames_ = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
        "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
        "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
        "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
        "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
        "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
        "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
        "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book",
        "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
    };
    
    try {
        initializeModel();
        enabled_ = true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Unified detector: " << e.what() << std::endl;
        enabled_ = false;
    }
}

UnifiedDetector::~UnifiedDetector() = default;

void UnifiedDetector::initializeModel() {
#ifdef HAVE_ONNXRUNTIME
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "UnifiedDetector");

    // Try to load the model from different possible paths
    std::vector<std::string> model_paths = {
        "./yolov8n.onnx",
        "../externals/yolov8n.onnx",
        "./externals/yolov8n.onnx",
        "/home/polarours/Projects/Personal/SnowOwl/yolov8n.onnx",
        "./externals/onnx/yolov8n.onnx",                                    // Added your path
        "../externals/onnx/yolov8n.onnx",
        "/home/polarours/Projects/Personal/SnowOwl/externals/onnx/yolov8n.onnx"  // Added your full path
    };
    
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    
    bool model_loaded = false;
    for (const auto& model_path : model_paths) {
        try {
            session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(), session_options);
            std::cout << "YOLO model loaded successfully from: " << model_path << std::endl;
            model_loaded = true;
            break;
        } catch (const Ort::Exception& e) {
            std::cerr << "Failed to load model from " << model_path << ": " << e.what() << std::endl;
            continue;
        }
    }
    
    if (!model_loaded) {
        throw std::runtime_error("Failed to load YOLO model from any of the expected paths");
    }
#else
    std::cerr << "ONNX Runtime not available. Unified detector will not function." << std::endl;
    throw std::runtime_error("ONNX Runtime not available");
#endif
}

void UnifiedDetector::process(const cv::Mat& frame, std::vector<DetectionResult>& outResults) {
    if (!enabled_ || frame.empty()) {
        return;
    }
    
#ifdef HAVE_ONNXRUNTIME
    if (!session_) {
        return;
    }

    // Preprocess the image
    cv::Mat processedImage = preprocessImage(frame);

    // Prepare input tensor
    std::vector<int64_t> input_shape = {1, 3, inputHeight_, inputWidth_};
    size_t input_tensor_size = 1 * 3 * inputHeight_ * inputWidth_;

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, 
        (float*)processedImage.data, 
        input_tensor_size, 
        input_shape.data(), 
        input_shape.size()
    );

    // Define input and output names
    const char* input_names[] = {"images"};
    const char* output_names[] = {"output0"};

    // Run inference
    auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr}, 
        input_names, 
        &input_tensor, 
        1, 
        output_names, 
        1
    );

    // Process output
    if (!output_tensors.empty()) {
        auto& output_tensor = output_tensors.front();
        std::vector<float> output_data;

        float* floatarr = output_tensor.GetTensorMutableData<float>();
        size_t output_count = output_tensor.GetTensorTypeAndShapeInfo().GetElementCount();
        output_data.assign(floatarr, floatarr + output_count);

        // Post-process detections
        std::vector<DetectionResult> detections = postprocessDetections(output_data, frame.size());

        // Add detections to output
        outResults.insert(outResults.end(), detections.begin(), detections.end());
    }
#endif
}

cv::Mat UnifiedDetector::preprocessImage(const cv::Mat& image) {
    cv::Mat resizedImage;
    cv::resize(image, resizedImage, cv::Size(inputWidth_, inputHeight_));

    // Convert BGR to RGB
    cv::cvtColor(resizedImage, resizedImage, cv::COLOR_BGR2RGB);

    // Normalize to [0,1]
    cv::Mat floatImage;
    resizedImage.convertTo(floatImage, CV_32F, 1.0 / 255.0);

    // Create blob from image
    cv::Mat blob;
    cv::dnn::blobFromImage(floatImage, blob);
    
    return blob;
}

std::vector<DetectionResult> UnifiedDetector::postprocessDetections(
    const std::vector<float>& output, 
    const cv::Size& originalSize) {
    std::vector<DetectionResult> detections;
    
#ifdef HAVE_ONNXRUNTIME
    const int num_classes = classNames_.size();
    const int num_detections = output.size() / (4 + num_classes);
    
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> class_ids;
    
    for (int i = 0; i < num_detections; ++i) {
        // Extract class scores
        std::vector<float> scores;
        for (int c = 0; c < num_classes; ++c) {
            scores.push_back(output[i * (4 + num_classes) + 4 + c]);
        }

        // Find the class with the highest score
        auto max_score_it = std::max_element(scores.begin(), scores.end());
        float max_score = *max_score_it;
        int class_id = std::distance(scores.begin(), max_score_it);

        // Filter by confidence threshold
        if (max_score > confidenceThreshold_) {
            // Extract bounding box coordinates
            float x = output[i * (4 + num_classes) + 0];
            float y = output[i * (4 + num_classes) + 1];
            float w = output[i * (4 + num_classes) + 2];
            float h = output[i * (4 + num_classes) + 3];

            // Convert to pixel coordinates
            int left = (int)((x - w / 2) * originalSize.width / inputWidth_);
            int top = (int)((y - h / 2) * originalSize.height / inputHeight_);
            int width = (int)(w * originalSize.width / inputWidth_);
            int height = (int)(h * originalSize.height / inputHeight_);
            
            // Ensure bounding box is within image bounds
            left = std::max(0, std::min(left, originalSize.width - 1));
            top = std::max(0, std::min(top, originalSize.height - 1));
            width = std::max(1, std::min(width, originalSize.width - left));
            height = std::max(1, std::min(height, originalSize.height - top));

            boxes.emplace_back(left, top, width, height);
            confidences.push_back(max_score);
            class_ids.push_back(class_id);
        }
    }
    
    // Apply Non-Maximum Suppression
    std::vector<int> indices = applyNMS(boxes, confidences);
    
    // Create detection results
    for (int idx : indices) {
        DetectionResult result;
        result.type = mapClassToDetectionType(classNames_[class_ids[idx]]);
        result.boundingBox = boxes[idx];
        result.confidence = confidences[idx];
        result.description = classNames_[class_ids[idx]];
        detections.push_back(result);
    }
#endif
    
    return detections;
}

std::vector<int> UnifiedDetector::applyNMS(
    const std::vector<cv::Rect>& boxes, 
    const std::vector<float>& scores) {
    std::vector<int> indices;
    
#ifdef HAVE_ONNXRUNTIME
    cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold_, nmsThreshold_, indices);
#endif
    
    return indices;
}

DetectionType UnifiedDetector::mapClassToDetectionType(const std::string& className) const {
    // Map COCO classes to our detection types
    if (className == "person") {
        return DetectionType::Intrusion;
    } else if (className == "fire hydrant" || className == "hot dog") {
        // fire hydrant sometimes detected in fire scenarios, hot dog has similar shape/color
        return DetectionType::Fire;
    } else if (className == "bottle" || className == "wine glass") {
        // Could indicate gas leak scenarios
        return DetectionType::GasLeak;
    } else {
        // Default to equipment failure for other objects
        return DetectionType::EquipmentFailure;
    }
}

}