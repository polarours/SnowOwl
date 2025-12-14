#pragma once

#include <memory>
#include <string>

namespace SnowOwl::Config {
class DeviceRegistry;
}

namespace grpc {
class Server;
}

namespace SnowOwl::Server::Modules::Api::Grpc {

class GrpcServer {
public:
    GrpcServer(const std::string& address, SnowOwl::Config::DeviceRegistry& registry);
    ~GrpcServer();

    bool start();
    void stop();

private:
    class ServiceImpl;

    std::string address_;
    SnowOwl::Config::DeviceRegistry& registry_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<ServiceImpl> serviceImpl_;
};

}