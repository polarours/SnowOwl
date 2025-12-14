#include "modules/ingest/stream_receiver.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <nlohmann/json.hpp>

namespace SnowOwl::Modules::Ingest {

namespace {

std::string safeDeviceId(const nlohmann::json& payload)
{
    if (payload.contains("device_id") && payload["device_id"].is_string()) {
        return payload["device_id"].get<std::string>();
    }
    return "unknown";
}

}

StreamReceiver::StreamReceiver() = default;

StreamReceiver::~StreamReceiver()
{
    stop();
}

bool StreamReceiver::start(std::uint16_t port)
{
    if (running_.load()) {
        return true;
    }

    try {
        ioContext_ = std::make_unique<boost::asio::io_context>();
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            *ioContext_,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

        running_ = true;
        acceptThread_ = std::thread([this]() {
            try {
                acceptLoop();
                ioContext_->run();
            } catch (const std::exception& ex) {
                std::cerr << "StreamReceiver accept loop error: " << ex.what() << std::endl;
            }
        });
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "StreamReceiver failed to start: " << ex.what() << std::endl;
        stop();
        return false;
    }
}

void StreamReceiver::stop()
{
    if (!running_.exchange(false)) {
        return;
    }

    if (acceptor_) {
        boost::system::error_code ec;
        acceptor_->close(ec);
    }

    if (ioContext_) {
        ioContext_->stop();
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& client : clients_) {
            client->running = false;
            if (client->socket && client->socket->is_open()) {
                boost::system::error_code ec;
                client->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                client->socket->close(ec);
            }
        }
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& client : clients_) {
            if (client->worker.joinable()) {
                client->worker.join();
            }
        }
        clients_.clear();
    }

    acceptor_.reset();
    ioContext_.reset();
}

bool StreamReceiver::latestFrame(ReceivedFrame& out) const
{
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (lastFrame_.frame.empty()) {
        return false;
    }
    out = lastFrame_;
    return true;
}

std::vector<std::string> StreamReceiver::connectedDevices() const
{
    std::vector<std::string> devices;
    std::lock_guard<std::mutex> lock(clientsMutex_);
    devices.reserve(clients_.size());
    for (const auto& client : clients_) {
        if (!client->deviceId.empty()) {
            devices.push_back(client->deviceId);
        }
    }
    return devices;
}

void StreamReceiver::acceptLoop()
{
    while (running_.load()) {
        auto context = std::make_shared<ClientContext>();
        context->socket = std::make_shared<boost::asio::ip::tcp::socket>(*ioContext_);

        boost::system::error_code ec;
        acceptor_->accept(*context->socket, ec);
        if (ec) {
            if (running_.load()) {
                std::cerr << "StreamReceiver accept failed: " << ec.message() << std::endl;
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_.push_back(context);
        }

        context->worker = std::thread([this, context]() {
            handleClient(context);
        });
    }
}

void StreamReceiver::handleClient(const std::shared_ptr<ClientContext>& context)
{
    auto& socket = *context->socket;
    std::cout << "StreamReceiver: client from " << socket.remote_endpoint() << std::endl;

    while (context->running.load()) {
        Message message;
        if (!readMessage(socket, message)) {
            break;
        }

        switch (message.type) {
        case SnowOwl::Protocol::MessageType::Frame:
            processFrame(context->deviceId, message.payload);
            break;
        case SnowOwl::Protocol::MessageType::Control:
            handleControl(context, message.payload);
            break;
        default:
            break;
        }
    }

    cleanupClient(context);
}

bool StreamReceiver::readMessage(boost::asio::ip::tcp::socket& socket, Message& message) const
{
    boost::system::error_code ec;
    std::array<std::uint8_t, 5> header{};
    boost::asio::read(socket, boost::asio::buffer(header), ec);
    if (ec) {
        return false;
    }

    message.type = static_cast<SnowOwl::Protocol::MessageType>(header[0]);
    const std::uint32_t length =
        header[1] | (static_cast<std::uint32_t>(header[2]) << 8) |
        (static_cast<std::uint32_t>(header[3]) << 16) |
        (static_cast<std::uint32_t>(header[4]) << 24);

    message.payload.resize(length);
    boost::asio::read(socket, boost::asio::buffer(message.payload), ec);
    if (ec) {
        return false;
    }
    return true;
}

void StreamReceiver::processFrame(const std::string& deviceId, const std::vector<std::uint8_t>& payload)
{
    if (payload.empty()) {
        return;
    }

    cv::Mat jpegMat(1, static_cast<int>(payload.size()), CV_8UC1, const_cast<std::uint8_t*>(payload.data()));
    cv::Mat frame = cv::imdecode(jpegMat, cv::IMREAD_COLOR);
    if (frame.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(frameMutex_);
    lastFrame_.frame = std::move(frame);
    lastFrame_.deviceId = deviceId.empty() ? "unknown" : deviceId;
    lastFrame_.sequence = ++sequence_;
    lastFrame_.timestamp = std::chrono::steady_clock::now();
}

void StreamReceiver::handleControl(const std::shared_ptr<ClientContext>& context, const std::vector<std::uint8_t>& payload)
{
    try {
        const auto json = nlohmann::json::parse(payload.begin(), payload.end());
        const std::string deviceId = safeDeviceId(json);
        context->deviceId = deviceId;

        if (json.contains("device_name") && json["device_name"].is_string()) {
            std::cout << "StreamReceiver: handshake from " << deviceId
                      << " (" << json["device_name"].get<std::string>() << ")" << std::endl;
        } else {
            std::cout << "StreamReceiver: handshake from " << deviceId << std::endl;
        }

        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            deviceLastSeen_[deviceId] = std::chrono::steady_clock::now();
        }
    } catch (const std::exception& ex) {
        std::cerr << "StreamReceiver: failed to parse control message: " << ex.what() << std::endl;
    }
}

void StreamReceiver::cleanupClient(const std::shared_ptr<ClientContext>& context)
{
    context->running = false;

    if (context->socket && context->socket->is_open()) {
        boost::system::error_code ec;
        context->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        context->socket->close(ec);
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = std::remove(clients_.begin(), clients_.end(), context);
        clients_.erase(it, clients_.end());
    }
}

}
