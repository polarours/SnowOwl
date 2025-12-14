#include "video_capture_manager.hpp"

#include <iostream>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

namespace SnowOwl::Server::Core {

static VideoCaptureManager* g_instance = nullptr;

namespace {
cv::Mat sampleToMat(GstSample* sample) {
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
}

VideoCaptureManager::VideoCaptureManager() {
    g_instance = this;
}

VideoCaptureManager::~VideoCaptureManager() {
	stop();
    g_instance = nullptr;
}


VideoCaptureManager* VideoCaptureManager::getInstance() {
    return g_instance;
}

bool VideoCaptureManager::start(const CaptureSourceConfig& config, SampleCallback sampleCallback, DetectionCallback detectionCallback) {
	if (running_.load()) {
		return false;
	}

	config_ = config;
	sampleCallback_ = std::move(sampleCallback);
    frameCallback_ = nullptr;
	detectionCallback_ = std::move(detectionCallback);

	capture_ = std::make_unique<VideoCapture>(
		nullptr, 
		config_.kind, 
		config_.cameraId, 
		config_.primaryUri, 
		config_.secondaryUri
	);

	if (!capture_->startVideoCaptureSystem()) {
		capture_.reset();
		return false;
	}

	running_ = true;
	captureActive_ = true;

	captureThread_ = std::thread(&VideoCaptureManager::captureLoop, this);
	processingThread_ = std::thread(&VideoCaptureManager::processingLoop, this);

	return true;
}

bool VideoCaptureManager::start(const CaptureSourceConfig& config, FrameCallback frameCallback, DetectionCallback detectionCallback) {
	if (running_.load()) {
		return false;
	}

	config_ = config;
	sampleCallback_ = nullptr;
    frameCallback_ = std::move(frameCallback);
	detectionCallback_ = std::move(detectionCallback);

	capture_ = std::make_unique<VideoCapture>(
		nullptr, 
		config_.kind, 
		config_.cameraId, 
		config_.primaryUri, 
		config_.secondaryUri
	);

	if (!capture_->startVideoCaptureSystem()) {
		capture_.reset();
		return false;
	}

	running_ = true;
	captureActive_ = true;

	captureThread_ = std::thread(&VideoCaptureManager::captureLoop, this);
	processingThread_ = std::thread(&VideoCaptureManager::processingLoop, this);

	return true;
}

bool VideoCaptureManager::restart(const CaptureSourceConfig& config) {
	stop();
	return start(config, sampleCallback_, detectionCallback_);
}

void VideoCaptureManager::stop() {
	running_ = false;
	captureActive_ = false;

	if (capture_) {
		capture_->stopVideoCaptureSystem();
		capture_.reset();
	}

	clearQueue();

	if (captureThread_.joinable()) {
		captureThread_.join();
	}

	if (processingThread_.joinable()) {
		processingThread_.join();
	}
}

void VideoCaptureManager::captureLoop() {
	while (captureActive_.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

void VideoCaptureManager::processingLoop() {
	while (running_.load()) {
		GstSample* sample = nullptr;
		
		{
			std::unique_lock<std::mutex> lock(queueMutex_);
			queueCv_.wait(lock, [this] { return !sampleQueue_.empty() || !running_.load(); });
			
			if (!running_.load() || sampleQueue_.empty()) {
				continue;
			}
			
			sample = sampleQueue_.front();
			sampleQueue_.pop_front();
		}
		
		if (sample) {
			cv::Mat frame = sampleToMat(sample);
			
			if (sampleCallback_) {
				sampleCallback_(sample);
			}
			
			if (!frame.empty()) {
				auto detections = processor_.processFrame(frame);
                
                if (frameCallback_) {
                    VideoProcessor::drawDetections(frame, detections);
                    frameCallback_(frame);
                }

				if (detectionCallback_ && !detections.empty()) {
					detectionCallback_(detections);
				}
			}
			
			gst_sample_unref(sample);
		}
	}
}

void VideoCaptureManager::clearQueue() {
	std::lock_guard<std::mutex> lock(queueMutex_);
	for (auto* sample : sampleQueue_) {
		gst_sample_unref(sample);
	}
	sampleQueue_.clear();
}


void VideoCaptureManager::addVideoCapture(int deviceId, VideoCapture* capture) {
    std::lock_guard<std::mutex> lock(captureMutex_);
    deviceCaptures_[deviceId] = capture;
}

void VideoCaptureManager::removeVideoCapture(int deviceId) {
    std::lock_guard<std::mutex> lock(captureMutex_);
    deviceCaptures_.erase(deviceId);
}

VideoCapture* VideoCaptureManager::getVideoCapture(int deviceId) {
    std::lock_guard<std::mutex> lock(captureMutex_);
    auto it = deviceCaptures_.find(deviceId);
    if (it != deviceCaptures_.end()) {
        return it->second;
    }
    return nullptr;
}

}