#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace boost::asio {
class io_context;
}

namespace SnowOwl::Config {
struct DeviceRecord;
class DeviceRegistry;
}

namespace SnowOwl::Server::Core {
class VideoProcessor;
}

namespace SnowOwl::Server::Modules::Media {
struct MediaMTXConfig;
}

namespace SnowOwl::Server::Modules::Api::Rest {

class RestServer {
public:
    RestServer(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port);
    ~RestServer();

    RestServer(const RestServer&) = delete;
    RestServer& operator=(const RestServer&) = delete;

    bool start();
    void stop();
    
    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor);
    
    void setMediaMTXConfig(const SnowOwl::Server::Modules::Media::MediaMTXConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}