#include "config_manager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

namespace fs = std::filesystem;

namespace SnowOwl::Config {

ConfigManager::ConfigManager() {
    configPath_ = getDefaultConfigPath();
}

std::string ConfigManager::getDefaultConfigPath() const {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path) + "\\snowowl\\config.json";
    }
    return ".\\config.json";

#else
    const char* xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        return std::string(xdg) + "/snowowl/config.json";
    }

    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) home = pwd->pw_dir;
    }

    if (home) {
#ifdef __APPLE__
        return std::string(home) + "/Library/Application Support/snowowl/config.json";
#else
        return std::string(home) + "/.config/snowowl/config.json";
#endif
    }

    return "./config.json";
#endif
}

bool ConfigManager::createConfigDirectory() const {
    fs::path configPath(configPath_);
    fs::path configDir = configPath.parent_path();
    
    std::error_code ec;
    if (!fs::exists(configDir, ec)) {
        return fs::create_directories(configDir, ec);
    }
    return true;
}

bool ConfigManager::initialize() {
    if (!createConfigDirectory()) {
        return false;
    }
    
    if (!fs::exists(configPath_)) {
        config_ = nlohmann::json::object();
        config_["database_connections"] = nlohmann::json::object();
        config_["default_database_connection"] = "";
        return save();
    }
    
    return true;
}

bool ConfigManager::load() {
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        file >> config_;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::save() const {
    if (!createConfigDirectory()) {
        return false;
    }
    
    std::ofstream file(configPath_);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        file << config_.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::getConfigPath() const {
    return configPath_;
}

bool ConfigManager::setDefaultDatabaseConnection(const DatabaseConnection& connection) {
    return addDatabaseConnection(connection.name, connection) && 
           setDefaultDatabaseConnectionName(connection.name);
}

bool ConfigManager::addDatabaseConnection(const std::string& name, const DatabaseConnection& connection) {
    if (!load()) {
        if (!initialize()) {
            return false;
        }
        load(); 
    }
    
    nlohmann::json dbConn;
    dbConn["name"] = connection.name;
    dbConn["host"] = connection.host;
    dbConn["port"] = connection.port;
    dbConn["database"] = connection.database;
    dbConn["user"] = connection.user;
    dbConn["password"] = connection.password;
    
    config_["database_connections"][name] = dbConn;
    return save();
}

bool ConfigManager::removeDatabaseConnection(const std::string& name) {
    if (!load()) {
        return false;
    }
    
    if (config_.contains("database_connections") && 
        config_["database_connections"].contains(name)) {
        config_["database_connections"].erase(name);
        
        if (getDefaultDatabaseConnectionName() == name) {
            config_["default_database_connection"] = "";
        }
        
        return save();
    }
    
    return false;
}

DatabaseConnection ConfigManager::getDefaultDatabaseConnection() const {
    std::string defaultName = getDefaultDatabaseConnectionName();
    if (defaultName.empty()) {
        return DatabaseConnection{};
    }
    
    auto connections = getAllDatabaseConnections();
    auto it = connections.find(defaultName);
    if (it != connections.end()) {
        return it->second;
    }
    
    return DatabaseConnection{};
}

std::map<std::string, DatabaseConnection> ConfigManager::getAllDatabaseConnections() const {
    std::map<std::string, DatabaseConnection> connections;
    
    if (!const_cast<ConfigManager*>(this)->load()) {
        return connections;
    }
    
    if (!config_.contains("database_connections")) {
        return connections;
    }
    
    for (auto& [name, connJson] : config_["database_connections"].items()) {
        DatabaseConnection conn;
        conn.name = connJson.value("name", "");
        conn.host = connJson.value("host", "");
        conn.port = connJson.value("port", 5432);
        conn.database = connJson.value("database", "");
        conn.user = connJson.value("user", "");
        conn.password = connJson.value("password", "");
        connections[name] = conn;
    }
    
    return connections;
}

bool ConfigManager::setDefaultDatabaseConnectionName(const std::string& name) {
    if (!load()) {
        return false;
    }
    
    config_["default_database_connection"] = name;
    return save();
}

std::string ConfigManager::getDefaultDatabaseConnectionName() const {
    if (!const_cast<ConfigManager*>(this)->load()) {
        return "";
    }
    
    if (config_.contains("default_database_connection")) {
        return config_["default_database_connection"].get<std::string>();
    }
    
    return "";
}

void ConfigManager::set(const std::string& key, const nlohmann::json& value) {
    if (!load()) {
        initialize();
        load();
    }
    
    config_[key] = value;
    save();
}

nlohmann::json ConfigManager::get(const std::string& key) const {
    if (!const_cast<ConfigManager*>(this)->load()) {
        return nlohmann::json{};
    }
    
    if (config_.contains(key)) {
        return config_[key];
    }
    
    return nlohmann::json{};
}

bool ConfigManager::has(const std::string& key) const {
    if (!const_cast<ConfigManager*>(this)->load()) {
        return false;
    }
    
    return config_.contains(key);
}

std::string DatabaseConnection::toConnectionString() const {
    std::string connStr = "postgresql://" + user;
    if (!password.empty()) {
        connStr += ":" + password;
    }
    connStr += "@" + host;
    if (port != 5432) { 
        connStr += ":" + std::to_string(port);
    }
    connStr += "/" + database;
    return connStr;
}

DatabaseConnection DatabaseConnection::fromConnectionString(const std::string& connectionString) {
    DatabaseConnection conn;
    
    if (connectionString.substr(0, 13) == "postgresql://") {
        std::string remaining = connectionString.substr(13);
        
        size_t atPos = remaining.find('@');
        if (atPos != std::string::npos) {
            std::string userPass = remaining.substr(0, atPos);
            remaining = remaining.substr(atPos + 1);
            
            size_t colonPos = userPass.find(':');
            if (colonPos != std::string::npos) {
                conn.user = userPass.substr(0, colonPos);
                conn.password = userPass.substr(colonPos + 1);
            } else {
                conn.user = userPass;
            }
        }
        
        size_t slashPos = remaining.find('/');
        if (slashPos != std::string::npos) {
            std::string hostPort = remaining.substr(0, slashPos);
            conn.database = remaining.substr(slashPos + 1);
            
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                conn.host = hostPort.substr(0, colonPos);
                try {
                    conn.port = std::stoi(hostPort.substr(colonPos + 1));
                } catch (...) {
                    conn.port = 5432; 
                }
            } else {
                conn.host = hostPort;
            }
        }
    }
    
    return conn;
}

}
