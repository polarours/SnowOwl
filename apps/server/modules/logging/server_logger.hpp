#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <mutex>

namespace SnowOwl::Server::Modules::Logging {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class ServerLogger {
public:
    static ServerLogger& getInstance();
    
    bool initialize(const std::string& logFilePath);
    
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    
    void setLogLevel(LogLevel level);
    
    void setConsoleOutput(bool enabled);
    
    void setFileOutput(bool enabled);

private:
    ServerLogger();
    ~ServerLogger();
    
    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();
    void writeLog(LogLevel level, const std::string& message);
    
    std::unique_ptr<std::ofstream> logFile_;
    std::mutex mutex_;
    LogLevel minLevel_;
    bool consoleOutput_;
    bool fileOutput_;
};

#define LOG_DEBUG(msg) SnowOwl::Server::Modules::Logging::ServerLogger::getInstance().debug(msg)
#define LOG_INFO(msg) SnowOwl::Server::Modules::Logging::ServerLogger::getInstance().info(msg)
#define LOG_WARN(msg) SnowOwl::Server::Modules::Logging::ServerLogger::getInstance().warn(msg)
#define LOG_ERROR(msg) SnowOwl::Server::Modules::Logging::ServerLogger::getInstance().error(msg)

}