#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <ctime>
#include <regex>

namespace collab {
namespace util {

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

/**
 * @brief A simple logger class with file rotation support
 */
class Logger {
public:
    /**
     * @brief Default constructor
     */
    Logger();
    
    /**
     * @brief Destructor that ensures log file is closed
     */
    ~Logger();

    /**
     * @brief Initialize the logger
     * @param logFilePath Path to the log file
     * @param minLevel Minimum level to log
     * @param consoleOutput Whether to also output to console
     * @return true if initialization succeeded
     */
    bool initialize(const std::filesystem::path& logFilePath, 
                    LogLevel minLevel = LogLevel::INFO,
                    bool consoleOutput = true);

    /**
     * @brief Log a message with specified severity
     * @param level Severity level
     * @param message The message to log
     */
    void log(LogLevel level, const std::string& message);

    /**
     * @brief Set the minimum log level
     * @param level The minimum level to log
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Get the current minimum log level
     * @return The current minimum log level
     */
    LogLevel getLogLevel() const;

    /**
     * @brief Check if a log level will be processed
     * @param level The level to check
     * @return true if the level will be logged
     */
    bool isLevelEnabled(LogLevel level) const;

    /**
     * @brief Log a trace message
     * @param message The message to log
     */
    void trace(const std::string& message);

    /**
     * @brief Log a debug message
     * @param message The message to log
     */
    void debug(const std::string& message);

    /**
     * @brief Log an info message
     * @param message The message to log
     */
    void info(const std::string& message);

    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void warning(const std::string& message);

    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void error(const std::string& message);

    /**
     * @brief Log a fatal message
     * @param message The message to log
     */
    void fatal(const std::string& message);

    /**
     * @brief Flush the log file
     */
    void flush();

private:
    /**
     * @brief Format the current time for logging
     * @return Formatted timestamp string
     */
    std::string formatTimestamp();

    /**
     * @brief Check if log file needs rotation and perform rotation if needed
     */
    void checkRotation();

    /**
     * @brief Rotate log files
     */
    void rotateLogFiles();

    // Configuration
    static constexpr size_t MAX_FILE_SIZE = 1024 * 1024; // 1MB
    static constexpr int MAX_BACKUP_FILES = 3;

    // State
    std::mutex mutex_;
    std::filesystem::path logFilePath_;
    std::ofstream logFile_;
    bool consoleOutput_ = true;
    LogLevel minLevel_ = LogLevel::INFO;
    bool initialized_ = false;
};

/**
 * @brief Get the global logger instance
 * @return Reference to the global logger
 */
Logger& getLogger();

/**
 * @brief Initialize the global logger
 * @param logFilePath Path to the log file
 * @param minLevel Minimum level to log
 * @param consoleOutput Whether to also output to console
 * @return true if initialization succeeded
 */
bool initLogger(const std::filesystem::path& logFilePath, 
                LogLevel minLevel = LogLevel::INFO,
                bool consoleOutput = true);

/**
 * @brief Converts LogLevel to string representation
 * @param level The log level
 * @return String representation of the level
 */
std::string logLevelToString(LogLevel level);

/**
 * @brief Converts string to LogLevel
 * @param str The string representation of log level
 * @return The corresponding LogLevel value
 */
LogLevel logLevelFromString(const std::string& str);

/**
 * @brief A simple logging utility for stream-style logging
 */
class LogStream {
public:
    /**
     * @brief Constructor that captures the log level and logger
     * @param logger Reference to the logger
     * @param level The log level for this stream
     */
    LogStream(Logger& logger, LogLevel level);

    /**
     * @brief Destructor that logs the accumulated message
     */
    ~LogStream();

    /**
     * @brief Stream operator for adding content to the log message
     * @tparam T Type of the value to log
     * @param value The value to add to the log
     * @return Reference to this LogStream
     */
    template<typename T>
    LogStream& operator<<(const T& value) {
        if (logger_.isLevelEnabled(level_)) {
            stream_ << value;
        }
        return *this;
    }

private:
    Logger& logger_;
    LogLevel level_;
    std::ostringstream stream_;
};

// Convenience macros for logging
#define LOG_TRACE   collab::util::LogStream(collab::util::getLogger(), collab::util::LogLevel::TRACE)
#define LOG_DEBUG   collab::util::LogStream(collab::util::getLogger(), collab::util::LogLevel::DEBUG)
#define LOG_INFO    collab::util::LogStream(collab::util::getLogger(), collab::util::LogLevel::INFO)
#define LOG_WARNING collab::util::LogStream(collab::util::getLogger(), collab::util::LogLevel::WARNING)
#define LOG_ERROR   collab::util::LogStream(collab::util::getLogger(), collab::util::LogLevel::ERROR)
#define LOG_FATAL   collab::util::LogStream(collab::util::getLogger(), collab::util::LogLevel::FATAL)

// Implementation of all inline functions to avoid linker errors

// Global logger instance
namespace {
    std::unique_ptr<Logger> g_logger = std::make_unique<Logger>();
}

inline Logger& getLogger() {
    return *g_logger;
}

inline bool initLogger(const std::filesystem::path& logFilePath, 
                LogLevel minLevel,
                bool consoleOutput) {
    return getLogger().initialize(logFilePath, minLevel, consoleOutput);
}

inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

inline LogLevel logLevelFromString(const std::string& str) {
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    
    if (upperStr == "TRACE")   return LogLevel::TRACE;
    if (upperStr == "DEBUG")   return LogLevel::DEBUG;
    if (upperStr == "INFO")    return LogLevel::INFO;
    if (upperStr == "WARNING") return LogLevel::WARNING;
    if (upperStr == "ERROR")   return LogLevel::ERROR;
    if (upperStr == "FATAL")   return LogLevel::FATAL;
    
    return LogLevel::INFO; // Default to INFO for unknown levels
}

inline Logger::Logger() : initialized_(false) {
}

inline Logger::~Logger() {
    flush();
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

inline bool Logger::initialize(const std::filesystem::path& logFilePath, 
                        LogLevel minLevel,
                        bool consoleOutput) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    logFilePath_ = logFilePath;
    consoleOutput_ = consoleOutput;
    minLevel_ = minLevel;
    
    try {
        // Create directory if it doesn't exist
        auto parentPath = logFilePath.parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
            std::filesystem::create_directories(parentPath);
        }
        
        // Open log file in append mode
        logFile_.open(logFilePath, std::ios::out | std::ios::app);
        if (!logFile_) {
            if (consoleOutput_) {
                std::cerr << "Failed to open log file: " << logFilePath << std::endl;
            }
            return false;
        }
        
        initialized_ = true;
        
        // Log initialization message
        std::ostringstream message;
        message << "Logger initialized with min level: " << logLevelToString(minLevel_);
        log(LogLevel::INFO, message.str());
        
        return true;
    } catch (const std::exception& e) {
        if (consoleOutput_) {
            std::cerr << "Error initializing logger: " << e.what() << std::endl;
        }
        return false;
    }
}

inline void Logger::log(LogLevel level, const std::string& message) {
    if (!isLevelEnabled(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        if (consoleOutput_) {
            std::cerr << "Logger not initialized" << std::endl;
        }
        return;
    }
    
    try {
        std::string timestamp = formatTimestamp();
        std::string levelStr = logLevelToString(level);
        
        std::ostringstream logLine;
        logLine << timestamp << " [" << std::left << std::setw(7) << levelStr << "] " << message;
        
        if (consoleOutput_) {
            // Color output for console (ANSI escape codes)
            const char* colorCode = "";
            const char* resetCode = "\033[0m";
            
            switch (level) {
                case LogLevel::TRACE:   colorCode = "\033[90m"; break; // Gray
                case LogLevel::DEBUG:   colorCode = "\033[37m"; break; // White
                case LogLevel::INFO:    colorCode = "\033[32m"; break; // Green
                case LogLevel::WARNING: colorCode = "\033[33m"; break; // Yellow
                case LogLevel::ERROR:   colorCode = "\033[31m"; break; // Red
                case LogLevel::FATAL:   colorCode = "\033[35m"; break; // Magenta
            }
            
            std::cout << colorCode << logLine.str() << resetCode << std::endl;
        }
        
        logFile_ << logLine.str() << std::endl;
        
        // Check if we need to rotate the log file
        checkRotation();
        
    } catch (const std::exception& e) {
        if (consoleOutput_) {
            std::cerr << "Error writing to log: " << e.what() << std::endl;
        }
    }
}

inline void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

inline LogLevel Logger::getLogLevel() const {
    return minLevel_;
}

inline bool Logger::isLevelEnabled(LogLevel level) const {
    return level >= minLevel_;
}

inline void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

inline void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

inline void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

inline void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

inline void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

inline void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

inline void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
    }
}

inline std::string Logger::formatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // Get milliseconds
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch() % std::chrono::seconds(1)).count();
    
    std::ostringstream timestamp;
    std::tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    timestamp << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") 
              << '.' << std::setfill('0') << std::setw(3) << milliseconds;
    
    return timestamp.str();
}

inline void Logger::checkRotation() {
    if (!initialized_ || !logFile_.is_open()) {
        return;
    }
    
    try {
        // Get the current size of the log file
        auto fileSize = std::filesystem::file_size(logFilePath_);
        
        if (fileSize >= MAX_FILE_SIZE) {
            // Close current log file
            logFile_.close();
            
            // Rotate log files
            rotateLogFiles();
            
            // Reopen the log file
            logFile_.open(logFilePath_, std::ios::out | std::ios::app);
            if (!logFile_) {
                initialized_ = false;
                if (consoleOutput_) {
                    std::cerr << "Failed to reopen log file after rotation" << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        if (consoleOutput_) {
            std::cerr << "Error checking log rotation: " << e.what() << std::endl;
        }
    }
}

inline void Logger::rotateLogFiles() {
    try {
        // Delete the oldest backup if it exists
        std::filesystem::path oldestBackup = logFilePath_;
        oldestBackup += "." + std::to_string(MAX_BACKUP_FILES);
        if (std::filesystem::exists(oldestBackup)) {
            std::filesystem::remove(oldestBackup);
        }
        
        // Shift existing backups
        for (int i = MAX_BACKUP_FILES - 1; i >= 1; --i) {
            std::filesystem::path oldPath = logFilePath_;
            oldPath += "." + std::to_string(i);
            
            std::filesystem::path newPath = logFilePath_;
            newPath += "." + std::to_string(i + 1);
            
            if (std::filesystem::exists(oldPath)) {
                std::filesystem::rename(oldPath, newPath);
            }
        }
        
        // Rename current log file to .1
        std::filesystem::path backupPath = logFilePath_;
        backupPath += ".1";
        std::filesystem::rename(logFilePath_, backupPath);
        
        if (consoleOutput_) {
            std::cout << "Rotated log file: " << logFilePath_ << std::endl;
        }
    } catch (const std::exception& e) {
        if (consoleOutput_) {
            std::cerr << "Error rotating log files: " << e.what() << std::endl;
        }
    }
}

inline LogStream::LogStream(Logger& logger, LogLevel level)
    : logger_(logger), level_(level) {
}

inline LogStream::~LogStream() {
    if (!stream_.str().empty()) {
        logger_.log(level_, stream_.str());
    }
}

} // namespace util
} // namespace collab