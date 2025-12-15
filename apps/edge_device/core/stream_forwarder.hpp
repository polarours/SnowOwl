#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>

#include "core/stream_capture.hpp"

namespace SnowOwl::Edge::Core {

// Configuration structure for the StreamForwarder
// Defines parameters for connecting and sending frames to the server
// such as host, port, frame interval, and device identification.
struct ForwarderConfig {
	bool enabled{false};
	std::string host{"127.0.0.1"};
	std::uint16_t port{7500};
	std::chrono::milliseconds frameInterval{std::chrono::milliseconds(100)};
	std::chrono::milliseconds reconnectDelay{std::chrono::milliseconds(2000)};
	std::string deviceId;
	std::string deviceName;
};

class StreamForwarder {
public:
	StreamForwarder();
	~StreamForwarder();

	void configure(const ForwarderConfig& config);
	bool start(StreamCapture* capture);
	void stop();

	bool isRunning() const { return running_.load(); }

	bool sendAudioData(const std::vector<std::uint8_t>& audioData);

private:
	bool ensureConnected();
	void forwardLoop();
	bool sendFrame(const cv::Mat& frame);
	std::vector<std::uint8_t> encodeFrame(const cv::Mat& frame) const;
	std::vector<std::uint8_t> encodeAudioData(const std::vector<std::uint8_t>& audioData) const;

	ForwarderConfig config_{};
	StreamCapture* capture_{nullptr};

	mutable std::mutex connectionMutex_;
	std::unique_ptr<boost::asio::io_context> ioContext_;
	std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    bool sentHandshake_{false};

	std::thread thread_;
	std::atomic<bool> running_{false};
};

}
