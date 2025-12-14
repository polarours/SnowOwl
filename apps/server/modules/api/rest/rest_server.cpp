#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <fstream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

#include "modules/api/rest/rest_server.hpp"
#include "common/config/device_registry.hpp"
#include "core/streams/video_processor.hpp"
#include "core/streams/video_capture_manager.hpp"
#include "modules/utils/server_utils.hpp"
#include "modules/discovery/device_discovery.hpp"

namespace SnowOwl::Server::Modules::Api::Rest { 

namespace {
    
namespace beast = boost::beast;
namespace http = beast::http;

class Session : public std::enable_shared_from_this<Session> {
public:
Session(boost::asio::ip::tcp::socket socket, SnowOwl::Config::DeviceRegistry& registry)
    : socket_(std::move(socket))
    , registry_(registry)
    , videoProcessor_(nullptr)
    , deviceDiscovery_(std::make_unique<SnowOwl::Server::Modules::Discovery::DeviceDiscovery>())
{

}

void run() {
    doRead();
}

void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
    videoProcessor_ = processor;
}

private:
void doRead() {
    auto self = shared_from_this();
    parser_.emplace();
    parser_->body_limit(1024 * 16);
    beast::http::async_read(socket_, buffer_, *parser_,
    [self](beast::error_code ec, std::size_t) {
        if (!ec) {
            self->processRequest();
        }
    });
}

void processRequest() {
    if (!parser_) {
        send(buildError(http::status::internal_server_error, "Parser not initialised"));
        return;
    }

    const auto& req = parser_->get();
    
    if (req.method() == http::verb::get) {
        if (req.target() == "/api/v1/status") {
            handleServerStatus(req);
            return;
        }
        
        if (req.target() == "/api/v1/devices") {
            handleDeviceList(req);
            return;
        }

        if (req.target() == "/api/v1/config") {
            handleConfigList(req);
            return;
        }

        if (req.target().starts_with("/api/v1/devices/")) {
            handleDeviceInfo(req);
            return;
        }

        if (req.target() == "/api/v1/devices/discover") {
            handleDeviceDiscovery(req);
            return;
        }

        if (req.target().starts_with("/api/v1/capture/session/")) {
            std::string targetStr(req.target().data(), req.target().size());
            if (targetStr.find("/preview") != std::string::npos) {
                handleCapturePreviewStream(req);
                return;
            }
            
            handleCaptureSession(req);
            return;
        }
        
        if (req.target() == "/config/frontend") {
            handleFrontendConfig(req);
            return;
        }
        
        if (req.target() == "/api/v1/detection/status") {
            handleDetectionStatus(req);
            return;
        }
    } 
    else if (req.method() == http::verb::post) {
        if (req.target() == "/api/v1/devices") {
            handleCreateDevice(req);
            return;
        }
        
        if (req.target() == "/api/v1/devices/discover") {
            handleStartDeviceDiscovery(req);
            return;
        }
        
        if (req.target() == "/api/v1/detection/control") {
            handleDetectionControl(req);
            return;
        }
        
        if (req.target().starts_with("/api/v1/capture/session/")) {
            std::string targetStr(req.target().data(), req.target().size());
            if (targetStr.find("/config") != std::string::npos) {
                handleCaptureConfigUpdate(req);
                return;
            }
        }
    }
    else if (req.method() == http::verb::put) {
        if (req.target().starts_with("/api/v1/devices/")) {
            handleUpdateDevice(req);
            return;
        }
    }
    else if (req.method() == http::verb::delete_) {
        if (req.target().starts_with("/api/v1/devices/")) {
            handleDeleteDevice(req);
            return;
        }
    }

    send(buildError(http::status::not_found, "Not Found"));
}

void handleServerStatus(const http::request<http::string_body>& req) {
    nlohmann::json response = nlohmann::json::object();
    
    response["status"] = "running";
    response["timestamp"] = std::time(nullptr);
    response["message"] = "SnowOwl server is running";
    
    response["active"] = true;
    response["loaded"] = true;
    
    response["server"] = nlohmann::json::object();
    response["server"]["version"] = "0.1.0";
    response["server"]["started_at"] = std::time(nullptr);
    
    response["process"] = nlohmann::json::object();
#if defined(_WIN32)
    response["process"]["pid"] = GetCurrentProcessId();
#else
    response["process"]["pid"] = getpid();
#endif
    
    response["listening_ports"] = nlohmann::json::array();
    response["listening_ports"].push_back(8081);
    response["listening_ports"].push_back(7000);
    response["listening_ports"].push_back(7500);
    
    response["components"] = nlohmann::json::object();
    response["components"]["rest_api"] = "active";
    response["components"]["stream_receiver"] = "active";
    response["components"]["database"] = "connected";
    
    send(buildJson(http::status::ok, response.dump(4)));
}

void handleConfigList(const http::request<http::string_body>& req) {
    if (videoProcessor_ == nullptr) {
        send(buildError(http::status::internal_server_error, "server video processor not available"));
        return;
    }

    nlohmann::json response = nlohmann::json::object();
    
    send(buildJson(http::status::ok, response.dump(4)));
}

void handleDeviceList(const http::request<http::string_body>& req) {
    const auto devices = registry_.listDevices();
    nlohmann::json response = nlohmann::json::array();
    
    for (const auto& device : devices) {
        nlohmann::json deviceJson = {
            {"id", device.id},
            {"name", device.name},
            {"kind", SnowOwl::Config::toString(device.kind)},
            {"uri", device.uri},
            {"is_primary", device.isPrimary},
            {"enabled", device.enabled},
            {"created_at", device.createdAt},
            {"updated_at", device.updatedAt},
            {"metadata", device.metadata.empty() ? nlohmann::json::object() : nlohmann::json::parse(device.metadata, nullptr, false)}
        };


        
        response.push_back(deviceJson);
    }
    
    send(buildJson(http::status::ok, response.dump()));
}

void handleCreateDevice(const http::request<http::string_body>& req) {
    try {
        const auto jsonBody = nlohmann::json::parse(req.body());

        SnowOwl::Config::DeviceRecord device;
        device.name = jsonBody.value("name", "");
        device.kind = SnowOwl::Config::deviceKindFromString(jsonBody.value("kind", "camera"));
        device.uri = jsonBody.value("uri", "");
        device.enabled = jsonBody.value("enabled", 1);

        if (jsonBody.contains("metadata")) {
            device.metadata = jsonBody["metadata"].dump();
        }

        const auto result = registry_.upsertDevice(device);

        nlohmann::json response = nlohmann::json::object();
        response["id"] = result.id;
        response["message"] = "Device created successfully";
        
        send(buildJson(http::status::ok, response.dump()));
    } catch (const std::exception& e) {
        send(buildError(http::status::bad_request, std::string("Failed to create device: ") + e.what()));
    }
}

void handleDeleteDevice(const http::request<http::string_body>& req) {
    try {
        const std::string_view prefix = "/api/v1/devices/";
        const auto idView = req.target().substr(prefix.size());
        std::string idStr(idView.begin(), idView.end());
        
        if (idStr.empty()) {
            send(buildError(http::status::bad_request, "Missing device ID"));
            return;
        }
        
        const int deviceId = std::stoi(idStr);
        
        const bool success = registry_.removeDevice(deviceId);
        
        if (success) {
            nlohmann::json response = nlohmann::json::object();
            response["message"] = "Device deleted successfully";
            send(buildJson(http::status::ok, response.dump()));
        } else {
            send(buildError(http::status::not_found, "Device not found"));
        }
    } catch (const std::exception& e) {
        send(buildError(http::status::bad_request, std::string("Failed to delete device: ") + e.what()));
    }
}

void handleDeviceInfo(const http::request<http::string_body>& req) {
    const std::string_view prefix = "/api/v1/devices/";
    const auto idView = req.target().substr(prefix.size());
    std::string idStr(idView.begin(), idView.end());
    
    if (idStr.empty()) {
        send(buildError(http::status::bad_request, "Missing device ID"));
        return;
    }
    
    try {
        const int deviceId = std::stoi(idStr);
        const auto device = registry_.findById(deviceId);
        
        if (!device) {
            send(buildError(http::status::not_found, "Device not found"));
            return;
        }
        
        nlohmann::json deviceJson = {
            {"id", device->id},
            {"name", device->name},
            {"kind", SnowOwl::Config::toString(device->kind)},
            {"uri", device->uri},
            {"is_primary", device->isPrimary},
            {"enabled", device->enabled},
            {"created_at", device->createdAt},
            {"updated_at", device->updatedAt},
            {"metadata", device->metadata.empty() ? nlohmann::json::object() : nlohmann::json::parse(device->metadata, nullptr, false)}
        };


        
        send(buildJson(http::status::ok, deviceJson.dump(4)));
    } catch (const std::exception& e) {
        send(buildError(http::status::bad_request, std::string("Invalid device ID: ") + e.what()));
    }
}

void handleCaptureSession(const http::request<http::string_body>& req) {
    const std::string_view prefix = "/api/v1/capture/session/";
    const auto identifierView = req.target().substr(prefix.size());
    std::string deviceId(identifierView.begin(), identifierView.end());

    if (deviceId.empty()) {
        send(buildError(http::status::bad_request, "Missing device id"));
        return;
    }

    std::optional<SnowOwl::Config::DeviceRecord> record;

    const bool numeric = std::all_of(deviceId.begin(), deviceId.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });

    if (numeric) {
        try {
            record = registry_.findById(std::stoi(deviceId));
        } catch (const std::exception&) {
            record.reset();
        }
    }

    if (!record) {
        record = registry_.findByUri(deviceId);
    }

    if (!record) {
        const auto devices = registry_.listDevices();
        for (const auto& candidate : devices) {
            if (candidate.name == deviceId) {
                record = candidate;
                break;
            }

            if (!candidate.metadata.empty()) {
                const auto meta = nlohmann::json::parse(candidate.metadata, nullptr, false);
                if (!meta.is_discarded() && meta.is_object()) {
                    const auto metaId = meta.value("device_id", std::string{});
                    if (!metaId.empty() && metaId == deviceId) {
                        record = candidate;
                        break;
                    }
                }
            }
        }
    }
    if (!record) {
        send(buildError(http::status::not_found, "Device not found"));
        return;
    }

    nlohmann::json metadata;
    if (!record->metadata.empty()) {
        metadata = nlohmann::json::parse(record->metadata, nullptr, false);
    }

    if (metadata.is_discarded()) {
        metadata = nlohmann::json::object();
    }

    auto currentTime = std::time(nullptr);
    
    nlohmann::json response = {
        {"requested_id", deviceId},
        {"device_id", record->id},
        {"device_name", record->name},
        {"kind", SnowOwl::Config::toString(record->kind)},
        {"uri", record->uri},
        {"enabled", record->enabled},
        {"is_primary", record->isPrimary},
        {"protocol", nullptr},
        {"rtmp_url", nullptr},
        {"stream_key", nullptr},
        {"hls_url", nullptr},
        {"stream_outputs", metadata.value("stream_outputs", nlohmann::json::object())},
        {"stream_status", nlohmann::json::object()},
        {"last_updated", currentTime}
    };

    auto outputs = metadata.find("stream_outputs");
    std::string rtmpUrl;
    std::string streamKey;
    bool rtmpEnabled = false;
    std::string hlsUrl;
    bool hlsEnabled = false;

    nlohmann::json streamInfo = nlohmann::json::object();
    streamInfo["status"] = "unknown";
    streamInfo["bitrate"] = 0;
    streamInfo["resolution"] = "unknown";
    streamInfo["fps"] = 0;
    streamInfo["codec"] = "unknown";
    
    nlohmann::json streamStatus = nlohmann::json::object();
    streamStatus["is_active"] = false;
    streamStatus["last_update"] = currentTime;
    streamStatus["errors"] = nlohmann::json::array();
    
    nlohmann::json serverBrain = nlohmann::json::object();
    serverBrain["server_host"] = serverHostName();
    serverBrain["tracking_since"] = currentTime;
    serverBrain["device_activities"] = nlohmann::json::array();
    
    nlohmann::json cliTracking = nlohmann::json::object();
    cliTracking["last_command_received"] = currentTime;
    cliTracking["supported_protocols"] = nlohmann::json::array({"rtmp", "hls", "rtsp", "webrtc"});
    cliTracking["active_streams"] = 0;
    cliTracking["total_devices"] = registry_.listDevices().size();
    
    const auto cmdArgs = getCommandLineArguments();
    nlohmann::json cmdArgArray = nlohmann::json::array();
    for (const auto& arg : cmdArgs) {
        cmdArgArray.push_back(arg);
    }
    cliTracking["command_line_args"] = cmdArgArray;
    
    serverBrain["cli_tracking"] = cliTracking;
    
    if (outputs != metadata.end() && outputs->is_object()) {
        if (const auto rtmp = outputs->find("rtmp"); rtmp != outputs->end()) {
            if (rtmp->is_object()) {
                rtmpEnabled = rtmp->value("enabled", false);
                rtmpUrl = rtmp->value("url", std::string{});
                streamKey = rtmp->value("stream_key", std::string{});
            } else if (rtmp->is_boolean()) {
                rtmpEnabled = rtmp->get<bool>();
            }
        }

        if (const auto hls = outputs->find("hls"); hls != outputs->end()) {
            if (hls->is_object()) {
                hlsEnabled = hls->value("enabled", false);
                hlsUrl = hls->value("playlist", std::string{});
            } else if (hls->is_boolean()) {
                hlsEnabled = hls->get<bool>();
            }
        }
        
        streamInfo["configuration"] = *outputs;
    }

    if ((rtmpUrl.empty() || hlsUrl.empty()) && videoProcessor_) {
        const auto& profile = videoProcessor_->getStreamProfile();
        
        if (rtmpUrl.empty() && profile.rtmp.enabled) {
            const auto urlIt = profile.rtmp.parameters.find("url");
            if (urlIt != profile.rtmp.parameters.end()) {
                rtmpUrl = urlIt->second;
                response["rtmp_url"] = rtmpUrl;
            }
            
            const auto keyIt = profile.rtmp.parameters.find("stream_key");
            if (keyIt != profile.rtmp.parameters.end()) {
                streamKey = keyIt->second;
                response["stream_key"] = streamKey;
            }
        }
        
        if (hlsUrl.empty() && profile.hls.enabled) {
            const auto playlistIt = profile.hls.parameters.find("playlist");
            if (playlistIt != profile.hls.parameters.end()) {
                hlsUrl = playlistIt->second;
                response["hls_url"] = hlsUrl;
            }
        }
    }

    if (!rtmpUrl.empty()) {
        response["rtmp_url"] = rtmpUrl;
    }
    if (!streamKey.empty()) {
        response["stream_key"] = streamKey;
    }

    if (!hlsUrl.empty()) {
        response["hls_url"] = hlsUrl;
    }

    if (hlsEnabled && !hlsUrl.empty()) {
        response["protocol"] = "hls";
        streamInfo["status"] = "active";
        streamInfo["type"] = "hls";
        streamStatus["is_active"] = true;
        streamStatus["active_protocol"] = "hls";
    } else if (rtmpEnabled && !rtmpUrl.empty()) {
        response["protocol"] = "rtmp";
        streamInfo["status"] = "active";
        streamInfo["type"] = "rtmp";
        streamStatus["is_active"] = true;
        streamStatus["active_protocol"] = "rtmp";
    } else if (!hlsUrl.empty()) {
        response["protocol"] = "hls";
        streamInfo["status"] = "configured";
        streamInfo["type"] = "hls";
        streamStatus["active_protocol"] = "none";
    } else if (!rtmpUrl.empty()) {
        response["protocol"] = "rtmp";
        streamInfo["status"] = "configured";
        streamInfo["type"] = "rtmp";
        streamStatus["active_protocol"] = "none";
    } else {
        if (videoProcessor_) {
            const auto& profile = videoProcessor_->getStreamProfile();
            if (profile.hls.enabled) {
                const auto playlistIt = profile.hls.parameters.find("playlist");
                if (playlistIt != profile.hls.parameters.end() && !playlistIt->second.empty()) {
                    response["protocol"] = "hls";
                    streamInfo["status"] = "active";
                    streamInfo["type"] = "hls";
                    streamStatus["is_active"] = true;
                    streamStatus["active_protocol"] = "hls";
                }
            } else if (profile.rtmp.enabled) {
                const auto urlIt = profile.rtmp.parameters.find("url");
                if (urlIt != profile.rtmp.parameters.end() && !urlIt->second.empty()) {
                    response["protocol"] = "rtmp";
                    streamInfo["status"] = "active";
                    streamInfo["type"] = "rtmp";
                    streamStatus["is_active"] = true;
                    streamStatus["active_protocol"] = "rtmp";
                }
            }
        }
        
        if (streamStatus.find("active_protocol") == streamStatus.end()) {
            streamStatus["active_protocol"] = "none";
        }
    }
    
    response["stream_info"] = streamInfo;
    response["stream_status"] = streamStatus;
    
    nlohmann::json captureSession = nlohmann::json::object();
    captureSession["started_at"] = currentTime;
    captureSession["is_live"] = true;
    captureSession["network_score"] = 80;
    
    if (outputs != metadata.end()) {
        streamInfo["configuration"] = *outputs;
    } else {
        streamInfo["configuration"] = nlohmann::json::object();
    }
    
    response["server_brain"] = serverBrain;
    
    nlohmann::json deviceInfo = nlohmann::json::object();
    deviceInfo["connection_type"] = "direct";
    deviceInfo["capabilities"] = nlohmann::json::array({"video", "audio"});
    deviceInfo["supported_codecs"] = nlohmann::json::array({"H.264", "H.265"});
    
    response["device_info"] = deviceInfo;
    
    nlohmann::json presets = nlohmann::json::array();
    nlohmann::json preset1;
    preset1["label"] = "1080p · 30fps";
    preset1["resolution"] = "1920x1080";
    preset1["bitrate_mbps"] = 4.0;
    preset1["frame_rate"] = 30;
    preset1["is_default"] = true;
    presets.push_back(preset1);
    
    nlohmann::json preset2;
    preset2["label"] = "720p · 30fps";
    preset2["resolution"] = "1280x720";
    preset2["bitrate_mbps"] = 2.5;
    preset2["frame_rate"] = 30;
    preset2["is_default"] = false;
    presets.push_back(preset2);
    
    captureSession["presets"] = presets;
    
    nlohmann::json metrics = nlohmann::json::array();
    nlohmann::json metric1;
    metric1["label"] = "Output Bitrate";
    metric1["value"] = "4.2 Mbps";
    metrics.push_back(metric1);
    
    nlohmann::json metric2;
    metric2["label"] = "Average Latency";
    metric2["value"] = "190 ms";
    metrics.push_back(metric2);
    
    nlohmann::json metric3;
    metric3["label"] = "Packet Loss Rate";
    metric3["value"] = "0.4 %";
    metrics.push_back(metric3);
    
    captureSession["metrics"] = metrics;
    
    response["capture_session"] = captureSession;

    send(buildJson(http::status::ok, response.dump(2)));
}

void handleDetectionControl(const http::request<http::string_body>& req) {
    try {
        const auto jsonBody = nlohmann::json::parse(req.body());
        
        const std::string detectionType = jsonBody.value("type", "");
        const bool enabled = jsonBody.value("enabled", false);
        
        if (videoProcessor_) {
            using SnowOwl::Detection::DetectionType;
            
            if (detectionType == "detection") {
                videoProcessor_->setMotionDetection(enabled);
                videoProcessor_->setIntrusionDetection(enabled);
                videoProcessor_->setFireDetection(enabled);
                videoProcessor_->setGasLeakDetection(enabled);
                videoProcessor_->setEquipmentDetection(enabled);
                videoProcessor_->setFaceRecognition(enabled);
                videoProcessor_->setPipelineInspection(enabled);
            }
            else if (detectionType == "motion") {
                videoProcessor_->setMotionDetection(enabled);
            } else if (detectionType == "intrusion") {
                videoProcessor_->setIntrusionDetection(enabled);
            } else if (detectionType == "fire") {
                videoProcessor_->setFireDetection(enabled);
            } else if (detectionType == "gas_leak") {
                videoProcessor_->setGasLeakDetection(enabled);
            } else if (detectionType == "equipment") {
                videoProcessor_->setEquipmentDetection(enabled);
            } else if (detectionType == "face_recognition") {
                videoProcessor_->setFaceRecognition(enabled);
            } else if (detectionType == "pipeline_inspection") {
                videoProcessor_->setPipelineInspection(enabled);
            } else {
                nlohmann::json response = nlohmann::json::object();
                response["error"] = "Unknown detection type: " + detectionType;
                send(buildJson(http::status::bad_request, response.dump()));
                return;
            }
            
            nlohmann::json response = nlohmann::json::object();
            response["message"] = "Detection control command processed successfully";
            response["type"] = detectionType;
            response["enabled"] = enabled;
            
            send(buildJson(http::status::ok, response.dump()));
        } else {
            nlohmann::json response = nlohmann::json::object();
            response["error"] = "Video processor not available";
            send(buildJson(http::status::internal_server_error, response.dump()));
        }
    } catch (const std::exception& e) {
        send(buildError(http::status::bad_request, std::string("Failed to process detection control: ") + e.what()));
    }
}

void handleCaptureConfigUpdate(const http::request<http::string_body>& req) {
    try {
        const auto jsonBody = nlohmann::json::parse(req.body());
        
        const std::string deviceIdStr = jsonBody.value("device_id", "");
        const std::string action = jsonBody.value("action", "");
        
        int deviceId = -1;
        try {
            deviceId = std::stoi(deviceIdStr);
        } catch (const std::exception& e) {
            nlohmann::json errorResponse = nlohmann::json::object();
            errorResponse["error"] = "Invalid device ID format";
            send(buildJson(http::status::bad_request, errorResponse.dump()));
            return;
        }
        
        nlohmann::json response = nlohmann::json::object();
        response["message"] = "Configuration update received";
        response["device_id"] = deviceId;
        response["action"] = action;
        
        auto* captureManager = SnowOwl::Server::Core::VideoCaptureManager::getInstance();
        if (!captureManager) {
            response["status"] = "error";
            response["error"] = "VideoCaptureManager not available";
            send(buildJson(http::status::internal_server_error, response.dump()));
            return;
        }
        
        auto* videoCapture = captureManager->getVideoCapture(deviceId);
        if (!videoCapture) {
            response["status"] = "error";
            response["error"] = "VideoCapture not found for device " + deviceIdStr;
            send(buildJson(http::status::not_found, response.dump()));
            return;
        }
        
        if (action == "update_resolution") {
            const std::string resolution = jsonBody.value("resolution", "");
            response["details"] = "Resolution update requested: " + resolution;
            
            videoCapture->updateResolution(resolution);
            response["status"] = "success";
        } else if (action == "update_frame_rate") {
            const int fps = jsonBody.value("fps", 0);
            response["details"] = "Frame rate update requested: " + std::to_string(fps) + " fps";
            
            videoCapture->updateFps(fps);
            response["status"] = "success";
        } else if (action == "update_bitrate") {
            const int bitrate = jsonBody.value("bitrate", 0);
            response["details"] = "Bitrate update requested: " + std::to_string(bitrate) + " kbps";
            
            videoCapture->updateBitrate(bitrate);
            response["status"] = "success";
        } else if (action == "update_scene_name") {
            const std::string sceneName = jsonBody.value("scene_name", "");
            response["details"] = "Scene name update requested: " + sceneName;
            response["status"] = "success";
        } else if (action == "update_protocol") {
            const std::string protocol = jsonBody.value("protocol", "");
            response["details"] = "Protocol update requested: " + protocol;
            response["status"] = "success";
        } else if (action == "apply_preset") {
            if (jsonBody.contains("preset")) {
                const auto& preset = jsonBody["preset"];
                response["details"] = "Preset application requested";
                
                SnowOwl::Server::Core::CaptureConfig config;
                config.resolution = preset.value("resolution", "1920x1080");
                double bitrate_mbps = preset.value("bitrate_mbps", 2.0);
                config.bitrate_kbps = static_cast<int>(bitrate_mbps * 1000);
                config.fps = preset.value("frame_rate", 30);
                
                videoCapture->updateConfig(config);
                response["status"] = "success";
            }
        } else {
            response["error"] = "Unknown action: " + action;
            send(buildJson(http::status::bad_request, response.dump()));
            return;
        }
        
        send(buildJson(http::status::ok, response.dump()));
    } catch (const std::exception& e) {
        send(buildError(http::status::bad_request, std::string("Failed to process configuration update: ") + e.what()));
    }
}

void handleDeviceDiscovery(const http::request<http::string_body>& req) {
    nlohmann::json response = nlohmann::json::array();
    
    auto networkDevices = deviceDiscovery_->discoverNetworkDevices();
    
    for (const auto& device : networkDevices) {
        nlohmann::json deviceJson;
        deviceJson["id"] = nullptr;
        deviceJson["name"] = device.modelName.empty() ? ("Network Device (" + device.ipAddress + ")") : device.modelName;
        deviceJson["ip_address"] = device.ipAddress;
        deviceJson["mac_address"] = device.macAddress;
        deviceJson["model_name"] = device.modelName;
        deviceJson["manufacturer"] = device.manufacturer;
        deviceJson["supported_protocols"] = device.supportedProtocols;
        deviceJson["rtsp_url"] = device.rtspUrl;
        deviceJson["http_admin_url"] = device.httpAdminUrl;
        deviceJson["type"] = "network";
        deviceJson["registered"] = false;
        deviceJson["status"] = "discovered";
        
        response.push_back(deviceJson);
    }
    
    auto localDevices = deviceDiscovery_->discoverLocalDevices();
    
    for (const auto& device : localDevices) {
        nlohmann::json deviceJson;
        deviceJson["id"] = nullptr;
        deviceJson["name"] = device.name;
        deviceJson["device_id"] = device.deviceId;
        deviceJson["manufacturer"] = device.manufacturer;
        deviceJson["model"] = device.model;
        deviceJson["supported_formats"] = device.supportedFormats;
        deviceJson["width"] = device.width;
        deviceJson["height"] = device.height;
        deviceJson["type"] = "local";
        deviceJson["registered"] = false;
        deviceJson["status"] = "discovered";
        
        response.push_back(deviceJson);
    }
    
    auto registeredDevices = registry_.listDevices();
    for (const auto& device : registeredDevices) {
        nlohmann::json deviceJson;
        deviceJson["id"] = device.id;
        deviceJson["name"] = device.name;
        deviceJson["kind"] = device.kind;
        deviceJson["uri"] = device.uri;
        deviceJson["enabled"] = device.enabled;
        deviceJson["is_primary"] = device.isPrimary;
        deviceJson["ip_address"] = device.ipAddress;
        deviceJson["mac_address"] = device.macAddress;
        deviceJson["manufacturer"] = device.manufacturer;
        deviceJson["type"] = "registered";
        deviceJson["registered"] = true;
        deviceJson["status"] = device.enabled ? "active" : "inactive";
        deviceJson["created_at"] = device.createdAt;
        deviceJson["updated_at"] = device.updatedAt;
        
        if (!device.metadata.empty()) {
            try {
                deviceJson["metadata"] = nlohmann::json::parse(device.metadata);
            } catch (...) {

            }
        }
        
        response.push_back(deviceJson);
    }
    
    send(buildJson(http::status::ok, response.dump()));
}

void handleStartDeviceDiscovery(const http::request<http::string_body>& req) {
    nlohmann::json response = nlohmann::json::object();
    response["status"] = "discovery_started";
    response["message"] = "Device discovery process initiated";
    response["estimated_completion_time"] = "30 seconds";
    
    send(buildJson(http::status::ok, response.dump()));
}

void handleCapturePreviewStream(const http::request<http::string_body>& req) {
    std::string targetStr(req.target().data(), req.target().size());
    size_t sessionIdPos = targetStr.find("session/");
    if (sessionIdPos == std::string::npos) {
        send(buildError(http::status::bad_request, "Invalid request path"));
        return;
    }
    
    size_t previewPos = targetStr.find("/preview");
    if (previewPos == std::string::npos) {
        send(buildError(http::status::bad_request, "Invalid request path"));
        return;
    }
    
    std::string deviceId = targetStr.substr(sessionIdPos + 8, previewPos - sessionIdPos - 8);
    if (deviceId.empty()) {
        send(buildError(http::status::bad_request, "Missing device ID"));
        return;
    }
    
    nlohmann::json response = nlohmann::json::object();
    response["device_id"] = deviceId;
    response["message"] = "Preview stream endpoint - to be implemented";
    response["preview_url"] = "rtmp://localhost:1935/live/" + deviceId + "_preview";
    
    send(buildJson(http::status::ok, response.dump()));
}

void handleFrontendConfig(const http::request<http::string_body>& req) {
    std::ifstream configFile("config/frontend_config.json");
    if (!configFile.is_open()) {
        configFile.open("/etc/snowowl/frontend_config.json");
        if (!configFile.is_open()) {
            configFile.open("../config/frontend_config.json");
            if (!configFile.is_open()) {
                send(buildError(http::status::not_found, "Config file not found"));
                return;
            }
        }
    }

    std::string configContent((std::istreambuf_iterator<char>(configFile)),
                               std::istreambuf_iterator<char>());
    
    if (configContent.empty()) {
        send(buildError(http::status::internal_server_error, "Config file is empty"));
        return;
    }

    send(buildJson(http::status::ok, configContent));
}

void handleUpdateDevice(const http::request<http::string_body>& req) {
    try {
        const std::string_view prefix = "/api/v1/devices/";
        const auto idView = req.target().substr(prefix.size());
        std::string idStr(idView.begin(), idView.end());
        
        if (idStr.empty()) {
            send(buildError(http::status::bad_request, "Missing device ID"));
            return;
        }
        
        const int deviceId = std::stoi(idStr);
        const auto jsonBody = nlohmann::json::parse(req.body());

        auto existingDevice = registry_.findById(deviceId);
        if (!existingDevice) {
            send(buildError(http::status::not_found, "Device not found"));
            return;
        }

        SnowOwl::Config::DeviceRecord device = existingDevice.value();
        
        if (jsonBody.contains("name")) {
            device.name = jsonBody.value("name", device.name);
        }
        
        if (jsonBody.contains("kind")) {
            device.kind = SnowOwl::Config::deviceKindFromString(
                jsonBody.value("kind", SnowOwl::Config::toString(device.kind))
            );
        }
        
        if (jsonBody.contains("uri")) {
            device.uri = jsonBody.value("uri", device.uri);
        }
        
        if (jsonBody.contains("enabled")) {
            device.enabled = jsonBody.value("enabled", device.enabled);
        }
        
        if (jsonBody.contains("is_primary")) {
            device.isPrimary = jsonBody.value("is_primary", device.isPrimary);
        }
        
        if (jsonBody.contains("metadata")) {
            device.metadata = jsonBody["metadata"].dump();
        }
        
        if (jsonBody.contains("ip_address")) {
            device.ipAddress = jsonBody.value("ip_address", device.ipAddress);
        }
        
        if (jsonBody.contains("mac_address")) {
            device.macAddress = jsonBody.value("mac_address", device.macAddress);
        }
        
        if (jsonBody.contains("manufacturer")) {
            device.manufacturer = jsonBody.value("manufacturer", device.manufacturer);
        }

        const auto result = registry_.upsertDevice(device);

        nlohmann::json response = nlohmann::json::object();
        response["id"] = result.id;
        response["message"] = "Device updated successfully";
        
        send(buildJson(http::status::ok, response.dump()));
    } catch (const std::exception& e) {
        send(buildError(http::status::bad_request, std::string("Failed to update device: ") + e.what()));
    }
}

void handleDetectionStatus(const http::request<http::string_body>& req) {
    if (!videoProcessor_) {
        send(buildError(http::status::internal_server_error, "Video processor not available"));
        return;
    }
    
    std::string detectionType = "detection";
    std::string targetStr(req.target().data(), req.target().size());
    
    size_t queryPos = targetStr.find('?');
    if (queryPos != std::string::npos) {
        std::string queryString = targetStr.substr(queryPos + 1);
        size_t typePos = queryString.find("type=");
        if (typePos != std::string::npos) {
            size_t endPos = queryString.find('&', typePos);
            if (endPos == std::string::npos) {
                endPos = queryString.length();
            }
            detectionType = queryString.substr(typePos + 5, endPos - typePos - 5);
        }
    }
    
    nlohmann::json response = nlohmann::json::object();
    
    if (detectionType == "detection") {
        bool anyEnabled = videoProcessor_->isDetectionEnabled(SnowOwl::Detection::DetectionType::Motion) ||
                         videoProcessor_->isDetectionEnabled(SnowOwl::Detection::DetectionType::Intrusion) ||
                         videoProcessor_->isDetectionEnabled(SnowOwl::Detection::DetectionType::Fire) ||
                         videoProcessor_->isDetectionEnabled(SnowOwl::Detection::DetectionType::GasLeak) ||
                         videoProcessor_->isDetectionEnabled(SnowOwl::Detection::DetectionType::EquipmentFailure) ||
                         videoProcessor_->isDetectionEnabled(SnowOwl::Detection::DetectionType::FaceRecognition);
                         
        response["type"] = detectionType;
        response["enabled"] = anyEnabled;
    } else {
        using SnowOwl::Detection::DetectionType;
        
        if (detectionType == "motion") {
            response["type"] = detectionType;
            response["enabled"] = videoProcessor_->isDetectionEnabled(DetectionType::Motion);
        } else if (detectionType == "intrusion") {
            response["type"] = detectionType;
            response["enabled"] = videoProcessor_->isDetectionEnabled(DetectionType::Intrusion);
        } else if (detectionType == "fire") {
            response["type"] = detectionType;
            response["enabled"] = videoProcessor_->isDetectionEnabled(DetectionType::Fire);
        } else if (detectionType == "gas_leak") {
            response["type"] = detectionType;
            response["enabled"] = videoProcessor_->isDetectionEnabled(DetectionType::GasLeak);
        } else if (detectionType == "equipment") {
            response["type"] = detectionType;
            response["enabled"] = videoProcessor_->isDetectionEnabled(DetectionType::EquipmentFailure);
        } else if (detectionType == "face_recognition") {
            response["type"] = detectionType;
            response["enabled"] = videoProcessor_->isDetectionEnabled(DetectionType::FaceRecognition);
        } else {
            response["error"] = "Unknown detection type: " + detectionType;
            send(buildJson(http::status::bad_request, response.dump()));
            return;
        }
    }
    
    send(buildJson(http::status::ok, response.dump()));
}

template <typename Body>
void send(http::response<Body>&& msg) {
    auto self = shared_from_this();
    auto response = std::make_shared<http::response<Body>>(std::move(msg));
    response->prepare_payload();
    http::async_write(socket_, *response,
    [self, response](beast::error_code ec, std::size_t) {
        if (!ec) {
            beast::error_code shutdownEc;
            self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, shutdownEc);
        }
    });
}

static http::response<http::string_body> buildError(http::status status, std::string message) {
    nlohmann::json payload = {
        {"error", std::move(message)},
    };
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json; charset=utf-8");
    res.body() = payload.dump();
    return res;
}

static http::response<http::string_body> buildJson(http::status status, std::string body) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json; charset=utf-8");
    res.body() = std::move(body);
    return res;
}

boost::asio::ip::tcp::socket socket_;
beast::flat_buffer buffer_;
std::optional<http::request_parser<http::string_body>> parser_;
SnowOwl::Config::DeviceRegistry& registry_;
SnowOwl::Server::Core::VideoProcessor* videoProcessor_;
std::unique_ptr<SnowOwl::Server::Modules::Discovery::DeviceDiscovery> deviceDiscovery_ {nullptr};
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(boost::asio::io_context& ioc,
             boost::asio::ip::tcp::endpoint endpoint,
             SnowOwl::Config::DeviceRegistry& registry)
        : ioc_(ioc)
        , acceptor_(ioc)
        , registry_(registry)
        , videoProcessor_(nullptr)
    {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (!ec) {
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        }
        if (!ec) {
            acceptor_.bind(endpoint, ec);
        }
        if (!ec) {
            acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        }
        if (ec) {
            throw std::runtime_error("RestServer: cannot start listener: " + ec.message());
        }
    }

void run() {
        doAccept();
    }

    void stop() {
        boost::system::error_code ec;
        acceptor_.close(ec);
    }

    void setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor) {
        videoProcessor_ = processor;
    }

private:
    void doAccept() {
        auto self = shared_from_this();
        acceptor_.async_accept(
            [self](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    auto session = std::make_shared<Session>(std::move(socket), self->registry_);
                    session->setVideoProcessor(self->videoProcessor_);
                    session->run();
                }
                if (self->acceptor_.is_open()) {
                    self->doAccept();
                }
            });
    }

    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    SnowOwl::Config::DeviceRegistry& registry_;
    SnowOwl::Server::Core::VideoProcessor* videoProcessor_{nullptr};
};

}

class RestServer::Impl {
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
        ioContext_ = std::make_unique<boost::asio::io_context>();
        bool boundAny = false;

        try {
            auto v6Endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port_);
            listenerV6_ = std::make_shared<Listener>(*ioContext_, v6Endpoint, registry_);
            listenerV6_->setVideoProcessor(videoProcessor_);
            listenerV6_->run();
            boundAny = true;
        } catch (const std::exception& e) {
            std::cerr << "RestServer: IPv6 bind failed: " << e.what() << std::endl;
            listenerV6_.reset();
        }

        try {
            auto v4Endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_);
            listenerV4_ = std::make_shared<Listener>(*ioContext_, v4Endpoint, registry_);
            listenerV4_->setVideoProcessor(videoProcessor_);
            listenerV4_->run();
            boundAny = true;
        } catch (const std::exception& e) {
            std::cerr << "RestServer: IPv4 bind failed: " << e.what() << std::endl;
            listenerV4_.reset();
        }

        if (!boundAny) {
            throw std::runtime_error("RestServer: failed to bind on any interface");
        }

        running_ = true;
        thread_ = std::thread([this]() {
            try {
                ioContext_->run();
            } catch (const std::exception& ex) {
                std::cerr << "RestServer: io_context error: " << ex.what() << std::endl;
            }
        });
    } catch (const std::exception& e) {
        std::cerr << "RestServer: failed to start: " << e.what() << std::endl;
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
    std::unique_ptr<boost::asio::io_context> ioContext_;
    std::shared_ptr<Listener> listenerV6_;
    std::shared_ptr<Listener> listenerV4_;
    std::thread thread_;
    bool running_{false};
};

RestServer::RestServer(SnowOwl::Config::DeviceRegistry& registry, std::uint16_t port)
    : impl_(std::make_unique<Impl>(registry, port)) {

}

RestServer::~RestServer() {
    stop();
}

bool RestServer::start() {
    return impl_->start();
}

void RestServer::stop() {
    if (impl_) {
        impl_->stop();
    }
}

void RestServer::setVideoProcessor(SnowOwl::Server::Core::VideoProcessor* processor)  {
    if (impl_) {
        impl_->setVideoProcessor(processor);
    }
}

void RestServer::setMediaMTXConfig(const SnowOwl::Server::Modules::Media::MediaMTXConfig&) {
}

}
