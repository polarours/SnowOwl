#include <iostream>

#include "core/output/rtmp_output.hpp"
#include "core/streams/stream_dispatcher.hpp"
#include "core/streams/video_capture_manager.hpp"
#include "core/output/rtsp_output.hpp"

namespace SnowOwl::Server::Core {

namespace {

class NullStreamOutput : public StreamOutput {
public:
	bool start() override { return true; }
	void stop() override {}
	void publishFrame(const cv::Mat&) override {}
	void publishEvents(const std::vector<Detection::DetectionResult>&) override {}
};

}

StreamDispatcher::StreamDispatcher() = default;

StreamDispatcher::~StreamDispatcher() {
	stopOutputs();
}

void StreamDispatcher::configure(const StreamTargetProfile& profile) {
	if (started_) {
		std::cerr << "StreamDispatcher: configure called while running" << std::endl;
		return;
	}

	profile_ = std::move(profile);
}

bool StreamDispatcher::startOutputs()
{
	if (started_) {
		return true;
	}

	outputs_.clear();

	if (profile_.tcp.enabled) {
		outputs_.push_back(std::make_unique<NullStreamOutput>());
	}
	if (profile_.rtmp.enabled) {
		outputs_.push_back(std::make_unique<RtmpOutput>(profile_.rtmp));
	}
	if (profile_.rtsp.enabled) {
		outputs_.push_back(std::make_unique<RtspOutput>(profile_.rtsp));
	}
	if (profile_.hls.enabled) {
		outputs_.push_back(std::make_unique<NullStreamOutput>());
	}
	if (profile_.webrtc.enabled) {
		outputs_.push_back(std::make_unique<NullStreamOutput>());
	}

	for (auto& output : outputs_) {
		if (!output->start()) {
			std::cerr << "StreamDispatcher: failed to start output" << std::endl;
			stopOutputs();
			return false;
		}
	}

	started_ = true;
	return true;
}

void StreamDispatcher::stopOutputs()
{
	if (!started_) {
		return;
	}

	for (auto& output : outputs_) {
		if (output) {
			output->stop();
		}
	}

	outputs_.clear();
	started_ = false;
}

void StreamDispatcher::onFrame(const cv::Mat& frame)
{
	if (!started_ || frame.empty()) {
		return;
	}

	for (auto& output : outputs_) {
		if (output) {
			output->publishFrame(frame);
		}
	}
}

void StreamDispatcher::onEvents(const std::vector<Detection::DetectionResult>& events)
{
	if (!started_) {
		return;
	}

	for (auto& output : outputs_) {
		if (output) {
			output->publishEvents(events);
		}
	}
}

}
