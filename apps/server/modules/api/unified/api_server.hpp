#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace SnowOwl::Config {
class DeviceRegistry;
}

namespace SnowOwl::Server::Core {
class VideoProcessor;
}

namespace SnowOwl::Server::Modules::Media {
struct MediaMTXConfig;
}

namespace SnowOwl::Server::Modules::Api::Rest {
class RestServer;
}

namespace SnowOwl::Server::Modules::Api::Websocket {
class WebsocketServer;
}

namespace SnowOwl::Server::Modules::Api::Unified {

class ApiServer {
public:
    ApiServer(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port);
    ~ApiServer();

    ApiServer(const ApiServer&) = delete;
    ApiServer& operator=(const ApiServer&) = delete;

    bool start();
    void stop();
    
    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor);
    void setMediaMTXConfig(const SnowOwl::Server::Modules::Media::MediaMTXConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}