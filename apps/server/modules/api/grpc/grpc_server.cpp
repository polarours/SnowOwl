#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>

#include "grpc_server.hpp"
#include "config/device_registry.hpp"
#include "snowowl.grpc.pb.h"

namespace SnowOwl::Server::Modules::Api::Grpc {

class GrpcServer::ServiceImpl {
public:
    explicit ServiceImpl(SnowOwl::Config::DeviceRegistry& registry) 
        : registry_(registry),
          deviceService_(std::make_unique<DeviceServiceImpl>(registry)),
          streamService_(std::make_unique<StreamServiceImpl>(registry)),
          monitoringService_(std::make_unique<MonitoringServiceImpl>(registry)) {
    }
    
    snowowl::DeviceService::Service* deviceService() { return deviceService_.get(); }
    snowowl::StreamService::Service* streamService() { return streamService_.get(); }
    snowowl::MonitoringService::Service* monitoringService() { return monitoringService_.get(); }
    
private:
    SnowOwl::Config::DeviceRegistry& registry_;
    
    class DeviceServiceImpl final : public snowowl::DeviceService::Service {
    public:
        explicit DeviceServiceImpl(SnowOwl::Config::DeviceRegistry& registry) 
            : registry_(registry) 
            {

            }
            
        grpc::Status ListDevices(grpc::ServerContext* context, 
                                 const snowowl::ListDevicesRequest* request,
                                 snowowl::ListDevicesResponse* response) override {
            try {
                const auto devices = registry_.listDevices();
                
                for (const auto& device : devices) {
                    auto* deviceProto = response->add_devices();
                    deviceProto->set_id(device.id);
                    deviceProto->set_name(device.name);
                    deviceProto->set_kind(static_cast<snowowl::DeviceKind>(device.kind));
                    deviceProto->set_uri(device.uri);
                    deviceProto->set_is_primary(device.isPrimary);
                    deviceProto->set_enabled(device.enabled);
                    deviceProto->set_metadata(device.metadata);
                    deviceProto->set_created_at(device.createdAt);
                    deviceProto->set_updated_at(device.updatedAt);
                }
                
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, 
                                    std::string("Failed to list devices: ") + e.what());
            }
        }
        
        grpc::Status GetDevice(grpc::ServerContext* context,
                               const snowowl::GetDeviceRequest* request,
                               snowowl::GetDeviceResponse* response) override {
            try {
                const auto device = registry_.findById(request->id());
                
                if (!device) {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
                }
                
                auto* deviceProto = response->mutable_device();
                deviceProto->set_id(device->id);
                deviceProto->set_name(device->name);
                deviceProto->set_kind(static_cast<snowowl::DeviceKind>(device->kind));
                deviceProto->set_uri(device->uri);
                deviceProto->set_is_primary(device->isPrimary);
                deviceProto->set_enabled(device->enabled);
                deviceProto->set_metadata(device->metadata);
                deviceProto->set_created_at(device->createdAt);
                deviceProto->set_updated_at(device->updatedAt);
                
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, 
                                    std::string("Failed to get device: ") + e.what());
            }
        }
        
        grpc::Status CreateDevice(grpc::ServerContext* context,
                                  const snowowl::CreateDeviceRequest* request,
                                  snowowl::CreateDeviceResponse* response) override {
            try {
                const auto& deviceProto = request->device();
                
                SnowOwl::Config::DeviceRecord device;
                device.name = deviceProto.name();
                device.kind = static_cast<SnowOwl::Config::DeviceKind>(deviceProto.kind());
                device.uri = deviceProto.uri();
                device.isPrimary = deviceProto.is_primary();
                device.enabled = deviceProto.enabled();
                device.metadata = deviceProto.metadata();
                
                const auto result = registry_.upsertDevice(device);
                
                auto* deviceResponse = response->mutable_device();
                deviceResponse->set_id(result.id);
                deviceResponse->set_name(result.name);
                deviceResponse->set_kind(static_cast<snowowl::DeviceKind>(result.kind));
                deviceResponse->set_uri(result.uri);
                deviceResponse->set_is_primary(result.isPrimary);
                deviceResponse->set_enabled(result.enabled);
                deviceResponse->set_metadata(result.metadata);
                deviceResponse->set_created_at(result.createdAt);
                deviceResponse->set_updated_at(result.updatedAt);
                
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, 
                                    std::string("Failed to create device: ") + e.what());
            }
        }
        
        grpc::Status UpdateDevice(grpc::ServerContext* context,
                                  const snowowl::UpdateDeviceRequest* request,
                                  snowowl::UpdateDeviceResponse* response) override {
            try {
                const auto existingDevice = registry_.findById(request->id());
                
                if (!existingDevice) {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
                }
                
                SnowOwl::Config::DeviceRecord device = existingDevice.value();
                
                const auto& deviceProto = request->device();
                device.name = deviceProto.name();
                device.kind = static_cast<SnowOwl::Config::DeviceKind>(deviceProto.kind());
                device.uri = deviceProto.uri();
                device.isPrimary = deviceProto.is_primary();
                device.enabled = deviceProto.enabled();
                device.metadata = deviceProto.metadata();
                
                const auto result = registry_.upsertDevice(device);
                
                auto* deviceResponse = response->mutable_device();
                deviceResponse->set_id(result.id);
                deviceResponse->set_name(result.name);
                deviceResponse->set_kind(static_cast<snowowl::DeviceKind>(result.kind));
                deviceResponse->set_uri(result.uri);
                deviceResponse->set_is_primary(result.isPrimary);
                deviceResponse->set_enabled(result.enabled);
                deviceResponse->set_metadata(result.metadata);
                deviceResponse->set_created_at(result.createdAt);
                deviceResponse->set_updated_at(result.updatedAt);
                
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, 
                                    std::string("Failed to update device: ") + e.what());
            }
        }
        
        grpc::Status DeleteDevice(grpc::ServerContext* context,
                                  const snowowl::DeleteDeviceRequest* request,
                                  snowowl::DeleteDeviceResponse* response) override {
            try {
                const bool success = registry_.removeDevice(request->id());
                
                if (!success) {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
                }
                
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, 
                                    std::string("Failed to delete device: ") + e.what());
            }
        }
        
        grpc::Status StreamDeviceEvents(grpc::ServerContext* context,
                                        const snowowl::StreamDeviceEventsRequest* request,
                                        grpc::ServerWriter<snowowl::DeviceEvent>* writer) override {
            // For now, we'll return unimplemented. In a real implementation, this would
            // establish a stream and push device events to the client.
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Streaming device events not implemented");
        }
        
    private:
        SnowOwl::Config::DeviceRegistry& registry_;
    };
    
    // Stream service implementation
    class StreamServiceImpl final : public snowowl::StreamService::Service {
    public:
        explicit StreamServiceImpl(SnowOwl::Config::DeviceRegistry& registry) 
            : m_registry(registry) {}
            
        grpc::Status GetStreamInfo(grpc::ServerContext* context,
                                   const snowowl::GetStreamInfoRequest* request,
                                   snowowl::GetStreamInfoResponse* response) override {
            // Set basic response information
            response->set_device_id(request->device_id());
            response->set_status(snowowl::StreamStatus::STOPPED);
            
            // In a real implementation, we would retrieve actual stream information
            // from the video processor or stream manager
            // For now, we'll return placeholder values
            
            // TODO: Retrieve actual stream information
            response->set_rtmp_url("");
            response->set_rtsp_url("");
            response->set_width(0);
            response->set_height(0);
            response->set_fps(0.0);
            
            return grpc::Status::OK;
        }
        
        grpc::Status ControlStream(grpc::ServerContext* context,
                                   const snowowl::ControlStreamRequest* request,
                                   snowowl::ControlStreamResponse* response) override {
            // In a real implementation, we would control the stream based on the action
            // For now, we'll just return a placeholder response
            
            response->set_success(true);
            response->set_message("Stream control command received");
            
            // TODO: Implement actual stream control logic
            // This would involve interfacing with the video processor or stream manager
            // to start, stop, or restart streams based on the request->action() value
            
            return grpc::Status::OK;
        }
        
        grpc::Status StreamVideo(grpc::ServerContext* context,
                                 grpc::ServerReaderWriter<snowowl::VideoFrameAck, snowowl::VideoFrame>* stream) override {
            // For now, we'll return unimplemented. In a real implementation, this would
            // handle video streaming.
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Streaming video not implemented");
        }
        
    private:
        SnowOwl::Config::DeviceRegistry& m_registry;
    };
    
    // Monitoring service implementation
    class MonitoringServiceImpl final : public snowowl::MonitoringService::Service {
    public:
        explicit MonitoringServiceImpl(SnowOwl::Config::DeviceRegistry& registry) 
            : registry_(registry) 
            {
                
            }
            
        grpc::Status GetRealTimeParams(grpc::ServerContext* context,
                                       const snowowl::GetRealTimeParamsRequest* request,
                                       snowowl::GetRealTimeParamsResponse* response) override {
            // Set the device ID in the response
            response->set_device_id(request->device_id());
            response->set_last_updated(std::time(nullptr));
            
            // In a real implementation, we would retrieve actual real-time parameters
            // For now, we'll return some placeholder parameters
            
            // TODO: Retrieve actual real-time parameters from the device or system
            (*response->mutable_params())["temperature"] = "25.6";
            (*response->mutable_params())["pressure"] = "101.3";
            (*response->mutable_params())["status"] = "normal";
            
            return grpc::Status::OK;
        }
        
        grpc::Status StreamRealTimeParams(grpc::ServerContext* context,
                                          const snowowl::StreamRealTimeParamsRequest* request,
                                          grpc::ServerWriter<snowowl::RealTimeParamUpdate>* writer) override {
            // For now, we'll return unimplemented. In a real implementation, this would
            // stream real-time parameter updates.
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Streaming real-time params not implemented");
        }
        
    private:
        SnowOwl::Config::DeviceRegistry& registry_;
    };
    
    std::unique_ptr<DeviceServiceImpl> deviceService_;
    std::unique_ptr<StreamServiceImpl> streamService_;
    std::unique_ptr<MonitoringServiceImpl> monitoringService_;
};

GrpcServer::GrpcServer(const std::string& address, SnowOwl::Config::DeviceRegistry& registry) 
    : address_(address)
    , registry_(registry) {
}

GrpcServer::~GrpcServer() {
    stop();
}

bool GrpcServer::start() {
    serviceImpl_ = std::make_unique<ServiceImpl>(registry_);
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(serviceImpl_->deviceService());
    builder.RegisterService(serviceImpl_->streamService());
    builder.RegisterService(serviceImpl_->monitoringService());
    
    server_ = builder.BuildAndStart();
    if (!server_) {
        std::cerr << "Failed to start gRPC server on " << address_ << std::endl;
        return false;
    }
    
    std::cout << "gRPC server listening on " << address_ << std::endl;
    return true;
}

void GrpcServer::stop() {
    if (server_) {
        server_->Shutdown();
        server_.reset();
    }
    
    serviceImpl_.reset();
}

}