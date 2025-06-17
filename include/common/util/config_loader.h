#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <iostream>

namespace collab {
namespace util {

/**
 * @brief Available editor modes
 */
enum class EditorMode {
    TEXT,       ///< Plain text editing
    CODE,       ///< Source code editing with syntax highlighting
    MARKDOWN,   ///< Markdown editing with preview
    RICH_TEXT   ///< Rich text editing with formatting
};

/**
 * @brief Converts string to EditorMode
 * @param str The string representation of editor mode
 * @return The corresponding EditorMode value
 */
EditorMode editorModeFromString(const std::string& str);

/**
 * @brief Converts EditorMode to string
 * @param mode The editor mode
 * @return String representation of the editor mode
 */
std::string editorModeToString(EditorMode mode);

/**
 * @brief Configuration class that loads and provides access to application settings
 */
class ConfigLoader {
public:
    /**
     * @brief Default constructor that sets default values
     */
    ConfigLoader();
    
    /**
     * @brief Load configuration from a file
     * @param configFilePath Path to the configuration file
     * @return true if loading succeeded, false otherwise
     */
    bool loadFromFile(const std::filesystem::path& configFilePath);
    
    /**
     * @brief Save current configuration to a file
     * @param configFilePath Path to the configuration file
     * @return true if saving succeeded, false otherwise
     */
    bool saveToFile(const std::filesystem::path& configFilePath) const;
    
    /**
     * @brief Get the server port
     * @return The configured server port
     */
    uint16_t getServerPort() const;
    
    /**
     * @brief Set the server port
     * @param port The port number to set
     */
    void setServerPort(uint16_t port);
    
    /**
     * @brief Get the editor mode
     * @return The configured editor mode
     */
    EditorMode getEditorMode() const;
    
    /**
     * @brief Set the editor mode
     * @param mode The editor mode to set
     */
    void setEditorMode(EditorMode mode);
    
    /**
     * @brief Get the autosave interval in seconds
     * @return The configured autosave interval
     */
    std::chrono::seconds getAutosaveInterval() const;
    
    /**
     * @brief Set the autosave interval
     * @param seconds The autosave interval in seconds
     */
    void setAutosaveInterval(std::chrono::seconds seconds);
    
    /**
     * @brief Get a generic configuration value
     * @param key The configuration key
     * @return Optional value if the key exists
     */
    std::optional<std::string> getValue(const std::string& key) const;
    
    /**
     * @brief Set a generic configuration value
     * @param key The configuration key
     * @param value The value to set
     */
    void setValue(const std::string& key, const std::string& value);

private:
    std::unordered_map<std::string, std::string> configValues_;
    
    // Default values
    static constexpr uint16_t DEFAULT_PORT = 8080;
    static constexpr auto DEFAULT_EDITOR_MODE = EditorMode::TEXT;
    static constexpr auto DEFAULT_AUTOSAVE_INTERVAL = std::chrono::seconds{30};
    
    // Configuration keys
    static constexpr auto PORT_KEY = "SERVER_PORT";
    static constexpr auto EDITOR_MODE_KEY = "EDITOR_MODE";
    static constexpr auto AUTOSAVE_INTERVAL_KEY = "AUTOSAVE_INTERVAL_SECONDS";
    
    /**
     * @brief Parse a configuration line in KEY=VALUE format
     * @param line The line to parse
     */
    void parseLine(const std::string& line);
    
    /**
     * @brief Trim whitespace from both ends of a string
     * @param str The string to trim
     * @return The trimmed string
     */
    static std::string trim(const std::string& str);
};

} // namespace util
} // namespace collab