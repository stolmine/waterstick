#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace WaterStick {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    ERROR = 2
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void startNewSession() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Close existing file and reopen to clear it
        if (logFile_.is_open()) {
            logFile_.close();
        }

        logFile_.open("/tmp/waterstick_debug.log", std::ios::trunc);
        if (logFile_.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            logFile_ << "======================================" << std::endl;
            logFile_ << "WaterStick VST3 Debug Session Started" << std::endl;
            logFile_ << "Time: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
            logFile_ << "======================================" << std::endl;
            logFile_.flush();
        }
    }

    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!logFile_.is_open()) {
            logFile_.open("/tmp/waterstick_debug.log", std::ios::app);
            if (!logFile_.is_open()) {
                return;
            }
        }

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

        const char* levelStr = "";
        switch (level) {
            case LogLevel::DEBUG: levelStr = "DEBUG"; break;
            case LogLevel::INFO:  levelStr = "INFO"; break;
            case LogLevel::ERROR: levelStr = "ERROR"; break;
        }

        logFile_ << "[" << timestamp.str() << "] [" << levelStr << "] " << message << std::endl;
        logFile_.flush();
    }

    void logParameterValue(int paramId, const std::string& paramName, double value) {
        std::ostringstream oss;
        oss << "PARAM[" << paramId << "] " << paramName << " = " << std::fixed
            << std::setprecision(6) << value;
        log(LogLevel::INFO, oss.str());
    }

    void logParameterContext(const std::string& context, int paramId,
                           const std::string& paramName, double value) {
        std::ostringstream oss;
        oss << context << " - PARAM[" << paramId << "] " << paramName << " = "
            << std::fixed << std::setprecision(6) << value;
        log(LogLevel::INFO, oss.str());
    }

    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

private:
    Logger() = default;
    std::ofstream logFile_;
    std::mutex mutex_;
};

}

// Convenience macros for logging
#define WS_LOG_SESSION_START() WaterStick::Logger::getInstance().startNewSession()
#define WS_LOG_DEBUG(msg) WaterStick::Logger::getInstance().log(WaterStick::LogLevel::DEBUG, msg)
#define WS_LOG_INFO(msg) WaterStick::Logger::getInstance().log(WaterStick::LogLevel::INFO, msg)
#define WS_LOG_ERROR(msg) WaterStick::Logger::getInstance().log(WaterStick::LogLevel::ERROR, msg)

#define WS_LOG_PARAM(id, name, value) WaterStick::Logger::getInstance().logParameterValue(id, name, value)
#define WS_LOG_PARAM_CONTEXT(context, id, name, value) WaterStick::Logger::getInstance().logParameterContext(context, id, name, value)