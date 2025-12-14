#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "core/streams/video_capture.hpp"
#include "core/streams/video_processor.hpp"
#include "detection/detection_types.hpp"

namespace SnowOwl::Server::Core {

struct CaptureSourceConfig {
	CaptureSourceKind kind{CaptureSourceKind::Camera};
	int cameraId{0};
	std::string primaryUri;
	std::string secondaryUri;
};

class VideoCaptureManager {
public:
	using SampleCallback = std::function<void(GstSample*)>;
    using FrameCallback = std::function<void(cv::Mat&)>;
	using DetectionCallback = std::function<void(const std::vector<Detection::DetectionResult>&)>;

	VideoCaptureManager();
	~VideoCaptureManager();

	VideoCaptureManager(const VideoCaptureManager&) = delete;
	VideoCaptureManager& operator=(const VideoCaptureManager&) = delete;
	VideoCaptureManager(VideoCaptureManager&&) = delete;
	VideoCaptureManager& operator=(VideoCaptureManager&&) = delete;

	bool start(const CaptureSourceConfig& config, SampleCallback sampleCallback, DetectionCallback detectionCallback);
    bool start(const CaptureSourceConfig& config, FrameCallback frameCallback, DetectionCallback detectionCallback);

	bool restart(const CaptureSourceConfig& config);
	void stop();

	[[nodiscard]] bool isRunning() const { return running_.load(); }
	
	VideoProcessor& getProcessor() { return processor_; }
    
    static VideoCaptureManager* getInstance();

    void addVideoCapture(int deviceId, VideoCapture* capture);
    void removeVideoCapture(int deviceId);
    VideoCapture* getVideoCapture(int deviceId);

private:
	void captureLoop();
	void processingLoop();
	void clearQueue();

	CaptureSourceConfig config_;
	std::unique_ptr<VideoCapture> capture_;
	VideoProcessor processor_;

	SampleCallback sampleCallback_;
    FrameCallback frameCallback_;
	DetectionCallback detectionCallback_;

	std::atomic<bool> running_{false};
	std::atomic<bool> captureActive_{false};

	std::thread captureThread_;
	std::thread processingThread_;

	std::mutex queueMutex_;
	std::condition_variable queueCv_;
	std::deque<GstSample*> sampleQueue_;
    const std::size_t queueCapacity_{3};
    
    std::unordered_map<int, VideoCapture*> deviceCaptures_;
    std::mutex captureMutex_;
};

}