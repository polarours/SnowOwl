#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include <nlohmann/json.hpp>

#include "modules/network/connection_manager.hpp"

namespace SnowOwl::Edge::Network {

class ForwardClient {
public:
	explicit ForwardClient(ConnectionManager& manager);

	void setIdentity(std::string deviceId, std::string deviceName);

	bool ensureConnected();
	bool sendHandshake();
	bool sendFrame(const cv::Mat& frame, int quality = 80);
	bool sendControl(const nlohmann::json& payload);

	bool isConnected() const { return manager_.isConnected(); }

private:
	std::vector<std::uint8_t> encodeFrame(const cv::Mat& frame, int quality) const;
	std::vector<std::uint8_t> serializeControl(const nlohmann::json& payload) const;

	ConnectionManager& manager_;
	std::string deviceId_;
	std::string deviceName_;
	bool handshakeSent_{false};
};

}
