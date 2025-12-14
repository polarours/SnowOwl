#include "server_logger.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <ctime>

namespace SnowOwl::Server::Modules::Logging {

ServerLogger& ServerLogger::getInstance() {
    static ServerLogger instance;
    return instance;
}

ServerLogger::ServerLogger()
    : minLevel_(LogLevel::INFO)
    , consoleOutput_(true)
    , fileOutput_(false) {
}

ServerLogger::~ServerLogger() {
    if (logFile_ && logFile_->is_open()) {
        logFile_->close();
    }
}

bool ServerLogger::initialize(const std::string& logFilePath) {
    logFile_ = std::make_unique<std::ofstream>(logFilePath, std::ios::app);
    if (!logFile_->is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        return false;
    }
    
    fileOutput_ = true;
    info("Log system initialized");
    return true;
}

void ServerLogger::log(LogLevel level, const std::string& message) {
    if (level >= minLevel_) {
        writeLog(level, message);
    }
}

void ServerLogger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void ServerLogger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void ServerLogger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void ServerLogger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void ServerLogger::setLogLevel(LogLevel level) {
    minLevel_ = level;
}

void ServerLogger::setConsoleOutput(bool enabled) {
    consoleOutput_ = enabled;
}

void ServerLogger::setFileOutput(bool enabled) {
    fileOutput_ = enabled;
}

std::string ServerLogger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

std::string ServerLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void ServerLogger::writeLog(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    if (consoleOutput_) {
        std::cout << formattedMessage << std::endl;
    }
    
    if (fileOutput_ && logFile_ && logFile_->is_open()) {
        *logFile_ << formattedMessage << std::endl;
        logFile_->flush();
    }
}

}