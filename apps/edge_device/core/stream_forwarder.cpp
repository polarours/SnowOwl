#include <chrono>
#include <ctime>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>

#include "core/stream_forwarder.hpp"
#include "protocol/message_types.hpp"

namespace SnowOwl::Edge::Core {

namespace {

template <typename T>
void writeLE(std::vector<std::uint8_t>& buffer, T value) {
	for (std::size_t i = 0; i < sizeof(T); ++i) {
		buffer.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF));
	}
}

}

StreamForwarder::StreamForwarder() = default;

StreamForwarder::~StreamForwarder() { stop();}

void StreamForwarder::configure(const ForwarderConfig& config) {
	config_ = config;
	sentHandshake_ = false;
}

bool StreamForwarder::start(StreamCapture* capture) {
	if (!config_.enabled) {
		return false;
	}

	if (running_.load()) {
		return true;
	}

	if (capture == nullptr) {
		std::cerr << "StreamForwarder: capture pointer is null" << std::endl;
		return false;
	}

	capture_ = capture;
	running_ = true;

	try {
		thread_ = std::thread(&StreamForwarder::forwardLoop, this);
	} catch (const std::exception& ex) {
		std::cerr << "StreamForwarder: failed to start thread - " << ex.what() << std::endl;
		running_ = false;
		return false;
	}

	return true;
}

void StreamForwarder::stop() {
	if (!running_.exchange(false)) {
		return;
	}

	if (thread_.joinable()) {
		thread_.join();
	}

	std::lock_guard<std::mutex> lock(connectionMutex_);
	if (socket_) {
		boost::system::error_code ec;
		if (socket_->is_open()) {
			socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		}
		socket_->close(ec);
		socket_.reset();
	}

	if (ioContext_) {
		ioContext_->stop();
	}

	ioContext_.reset();
	sentHandshake_ = false;
}

bool StreamForwarder::ensureConnected() {
	std::lock_guard<std::mutex> lock(connectionMutex_);
	if (socket_ && socket_->is_open()) {
		return true;
	}

	try {
		if (!ioContext_) {
			ioContext_ = std::make_unique<boost::asio::io_context>();
		}

		auto socket = std::make_unique<boost::asio::ip::tcp::socket>(*ioContext_);
		boost::asio::ip::tcp::resolver resolver(*ioContext_);
		boost::asio::ip::tcp::resolver::results_type endpoints = 
			resolver.resolve(config_.host, std::to_string(config_.port));

		boost::asio::connect(*socket, endpoints);
		socket_ = std::move(socket);
		sentHandshake_ = false;
		std::cout << "StreamForwarder: connected to " << config_.host << ':' << config_.port << std::endl;

		if (!config_.deviceId.empty() && !sentHandshake_) {
			nlohmann::json payload;
			payload["device_id"] = config_.deviceId;
			if (!config_.deviceName.empty()) {
				payload["device_name"] = config_.deviceName;
			}
			const auto now = std::chrono::system_clock::now();
			const std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
			std::tm utcTime{};
#if defined(_WIN32)
			gmtime_s(&utcTime, &timeValue);
#else
			gmtime_r(&timeValue, &utcTime);
#endif
			char buffer[32];
			if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utcTime) != 0) {
				payload["connected_at"] = buffer;
			}

			const std::string serialized = payload.dump();
			std::vector<std::uint8_t> controlBuffer;
			controlBuffer.reserve(serialized.size() + 5);
			controlBuffer.push_back(static_cast<std::uint8_t>(SnowOwl::Protocol::MessageType::Control));
			writeLE<std::uint32_t>(controlBuffer, static_cast<std::uint32_t>(serialized.size()));
			controlBuffer.insert(controlBuffer.end(), serialized.begin(), serialized.end());

			boost::system::error_code ec;
			boost::asio::write(*socket_, boost::asio::buffer(controlBuffer), ec);
			if (ec) {
				std::cerr << "StreamForwarder: failed to send handshake - " << ec.message() << std::endl;
				socket_->close();
				socket_.reset();
				return false;
			}
			sentHandshake_ = true;
		}
		return true;
	} catch (const std::exception& ex) {
		std::cerr << "StreamForwarder: connection failed - " << ex.what() << std::endl;
		socket_.reset();
		return false;
	}
}

void StreamForwarder::forwardLoop() {
	while (running_.load()) {
		if (!ensureConnected()) {
			std::this_thread::sleep_for(config_.reconnectDelay);
			continue;
		}

		if (!capture_) {
			std::this_thread::sleep_for(config_.frameInterval);
			continue;
		}

		cv::Mat frame = capture_->latestFrame();
		if (!frame.empty()) {
			if (!sendFrame(frame)) {
				std::lock_guard<std::mutex> lock(connectionMutex_);
				if (socket_) {
					boost::system::error_code ec;
					socket_->close(ec);
				}
			}
		}

		std::this_thread::sleep_for(config_.frameInterval);
	}
}

std::vector<std::uint8_t> StreamForwarder::encodeFrame(const cv::Mat& frame) const {
	std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
	std::vector<unsigned char> jpegBuffer;
	if (!cv::imencode(".jpg", frame, jpegBuffer, params)) {
		return {};
	}

	std::vector<std::uint8_t> buffer;
	buffer.reserve(jpegBuffer.size() + 5);
	buffer.push_back(static_cast<std::uint8_t>(SnowOwl::Protocol::MessageType::Frame));
	writeLE<std::uint32_t>(buffer, static_cast<std::uint32_t>(jpegBuffer.size()));
	buffer.insert(buffer.end(), jpegBuffer.begin(), jpegBuffer.end());
	return buffer;
}

bool StreamForwarder::sendFrame(const cv::Mat& frame) {
	const auto payload = encodeFrame(frame);

	std::lock_guard<std::mutex> lock(connectionMutex_);
	if (!socket_ || !socket_->is_open()) {
		return false;
	}

	boost::system::error_code ec;
	boost::asio::write(*socket_, boost::asio::buffer(payload), ec);
	return !ec.failed();
}

}
