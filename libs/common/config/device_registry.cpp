#include <iostream>
#include <libpq-fe.h>

#include "config/device_registry.hpp"

namespace SnowOwl::Config {

namespace {

constexpr const char* kCreateTableSql = R"SQL(
CREATE TABLE IF NOT EXISTS devices (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    kind TEXT NOT NULL,
    uri TEXT NOT NULL,
    is_primary BOOLEAN NOT NULL DEFAULT FALSE,
    enabled BOOLEAN NOT NULL DEFAULT TRUE,
    metadata TEXT DEFAULT '',
    ip_address TEXT DEFAULT '',
    mac_address TEXT DEFAULT '',
    manufacturer TEXT DEFAULT '',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
)SQL";

constexpr const char* kCreatePrimaryIndexSql = R"SQL(
CREATE UNIQUE INDEX IF NOT EXISTS idx_devices_primary
    ON devices(is_primary)
    WHERE is_primary = TRUE;
)SQL";

constexpr const char* kCreateUriIndexSql = R"SQL(
CREATE UNIQUE INDEX IF NOT EXISTS idx_devices_uri
    ON devices(uri);
)SQL";

constexpr const char* kCreateIpIndexSql = R"SQL(
CREATE INDEX IF NOT EXISTS idx_devices_ip
    ON devices(ip_address);
)SQL";

std::string normalizeLabel(DeviceKind kind) {
    switch (kind) {
        case DeviceKind::Camera: 
            return "camera";
        case DeviceKind::RTSP: 
            return "rtsp";
        case DeviceKind::RTMP: 
            return "rtmp";
        case DeviceKind::File: 
            return "file";
        case DeviceKind::HTTP: 
            return "http";
        case DeviceKind::HLS: 
            return "hls";
        case DeviceKind::WebRTC: 
            return "webrtc";
        case DeviceKind::ONVIF: 
            return "onvif";
        case DeviceKind::Discovered: 
            return "discovered";
        default: 
            return "unknown";
    }
}

DeviceKind kindFromLabel(const std::string& value) {
    if (value == "camera") {
        return DeviceKind::Camera;
    }
    if (value == "rtsp") {
        return DeviceKind::RTSP;
    }
    if (value == "rtmp") {
        return DeviceKind::RTMP;
    }
    if (value == "file") {
        return DeviceKind::File;
    }
    if (value == "http") {
        return DeviceKind::HTTP;
    }
    if (value == "hls") {
        return DeviceKind::HLS;
    }
    if (value == "webrtc") {
        return DeviceKind::WebRTC;
    }
    if (value == "onvif") {
        return DeviceKind::ONVIF;
    }
    if (value == "discovered") {
        return DeviceKind::Discovered;
    }
    return DeviceKind::Unknown;
}

bool execSql(PGconn* db, const char* sql) {
    PGresult* res = PQexec(db, sql);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK || PQresultStatus(res) == PGRES_TUPLES_OK);
    
    if (!success) {
        std::cerr << "PostgreSQL error: " << PQresultErrorMessage(res) << std::endl;
    }
    
    PQclear(res);
    return success;
}

int getInteger(PGresult* res, int row, int col) {
    char* val = PQgetvalue(res, row, col);
    return val ? atoi(val) : 0;
}

std::string getText(PGresult* res, int row, int col) {
    char* val = PQgetvalue(res, row, col);
    return val ? std::string(val) : std::string();
}

}

std::string toString(DeviceKind kind) {
    return normalizeLabel(kind);
}

DeviceKind deviceKindFromString(const std::string& value) {
    return kindFromLabel(value);
}

DeviceRegistry::DeviceRegistry()
    : db_(nullptr) {
}

DeviceRegistry::~DeviceRegistry() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        PQfinish(db_);
        db_ = nullptr;
    }
}

bool DeviceRegistry::open(const std::string& connectionString) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (db_) {
        PQfinish(db_);
        db_ = nullptr;
    }

    db_ = PQconnectdb(connectionString.c_str());
    
    if (PQstatus(db_) != CONNECTION_OK) {
        std::cerr << "Failed to connect to database: " << PQerrorMessage(db_) << std::endl;
        PQfinish(db_);
        db_ = nullptr;
        return false;
    }

    connectionString_ = connectionString;
    ensureSchema();
    return true;
}

std::string DeviceRegistry::databasePath() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connectionString_;
}

std::vector<DeviceRecord> DeviceRegistry::listDevices() const
{
    return queryAll("SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices ORDER BY id ASC;");
}

std::optional<DeviceRecord> DeviceRegistry::primaryDevice() const
{
    auto all = queryAll("SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE is_primary = TRUE LIMIT 1;");
    if (all.empty()) {
        return std::nullopt;
    }
    return all.front();
}

std::optional<DeviceRecord> DeviceRegistry::findById(int id) const
{
    std::vector<DeviceRecord> rows;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!db_) {
            return std::nullopt;
        }

        std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE id = " + std::to_string(id) + " LIMIT 1;";
        PGresult* res = PQexec(db_, query.c_str());
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cerr << "PostgreSQL query failed: " << PQresultErrorMessage(res) << std::endl;
            PQclear(res);
            return std::nullopt;
        }

        if (PQntuples(res) > 0) {
            rows.push_back(mapRow(res, 0));
        }
        
        PQclear(res);
    }

    if (rows.empty()) {
        return std::nullopt;
    }
    return rows.front();
}

std::optional<DeviceRecord> DeviceRegistry::findByUri(const std::string& uri) const
{
    std::vector<DeviceRecord> rows;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!db_) {
            return std::nullopt;
        }

        std::string escapedUri = uri;
        std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE uri = '" + escapedUri + "' LIMIT 1;";
        PGresult* res = PQexec(db_, query.c_str());
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cerr << "PostgreSQL query failed: " << PQresultErrorMessage(res) << std::endl;
            PQclear(res);
            return std::nullopt;
        }

        if (PQntuples(res) > 0) {
            rows.push_back(mapRow(res, 0));
        }
        
        PQclear(res);
    }

    if (rows.empty()) {
        return std::nullopt;
    }
    return rows.front();
}

std::vector<DeviceRecord> DeviceRegistry::listDevicesByKind(DeviceKind kind) const
{
    std::string kindStr = normalizeLabel(kind);
    std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE kind = '" + kindStr + "' ORDER BY id ASC;";
    return queryAll(query);
}

std::vector<DeviceRecord> DeviceRegistry::searchByName(const std::string& namePattern) const
{
    std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE name ILIKE '%" + namePattern + "%' ORDER BY id ASC;";
    return queryAll(query);
}

std::vector<DeviceRecord> DeviceRegistry::listDevicesByProtocol(const std::string& protocol) const
{
    std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE metadata->'protocols' @> '\""+ protocol +"\"' ORDER BY id ASC;";
    return queryAll(query);
}

std::vector<DeviceRecord> DeviceRegistry::listActiveDevices() const
{
    // This would typically check for devices that have been recently active
    std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE enabled = 1 ORDER BY id ASC;";
    return queryAll(query);
}

std::vector<DeviceRecord> DeviceRegistry::searchByIpAddress(const std::string& ipPattern) const
{
    std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE ip_address ILIKE '%" + ipPattern + "%' ORDER BY id ASC;";
    return queryAll(query);
}

DeviceRecord DeviceRegistry::upsertDevice(const DeviceRecord& record)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) {
        std::cerr << "DeviceRegistry not opened" << std::endl;
        return record;
    }

    PGresult* res = nullptr;
    DeviceRecord result = record;
    
    if (record.id > 0) {
        std::string checkQuery = "SELECT COUNT(*) FROM devices WHERE id = " + std::to_string(record.id) + ";";
        res = PQexec(db_, checkQuery.c_str());
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cerr << "PostgreSQL check failed: " << PQresultErrorMessage(res) << std::endl;
            PQclear(res);
            return record;
        }
        
        int count = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        
        if (count > 0) {
            std::string query = "UPDATE devices SET name = '" + record.name + 
                                "', kind = '" + normalizeLabel(record.kind) +
                                "', uri = '" + record.uri +
                                "', is_primary = " + (record.isPrimary ? "TRUE" : "FALSE") +
                                ", enabled = " + (record.enabled ? "TRUE" : "FALSE") +
                                ", metadata = '" + record.metadata +
                                "', ip_address = '" + record.ipAddress +
                                "', mac_address = '" + record.macAddress +
                                "', manufacturer = '" + record.manufacturer +
                                "' WHERE id = " + std::to_string(record.id) + 
                                " RETURNING id;";
                                
            res = PQexec(db_, query.c_str());
            if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                std::cerr << "PostgreSQL update failed: " << PQresultErrorMessage(res) << std::endl;
                PQclear(res);
                return record;
            }
            
            if (PQntuples(res) > 0) {
                result.id = getInteger(res, 0, 0);
            }
            PQclear(res);
        } else {
            std::string query = "INSERT INTO devices(id, name, kind, uri, is_primary, enabled, metadata, ip_address, mac_address, manufacturer) VALUES(" +
                    std::to_string(record.id) + ", '" +
                    record.name + "', '" + 
                    normalizeLabel(record.kind) + "', '" +
                    record.uri + "', " +
                    (record.isPrimary ? "1" : "0") + ", " +
                    (record.enabled ? "1" : "0") + ", '" +
                    record.metadata + "', '" +
                    record.ipAddress + "', '" +
                    record.macAddress + "', '" +
                    record.manufacturer + "') RETURNING id;";
                            
            res = PQexec(db_, query.c_str());
            if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                std::cerr << "PostgreSQL insert failed: " << PQresultErrorMessage(res) << std::endl;
                std::cerr << "Query was: " << query << std::endl;
                PQclear(res);
                return record;
            }
            
            if (PQntuples(res) > 0) {
                result.id = getInteger(res, 0, 0);
            }
            PQclear(res);
        }
    } else {
        std::string query = "INSERT INTO devices(name, kind, uri, is_primary, enabled, metadata, ip_address, mac_address, manufacturer) VALUES('" +
                            record.name + "', '" + 
                            normalizeLabel(record.kind) + "', '" +
                            record.uri + "', " +
                            (record.isPrimary ? "1" : "0") + ", " +
                            (record.enabled ? "1" : "0") + ", '" +
                            record.metadata + "', '" +
                            record.ipAddress + "', '" +
                            record.macAddress + "', '" +
                            record.manufacturer + "') RETURNING id;";
                            
        res = PQexec(db_, query.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cerr << "PostgreSQL insert failed: " << PQresultErrorMessage(res) << std::endl;
            std::cerr << "Query was: " << query << std::endl;
            PQclear(res);
            return record;
        }
        
        std::cout << "PostgreSQL insert successful" << std::endl;
        if (PQntuples(res) > 0) {
            result.id = getInteger(res, 0, 0);
            std::cout << "Assigned ID: " << result.id << std::endl;
        }
        PQclear(res);
    }
    
    if (record.id > 0 && result.id == record.id) {
        std::string seqQuery = "SELECT setval('devices_id_seq', (SELECT GREATEST(MAX(id), " + 
                              std::to_string(record.id) + ") FROM devices));";
        std::cout << "Updating sequence with query: " << seqQuery << std::endl;
        res = PQexec(db_, seqQuery.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cerr << "PostgreSQL sequence update failed: " << PQresultErrorMessage(res) << std::endl;
        }
        PQclear(res);
    }

    if (record.isPrimary && result.id > 0) {
        std::string query = "UPDATE devices SET is_primary = CASE WHEN id = " + 
                            std::to_string(result.id) + " THEN 1 ELSE 0 END;";
        res = PQexec(db_, query.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "PostgreSQL primary update failed: " << PQresultErrorMessage(res) << std::endl;
        }
        PQclear(res);
    }

    if (result.id > 0) {
        std::string query = "SELECT id, name, kind, uri, is_primary, enabled, COALESCE(metadata, '{}'::jsonb), ip_address, mac_address, manufacturer, EXTRACT(EPOCH FROM created_at)::BIGINT, EXTRACT(EPOCH FROM updated_at)::BIGINT FROM devices WHERE id = " + 
                            std::to_string(result.id) + " LIMIT 1;";
        res = PQexec(db_, query.c_str());
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
            result = mapRow(res, 0);
        }
        PQclear(res);
    }

    return result;
}

bool DeviceRegistry::removeDevice(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) {
        return false;
    }

    std::string query = "DELETE FROM devices WHERE id = " + std::to_string(id) + ";";
    PGresult* res = PQexec(db_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "PostgreSQL delete failed: " << PQresultErrorMessage(res) << std::endl;
        PQclear(res);
        return false;
    }
    
    int affectedRows = atoi(PQcmdTuples(res));
    PQclear(res);
    return affectedRows > 0;
}

bool DeviceRegistry::updateDeviceStatus(int id, bool active)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) {
        return false;
    }

    std::string query = "UPDATE devices SET enabled = " + std::string(active ? "1" : "0") + " WHERE id = " + std::to_string(id) + ";";
    PGresult* res = PQexec(db_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "PostgreSQL update failed: " << PQresultErrorMessage(res) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool DeviceRegistry::updateLastSeen(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) {
        return false;
    }

    std::string query = "UPDATE devices SET updated_at = CURRENT_TIMESTAMP WHERE id = " + std::to_string(id) + ";";
    PGresult* res = PQexec(db_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "PostgreSQL update failed: " << PQresultErrorMessage(res) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool DeviceRegistry::setPrimaryDevice(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) {
        return false;
    }

    PGresult* res = PQexec(db_, "UPDATE devices SET is_primary = FALSE;");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "PostgreSQL reset primary failed: " << PQresultErrorMessage(res) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::string query = "UPDATE devices SET is_primary = TRUE WHERE id = " + std::to_string(id) + ";";
    res = PQexec(db_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "PostgreSQL set primary failed: " << PQresultErrorMessage(res) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

void DeviceRegistry::ensureSchema()
{
    if (!db_) {
        return;
    }

    execSql(db_, kCreateTableSql);
    execSql(db_, kCreatePrimaryIndexSql);
    execSql(db_, kCreateUriIndexSql);
    execSql(db_, kCreateIpIndexSql);
}

DeviceRecord DeviceRegistry::mapRow(PGresult* result, int row) const
{
    DeviceRecord record;
    record.id = getInteger(result, row, 0);
    record.name = getText(result, row, 1);
    std::string kindStr = getText(result, row, 2);
    record.kind = kindFromLabel(kindStr);
    record.uri = getText(result, row, 3);
    record.isPrimary = getInteger(result, row, 4) != 0;
    record.enabled = getInteger(result, row, 5) != 0;
    record.metadata = getText(result, row, 6);
    record.ipAddress = getText(result, row, 7);
    record.macAddress = getText(result, row, 8);
    record.manufacturer = getText(result, row, 9);
    
    // Handle new timestamp fields if they exist in the result
    if (PQnfields(result) > 10) {
        record.createdAt = getInteger(result, row, 10);
    }
    if (PQnfields(result) > 11) {
        record.updatedAt = getInteger(result, row, 11);
    }
    
    return record;
}

std::vector<DeviceRecord> DeviceRegistry::queryAll(const std::string& sql) const
{
    std::vector<DeviceRecord> rows;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) {
        return rows;
    }

    PGresult* res = PQexec(db_, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "PostgreSQL query failed: " << PQresultErrorMessage(res) << std::endl;
        PQclear(res);
        return rows;
    }

    int numRows = PQntuples(res);
    for (int i = 0; i < numRows; i++) {
        rows.push_back(mapRow(res, i));
    }

    PQclear(res);
    return rows;
}

}