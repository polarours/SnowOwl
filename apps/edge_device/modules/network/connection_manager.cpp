#include "modules/network/connection_manager.hpp"

#include <iostream>

namespace SnowOwl::Edge::Network {

ConnectionManager::ConnectionManager()
	: state_(ConnectionState::Disconnected) {
}

ConnectionManager::~ConnectionManager() {
	disconnect();
}

void ConnectionManager::configure(const ConnectionSettings& settings) {
	std::lock_guard<std::mutex> lock(mutex_);
	settings_ = settings;
}

ConnectionSettings ConnectionManager::settings() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return settings_;
}

bool ConnectionManager::connect() {
	std::lock_guard<std::mutex> lock(mutex_);
	return establishLocked();
}

void ConnectionManager::disconnect() {
	std::unique_lock<std::mutex> lock(mutex_);

	if (socket_ && socket_->is_open()) {
		boost::system::error_code ec;
		socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_->close(ec);
	}

	socket_.reset();
	ioContext_.reset();
	state_ = ConnectionState::Disconnected;
}

bool ConnectionManager::isConnected() const {
	return state_.load() == ConnectionState::Connected;
}

ConnectionState ConnectionManager::state() const {
	return state_.load();
}

std::string ConnectionManager::lastError() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return lastError_;
}

bool ConnectionManager::send(const std::vector<std::uint8_t>& payload) {
	return send(payload.data(), payload.size());
}

bool ConnectionManager::send(const void* data, std::size_t size) {
	std::unique_lock<std::mutex> lock(mutex_);

	if (!establishLocked()) {
		return false;
	}

	boost::system::error_code ec;
	boost::asio::write(*socket_, boost::asio::buffer(data, size), ec);
	if (ec) {
		lastError_ = ec.message();
		state_ = ConnectionState::Error;
		std::cerr << "ConnectionManager: send failed - " << ec.message() << std::endl;
		return false;
	}

	return true;
}

bool ConnectionManager::establishLocked() {
	if (socket_ && socket_->is_open()) {
		state_ = ConnectionState::Connected;
		return true;
	}

	try {
		if (!ioContext_) {
			ioContext_ = std::make_unique<boost::asio::io_context>();
		}

		auto socket = std::make_unique<boost::asio::ip::tcp::socket>(*ioContext_);
		boost::asio::ip::tcp::resolver resolver(*ioContext_);
		auto endpoints = resolver.resolve(settings_.host, std::to_string(settings_.port));

		state_ = ConnectionState::Connecting;
		boost::asio::connect(*socket, endpoints);

		socket_ = std::move(socket);
		lastError_.clear();
		state_ = ConnectionState::Connected;
		return true;
	} catch (const std::exception& ex) {
		lastError_ = ex.what();
		socket_.reset();
		state_ = ConnectionState::Error;
		std::cerr << "ConnectionManager: connect failed - " << ex.what() << std::endl;
		return false;
	}
}

}
