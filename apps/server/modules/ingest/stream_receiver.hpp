#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>

#include "protocol/message_types.hpp"

namespace SnowOwl::Modules::Ingest {

struct ReceivedFrame {
    cv::Mat frame;
    std::string deviceId;
    std::uint64_t sequence{0};
    std::chrono::steady_clock::time_point timestamp{};
};

class StreamReceiver {
public:
    StreamReceiver();
    ~StreamReceiver();

    bool start(std::uint16_t port);
    void stop();

    bool latestFrame(ReceivedFrame& out) const;
    std::vector<std::string> connectedDevices() const;

private:
    struct ClientContext {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket;
        std::thread worker;
        std::string deviceId;
        std::atomic<bool> running{true};
    };

    struct Message {
        SnowOwl::Protocol::MessageType type{SnowOwl::Protocol::MessageType::Control};
        std::vector<std::uint8_t> payload;
    };

    void acceptLoop();
    void handleClient(const std::shared_ptr<ClientContext>& context);
    bool readMessage(boost::asio::ip::tcp::socket& socket, Message& message) const;
    void processFrame(const std::string& deviceId, const std::vector<std::uint8_t>& payload);
    void handleControl(const std::shared_ptr<ClientContext>& context, const std::vector<std::uint8_t>& payload);
    void cleanupClient(const std::shared_ptr<ClientContext>& context);

    std::unique_ptr<boost::asio::io_context> ioContext_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::thread acceptThread_;
    std::atomic<bool> running_{false};

    mutable std::mutex clientsMutex_;
    std::vector<std::shared_ptr<ClientContext>> clients_;

    mutable std::mutex frameMutex_;
    ReceivedFrame lastFrame_;
    std::uint64_t sequence_{0};
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> deviceLastSeen_;
};

}
