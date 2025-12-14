#include "modules/network/forward_client.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "protocol/message_types.hpp"

namespace SnowOwl::Edge::Network {

namespace {

template <typename T>
void writeLE(std::vector<std::uint8_t>& buffer, T value) {
	for (std::size_t i = 0; i < sizeof(T); ++i) {
		buffer.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF));
	}
}

std::string utcNow() {
	const auto now = std::chrono::system_clock::now();
	const std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
	std::tm utcTime{};
#if defined(_WIN32)
	gmtime_s(&utcTime, &timeValue);
#else
	gmtime_r(&timeValue, &utcTime);
#endif
	std::ostringstream oss;
	oss << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
	return oss.str();
}

}

ForwardClient::ForwardClient(ConnectionManager& manager)
	: manager_(manager) {}

void ForwardClient::setIdentity(std::string deviceId, std::string deviceName) {
	deviceId_ = std::move(deviceId);
	deviceName_ = std::move(deviceName);
	handshakeSent_ = false;
}

bool ForwardClient::ensureConnected() {
	if (manager_.isConnected()) {
		return true;
	}
	handshakeSent_ = false;
	return manager_.connect();
}

bool ForwardClient::sendHandshake() {
	if (!ensureConnected()) {
		return false;
	}

	nlohmann::json payload = nlohmann::json::object();
	if (!deviceId_.empty()) {
		payload["device_id"] = deviceId_;
	}
	if (!deviceName_.empty()) {
		payload["device_name"] = deviceName_;
	}
	payload["connected_at"] = utcNow();

	const auto buffer = serializeControl(payload);
	if (!manager_.send(buffer)) {
		return false;
	}

	handshakeSent_ = true;
	return true;
}

bool ForwardClient::sendFrame(const cv::Mat& frame, int quality) {
	if (frame.empty()) {
		return false;
	}

	if (!ensureConnected()) {
		return false;
	}

	if (!handshakeSent_ && !sendHandshake()) {
		return false;
	}

	const auto payload = encodeFrame(frame, quality);
	if (payload.empty()) {

		return false;
	}

	return manager_.send(payload);
}

bool ForwardClient::sendControl(const nlohmann::json& payload) {
	if (!ensureConnected()) {
		return false;
	}

	return manager_.send(serializeControl(payload));
}

std::vector<std::uint8_t> ForwardClient::encodeFrame(const cv::Mat& frame, int quality) const {
	std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
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

std::vector<std::uint8_t> ForwardClient::serializeControl(const nlohmann::json& payload) const {
	const std::string serialized = payload.dump();

	std::vector<std::uint8_t> buffer;
	buffer.reserve(serialized.size() + 5);
	buffer.push_back(static_cast<std::uint8_t>(SnowOwl::Protocol::MessageType::Control));
	writeLE<std::uint32_t>(buffer, static_cast<std::uint32_t>(serialized.size()));
	buffer.insert(buffer.end(), serialized.begin(), serialized.end());
	return buffer;
}

}
