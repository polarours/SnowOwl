#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio.hpp>

namespace SnowOwl::Edge::Network {

enum class ConnectionState {
	Disconnected,
	Connecting,
	Connected,
	Error
};

struct ConnectionSettings {
	std::string host{"127.0.0.1"};
	std::uint16_t port{7500};
	std::chrono::milliseconds timeout{std::chrono::milliseconds(3000)};
	std::chrono::milliseconds reconnectDelay{std::chrono::milliseconds(2000)};
};

class ConnectionManager {
public:
	ConnectionManager();
	~ConnectionManager();

	void configure(const ConnectionSettings& settings);
	ConnectionSettings settings() const;

	bool connect();
	void disconnect();

	bool isConnected() const;
	ConnectionState state() const;
	std::string lastError() const;

	bool send(const std::vector<std::uint8_t>& payload);
	bool send(const void* data, std::size_t size);

private:
	bool establishLocked();

	mutable std::mutex mutex_;
	ConnectionSettings settings_;
	std::atomic<ConnectionState> state_;
	std::string lastError_;

	std::unique_ptr<boost::asio::io_context> ioContext_;
	std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
};

}
