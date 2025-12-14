#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace SnowOwl::Config {
class DeviceRegistry;
}

namespace SnowOwl::Server::Core {
class VideoProcessor;
}

namespace SnowOwl::Server::Modules::Media {
struct MediaMTXConfig;
}

namespace SnowOwl::Server::Modules::Api::Websocket {

class WebsocketServer {
public:
    WebsocketServer(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port);
    ~WebsocketServer();

    WebsocketServer(const WebsocketServer&) = delete;
    WebsocketServer& operator=(const WebsocketServer&) = delete;

    bool start();
    void stop();
    
    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor);
    void setMediaMTXConfig(const SnowOwl::Server::Modules::Media::MediaMTXConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}