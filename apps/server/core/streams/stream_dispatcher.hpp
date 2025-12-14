#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

#include "detection/detection_types.hpp"

namespace SnowOwl::Server::Core {

class StreamOutput {
public:
	virtual ~StreamOutput() = default;
	virtual bool start() = 0;
	virtual void stop() = 0;
	virtual void publishFrame(const cv::Mat& frame) = 0;
	virtual void publishEvents(const std::vector<Detection::DetectionResult>& events) = 0;
};

struct StreamOutputConfig {
	bool enabled{false};
	std::unordered_map<std::string, std::string> parameters;
};

struct StreamTargetProfile {
	StreamOutputConfig tcp;
	StreamOutputConfig rtmp;
	StreamOutputConfig rtsp;
	StreamOutputConfig hls;
	StreamOutputConfig webrtc;
};

inline bool hasAnyEnabled(const StreamTargetProfile& profile) {
	return profile.tcp.enabled || profile.rtmp.enabled || profile.rtsp.enabled || profile.hls.enabled || profile.webrtc.enabled;
}

class StreamDispatcher {
public:
	StreamDispatcher();
	~StreamDispatcher();

	StreamDispatcher(const StreamDispatcher&) = delete;
	StreamDispatcher& operator=(const StreamDispatcher&) = delete;
	StreamDispatcher(StreamDispatcher&&) = delete;
	StreamDispatcher& operator=(StreamDispatcher&&) = delete;

	void configure(const StreamTargetProfile& profile);
	bool startOutputs();
	void stopOutputs();

	void onFrame(const cv::Mat& frame);
	void onEvents(const std::vector<Detection::DetectionResult>& events);

private:
	std::vector<std::unique_ptr<StreamOutput>> outputs_;
	StreamTargetProfile profile_;
	bool started_{false};
};

}