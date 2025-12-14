#include "websocket_server.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "common/config/device_registry.hpp"
#include "core/streams/video_processor.hpp"
#include "modules/utils/server_utils.hpp"

namespace SnowOwl::Server::Modules::Api::Websocket { 

namespace {
    
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebsocketSession;
class WebsocketSession : public std::enable_shared_from_this<WebsocketSession> {
public:
    explicit WebsocketSession(tcp::socket socket, SnowOwl::Config::DeviceRegistry& registry)
        : websocket_(std::move(socket))
        , registry_(registry)
        , videoProcessor_(nullptr)
        , heartbeatTimer_(websocket_.get_executor())
    {
    }

    void run() {
        websocket_.async_accept(
            [self = shared_from_this()](beast::error_code ec) {
                if (!ec) {
                    self->onAccept();
                }
            });
    }

    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
        videoProcessor_ = processor;
    }

private:
    websocket::stream<tcp::socket> websocket_;
    beast::flat_buffer buffer_;
    SnowOwl::Config::DeviceRegistry& registry_;
    SnowOwl::Server::Core::VideoProcessor* videoProcessor_;
    net::steady_timer heartbeatTimer_;
    std::vector<std::string> subscriptions_;

    void onAccept() {
        nlohmann::json welcomeMsg;
        welcomeMsg["type"] = "welcome";
        welcomeMsg["message"] = "Connected to SnowOwl WebSocket API";
        welcomeMsg["version"] = "1.0";
        welcomeMsg["timestamp"] = std::time(nullptr);

        sendText(welcomeMsg.dump());

        startHeartbeat();

        doRead();
    }

    void startHeartbeat() {
        heartbeatTimer_.expires_after(std::chrono::seconds(30));
        heartbeatTimer_.async_wait([self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                nlohmann::json heartbeat;
                heartbeat["type"] = "heartbeat";
                heartbeat["timestamp"] = std::time(nullptr);
                self->sendText(heartbeat.dump());

                self->startHeartbeat();
            }
        });
    }

    void doRead() {
        websocket_.async_read(
            buffer_,
            [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->onRead(bytes_transferred);
                } else {
                    self->onClose();
                }
            });
    }

    void onRead(std::size_t bytes_transferred) {
        std::string message(static_cast<const char*>(buffer_.data().data()), bytes_transferred);
        buffer_.consume(bytes_transferred);

        processMessage(message);

        doRead();
    }

    void processMessage(const std::string& message) {
        try {
            auto jsonMsg = nlohmann::json::parse(message);
            
            std::string type = jsonMsg.value("type", "");
            
            if (type == "subscribe") {
                handleSubscribe(jsonMsg);
            } else if (type == "unsubscribe") {
                handleUnsubscribe(jsonMsg);
            } else if (type == "get_device_list") {
                handleGetDeviceList();
            } else if (type == "get_device_info") {
                handleGetDeviceInfo(jsonMsg);
            } else if (type == "get_stream_status") {
                handleGetStreamStatus(jsonMsg);
            } else if (type == "start_streaming") {
                handleStartStreaming(jsonMsg);
            } else if (type == "stop_streaming") {
                handleStopStreaming(jsonMsg);
            } else {
                nlohmann::json errorMsg;
                errorMsg["type"] = "error";
                errorMsg["message"] = "Unknown message type: " + type;
                sendText(errorMsg.dump());
            }
        } catch (const std::exception& e) {
            nlohmann::json errorMsg;
            errorMsg["type"] = "error";
            errorMsg["message"] = "Failed to parse message: " + std::string(e.what());
            sendText(errorMsg.dump());
        }
    }

    void handleSubscribe(const nlohmann::json& msg) {
        std::string topic = msg.value("topic", "");
        
        if (!topic.empty()) {
            if (std::find(subscriptions_.begin(), subscriptions_.end(), topic) == subscriptions_.end()) {
                subscriptions_.push_back(topic);
            }
            
            nlohmann::json response;
            response["type"] = "subscribed";
            response["topic"] = topic;
            response["message"] = "Successfully subscribed to topic";
            sendText(response.dump());
        } else {
            nlohmann::json errorMsg;
            errorMsg["type"] = "error";
            errorMsg["message"] = "Missing topic in subscribe message";
            sendText(errorMsg.dump());
        }
    }

    void handleUnsubscribe(const nlohmann::json& msg) {
        std::string topic = msg.value("topic", "");
        
        if (!topic.empty()) {
            subscriptions_.erase(
                std::remove(subscriptions_.begin(), subscriptions_.end(), topic),
                subscriptions_.end()
            );
            
            nlohmann::json response;
            response["type"] = "unsubscribed";
            response["topic"] = topic;
            response["message"] = "Successfully unsubscribed from topic";
            sendText(response.dump());
        } else {
            nlohmann::json errorMsg;
            errorMsg["type"] = "error";
            errorMsg["message"] = "Missing topic in unsubscribe message";
            sendText(errorMsg.dump());
        }
    }

    void handleGetDeviceList() {
        const auto devices = registry_.listDevices();
        nlohmann::json response;
        response["type"] = "device_list";
        response["devices"] = nlohmann::json::array();
        
        for (const auto& device : devices) {
            nlohmann::json deviceJson;
            deviceJson["id"] = device.id;
            deviceJson["name"] = device.name;
            deviceJson["kind"] = SnowOwl::Config::toString(device.kind);
            deviceJson["uri"] = device.uri;
            deviceJson["enabled"] = device.enabled;
            deviceJson["is_primary"] = device.isPrimary;
            response["devices"].push_back(deviceJson);
        }
        
        sendText(response.dump());
    }

    void handleGetDeviceInfo(const nlohmann::json& msg) {
        int deviceId = msg.value("device_id", -1);
        
        if (deviceId >= 0) {
            const auto device = registry_.findById(deviceId);
            
            if (device) {
                nlohmann::json response;
                response["type"] = "device_info";
                response["device"]["id"] = device->id;
                response["device"]["name"] = device->name;
                response["device"]["kind"] = SnowOwl::Config::toString(device->kind);
                response["device"]["uri"] = device->uri;
                response["device"]["enabled"] = device->enabled;
                response["device"]["is_primary"] = device->isPrimary;
                response["device"]["created_at"] = device->createdAt;
                response["device"]["updated_at"] = device->updatedAt;
                
                sendText(response.dump());
            } else {
                nlohmann::json errorMsg;
                errorMsg["type"] = "error";
                errorMsg["message"] = "Device not found";
                sendText(errorMsg.dump());
            }
        } else {
            nlohmann::json errorMsg;
            errorMsg["type"] = "error";
            errorMsg["message"] = "Missing or invalid device_id";
            sendText(errorMsg.dump());
        }
    }

    void handleGetStreamStatus(const nlohmann::json& msg) {
        int deviceId = msg.value("device_id", -1);
        
        nlohmann::json response;
        response["type"] = "stream_status";
        response["device_id"] = deviceId;
        
        if (videoProcessor_) {
            response["status"] = "active";
            response["bitrate"] = 4500;
            response["resolution"] = "1920x1080";
            response["fps"] = 30.0;
            response["codec"] = "H.264";
        } else {
            response["status"] = "unknown";
        }
        
        sendText(response.dump());
    }

    void handleStartStreaming(const nlohmann::json& msg) {
        int deviceId = msg.value("device_id", -1);
        std::string streamType = msg.value("stream_type", "live");
        
        nlohmann::json response;
        response["type"] = "streaming_started";
        response["device_id"] = deviceId;
        response["stream_type"] = streamType;
        response["message"] = "Streaming started for device ID: " + std::to_string(deviceId);
        
        std::string streamTopic = "stream_" + std::to_string(deviceId);
        if (std::find(subscriptions_.begin(), subscriptions_.end(), streamTopic) == subscriptions_.end()) {
            subscriptions_.push_back(streamTopic);
        }
        
        sendText(response.dump());
        
        sendStreamUpdates(deviceId);
    }

    void handleStopStreaming(const nlohmann::json& msg) {
        int deviceId = msg.value("device_id", -1);
        
        nlohmann::json response;
        response["type"] = "streaming_stopped";
        response["device_id"] = deviceId;
        response["message"] = "Streaming stopped for device ID: " + std::to_string(deviceId);
        
        std::string streamTopic = "stream_" + std::to_string(deviceId);
        subscriptions_.erase(
            std::remove(subscriptions_.begin(), subscriptions_.end(), streamTopic),
            subscriptions_.end()
        );
        
        sendText(response.dump());
    }

    void sendStreamUpdates(int deviceId) {
        std::string streamTopic = "stream_" + std::to_string(deviceId);
        if (std::find(subscriptions_.begin(), subscriptions_.end(), streamTopic) == subscriptions_.end()) {
            return;
        }
        
        nlohmann::json update;
        update["type"] = "stream_update";
        update["device_id"] = deviceId;
        update["timestamp"] = std::time(nullptr);
        
        static int frameCount = 0;
        update["frame_count"] = ++frameCount;
        update["bitrate"] = 4000 + (rand() % 1000);
        
        sendText(update.dump());
        
        auto timer = std::make_shared<net::steady_timer>(websocket_.get_executor());
        timer->expires_after(std::chrono::seconds(2));
        timer->async_wait([self = shared_from_this(), deviceId, timer](beast::error_code ec) {
            if (!ec) {
                self->sendStreamUpdates(deviceId);
            }
        });
    }

    void sendText(const std::string& text) {
        websocket_.async_write(
            net::buffer(text),
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
            });
    }

    void onClose() {
        heartbeatTimer_.cancel();
    }
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::io_context& ioc,
             tcp::endpoint endpoint,
             SnowOwl::Config::DeviceRegistry& registry)
        : ioc_(ioc)
        , acceptor_(ioc)
        , registry_(registry)
        , videoProcessor_(nullptr)
    {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (!ec) {
            acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        }
        if (!ec) {
            acceptor_.bind(endpoint, ec);
        }
        if (!ec) {
            acceptor_.listen(net::socket_base::max_listen_connections, ec);
        }
        if (ec) {
            throw std::runtime_error("WebsocketServer: cannot start listener: " + ec.message());
        }
    }

    void run() {
        doAccept();
    }

    void stop() {
        beast::error_code ec;
        acceptor_.close(ec);
    }

    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
        videoProcessor_ = processor;
    }

private:
    void doAccept() {
        auto self = shared_from_this();
        acceptor_.async_accept(
            [self](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    auto session = std::make_shared<WebsocketSession>(std::move(socket), self->registry_);
                    session->setVideoProcessor(self->videoProcessor_);
                    session->run();
                }
                if (self->acceptor_.is_open()) {
                    self->doAccept();
                }
            });
    }

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    SnowOwl::Config::DeviceRegistry& registry_;
    SnowOwl::Server::Core::VideoProcessor* videoProcessor_{nullptr};
};

}

class WebsocketServer::Impl {
public:
    Impl(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port)
        : registry_(registry)
        , port_(port)
        , videoProcessor_(nullptr)
    {
    }

    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
        videoProcessor_ = processor;
        if (listenerV6_) {
            listenerV6_->setVideoProcessor(processor);
        }
        if (listenerV4_) {
            listenerV4_->setVideoProcessor(processor);
        }
    }

    bool start() {
        if (running_) {
            return true;
        }

        try {
            ioContext_ = std::make_unique<net::io_context>();
            bool boundAny = false;

            try {
                auto v6Endpoint = tcp::endpoint(net::ip::tcp::v6(), port_);
                listenerV6_ = std::make_shared<Listener>(*ioContext_, v6Endpoint, registry_);
                listenerV6_->setVideoProcessor(videoProcessor_);
                listenerV6_->run();
                boundAny = true;
            } catch (const std::exception& e) {
                std::cerr << "WebsocketServer: IPv6 bind failed: " << e.what() << std::endl;
                listenerV6_.reset();
            }

            try {
                auto v4Endpoint = tcp::endpoint(net::ip::tcp::v4(), port_);
                listenerV4_ = std::make_shared<Listener>(*ioContext_, v4Endpoint, registry_);
                listenerV4_->setVideoProcessor(videoProcessor_);
                listenerV4_->run();
                boundAny = true;
            } catch (const std::exception& e) {
                std::cerr << "WebsocketServer: IPv4 bind failed: " << e.what() << std::endl;
                listenerV4_.reset();
            }

            if (!boundAny) {
                throw std::runtime_error("WebsocketServer: failed to bind on any interface");
            }

            running_ = true;
            thread_ = std::thread([this]() {
                try {
                    ioContext_->run();
                } catch (const std::exception& ex) {
                    std::cerr << "WebsocketServer: io_context error: " << ex.what() << std::endl;
                }
            });
        } catch (const std::exception& e) {
            std::cerr << "WebsocketServer: failed to start: " << e.what() << std::endl;
            stop();
            return false;
        }

        return true;
    }

    void stop() {
        if (!running_) {
            return;
        }

        running_ = false;

        if (listenerV6_) {
            listenerV6_->stop();
            listenerV6_.reset();
        }

        if (listenerV4_) {
            listenerV4_->stop();
            listenerV4_.reset();
        }

        if (ioContext_) {
            ioContext_->stop();
        }

        if (thread_.joinable()) {
            thread_.join();
        }

        ioContext_.reset();
    }

private:
    SnowOwl::Config::DeviceRegistry& registry_;
    std::uint16_t port_;
    SnowOwl::Server::Core::VideoProcessor* videoProcessor_;
    std::unique_ptr<net::io_context> ioContext_;
    std::shared_ptr<Listener> listenerV6_;
    std::shared_ptr<Listener> listenerV4_;
    std::thread thread_;
    bool running_{false};
};

WebsocketServer::WebsocketServer(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port)
    : impl_(std::make_unique<Impl>(registry, port)) {
}

WebsocketServer::~WebsocketServer() {
    stop();
}

bool WebsocketServer::start() {
    return impl_->start();
}

void WebsocketServer::stop() {
    if (impl_) {
        impl_->stop();
    }
}

void WebsocketServer::setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
    if (impl_) {
        impl_->setVideoProcessor(processor);
    }
}

void WebsocketServer::setMediaMTXConfig(const SnowOwl::Server::Modules::Media::MediaMTXConfig&) {
}

}
