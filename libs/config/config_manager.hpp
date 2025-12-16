#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace SnowOwl::Config {

struct DatabaseConnection {
    std::string name;
    std::string host;
    int port = 5432;
    std::string database;
    std::string user;
    std::string password;
    
    std::string toConnectionString() const;
    static DatabaseConnection fromConnectionString(const std::string& connectionString);
};

class ConfigManager {
public:
    ConfigManager();
    
    bool initialize();
    bool load();
    bool save() const;
    
    bool setDefaultDatabaseConnection(const DatabaseConnection& connection);
    bool addDatabaseConnection(const std::string& name, const DatabaseConnection& connection);
    bool removeDatabaseConnection(const std::string& name);
    DatabaseConnection getDefaultDatabaseConnection() const;
    std::map<std::string, DatabaseConnection> getAllDatabaseConnections() const;
    bool setDefaultDatabaseConnectionName(const std::string& name);
    std::string getDefaultDatabaseConnectionName() const;
    
    void set(const std::string& key, const nlohmann::json& value);
    nlohmann::json get(const std::string& key) const;
    bool has(const std::string& key) const;
    
    std::string getConfigPath() const;
    
private:
    nlohmann::json config_;
    std::string configPath_;
    
    bool createConfigDirectory() const;
    std::string getDefaultConfigPath() const;
};

}
