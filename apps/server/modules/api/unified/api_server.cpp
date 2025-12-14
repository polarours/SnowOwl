#include "api_server.hpp"

#include <iostream>
#include <memory>

#include "common/config/device_registry.hpp"
#include "core/streams/video_processor.hpp"
#include "modules/api/rest/rest_server.hpp"
#include "modules/api/websocket/websocket_server.hpp"

namespace SnowOwl::Server::Modules::Api::Unified { 

class ApiServer::Impl {
public:
    Impl(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port)
        : registry_(registry)
        , port_(port)
        , videoProcessor_(nullptr)
    {
    }

    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
        videoProcessor_ = processor;
        
        if (restServer_) {
            restServer_->setVideoProcessor(processor);
        }
        
        if (websocketServer_) {
            websocketServer_->setVideoProcessor(processor);
        }
    }

    bool start() {
        restServer_ = std::make_unique<SnowOwl::Server::Modules::Api::Rest::RestServer>(registry_, port_);
        if (videoProcessor_) {
            restServer_->setVideoProcessor(videoProcessor_);
        }
        
        if (!restServer_->start()) {
            std::cerr << "  ⚠️  Warning: Failed to start REST API on port " << port_ << std::endl;
            restServer_.reset();
            return false;
        } else {
            std::cout << "     🌐 REST API listening on port " << port_ << std::endl;
        }

        websocketServer_ = std::make_unique<SnowOwl::Server::Modules::Api::Websocket::WebsocketServer>(registry_, port_ + 1);
        if (videoProcessor_) {
            websocketServer_->setVideoProcessor(videoProcessor_);
        }
        
        if (!websocketServer_->start()) {
            std::cerr << "  ⚠️  Warning: Failed to start WebSocket server on port " << (port_ + 1) << std::endl;
            websocketServer_.reset();
        } else {
            std::cout << "     🌐 WebSocket server listening on port " << (port_ + 1) << std::endl;
        }

        return true;
    }

    void stop() {
        if (restServer_) {
            restServer_->stop();
            restServer_.reset();
        }
        
        if (websocketServer_) {
            websocketServer_->stop();
            websocketServer_.reset();
        }
    }

private:
    SnowOwl::Config::DeviceRegistry& registry_;
    std::uint16_t port_;
    SnowOwl::Server::Core::VideoProcessor* videoProcessor_;
    
    std::unique_ptr<SnowOwl::Server::Modules::Api::Rest::RestServer> restServer_;
    std::unique_ptr<SnowOwl::Server::Modules::Api::Websocket::WebsocketServer> websocketServer_;
};

ApiServer::ApiServer(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port)
    : impl_(std::make_unique<Impl>(registry, port)) {
}

ApiServer::~ApiServer() {
    stop();
}

bool ApiServer::start() {
    return impl_->start();
}

void ApiServer::stop() {
    if (impl_) {
        impl_->stop();
    }
}

void ApiServer::setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
    if (impl_) {
        impl_->setVideoProcessor(processor);
    }
}

void ApiServer::setMediaMTXConfig(const SnowOwl::Server::Modules::Media::MediaMTXConfig&) {
}

}