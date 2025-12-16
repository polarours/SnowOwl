#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>

typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

namespace SnowOwl::Config {

// device kinds supported by the system
// also include streaming protocols for easier filtering.
enum class DeviceKind {
    Unknown,
    Camera,
    RTSP,
    RTMP,
    File,
    HTTP,         
    HLS,          
    WebRTC,       
    ONVIF,        
    Discovered,
    Microphone,
    Speaker
};

struct DeviceRecord {
    int id{0};
    std::string name;
    DeviceKind kind{DeviceKind::Unknown};
    std::string uri;
    bool isPrimary{false};
    bool enabled{true};
    std::string metadata;
    long long createdAt{0};  
    long long updatedAt{0};  
    
    std::string ipAddress;
    std::string macAddress;
    std::string manufacturer;
    std::vector<std::string> supportedProtocols;
};

std::string toString(DeviceKind kind);
DeviceKind deviceKindFromString(const std::string& value);

class DeviceRegistry {
public:
    DeviceRegistry();
    ~DeviceRegistry();

    bool open(const std::string& connectionString);
    std::string databasePath() const;

    std::vector<DeviceRecord> listDevices() const;
    std::vector<DeviceRecord> listDevicesByKind(DeviceKind kind) const;
    std::optional<DeviceRecord> primaryDevice() const;
    std::optional<DeviceRecord> findById(int id) const;
    std::optional<DeviceRecord> findByUri(const std::string& uri) const;
    std::vector<DeviceRecord> searchByName(const std::string& namePattern) const;

    std::vector<DeviceRecord> listDevicesByProtocol(const std::string& protocol) const;
    std::vector<DeviceRecord> listActiveDevices() const;
    std::vector<DeviceRecord> searchByIpAddress(const std::string& ipPattern) const;

    DeviceRecord upsertDevice(const DeviceRecord& record);
    bool removeDevice(int id);
    bool setPrimaryDevice(int id);
    
    bool updateDeviceStatus(int id, bool active);
    bool updateLastSeen(int id);

private:
    void ensureSchema();
    DeviceRecord mapRow(PGresult* result, int row) const;
    std::vector<DeviceRecord> queryAll(const std::string& sql) const;

    PGconn* db_;
    std::string connectionString_;
    mutable std::mutex mutex_;
};

}