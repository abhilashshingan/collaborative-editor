#include "common/util/config_loader.h"
#include <algorithm>
#include <cctype>
#include <regex>

namespace collab {
namespace util {

EditorMode editorModeFromString(const std::string& str) {
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    
    if (upperStr == "CODE") return EditorMode::CODE;
    if (upperStr == "MARKDOWN") return EditorMode::MARKDOWN;
    if (upperStr == "RICH_TEXT") return EditorMode::RICH_TEXT;
    return EditorMode::TEXT; // Default
}

std::string editorModeToString(EditorMode mode) {
    switch (mode) {
        case EditorMode::CODE: return "CODE";
        case EditorMode::MARKDOWN: return "MARKDOWN";
        case EditorMode::RICH_TEXT: return "RICH_TEXT";
        default: return "TEXT";
    }
}

ConfigLoader::ConfigLoader() {
    // Initialize with default values
    setValue(PORT_KEY, std::to_string(DEFAULT_PORT));
    setValue(EDITOR_MODE_KEY, editorModeToString(DEFAULT_EDITOR_MODE));
    setValue(AUTOSAVE_INTERVAL_KEY, std::to_string(DEFAULT_AUTOSAVE_INTERVAL.count()));
}

bool ConfigLoader::loadFromFile(const std::filesystem::path& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(configFile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        parseLine(line);
    }
    
    return true;
}

bool ConfigLoader::saveToFile(const std::filesystem::path& configFilePath) const {
    std::ofstream configFile(configFilePath);
    if (!configFile.is_open()) {
        return false;
    }
    
    configFile << "# CollabEdit Configuration File\n";
    configFile << "# Generated on " << std::chrono::system_clock::now() << "\n\n";
    
    // Save all configuration values
    for (const auto& [key, value] : configValues_) {
        configFile << key << "=" << value << "\n";
    }
    
    return configFile.good();
}

uint16_t ConfigLoader::getServerPort() const {
    const auto& portStr = getValue(PORT_KEY).value_or(std::to_string(DEFAULT_PORT));
    
    try {
        uint16_t port = static_cast<uint16_t>(std::stoi(portStr));
        return port;
    } catch (const std::exception& e) {
        // Log error and return default
        std::cerr << "Error parsing port value: " << e.what() << std::endl;
        return DEFAULT_PORT;
    }
}

void ConfigLoader::setServerPort(uint16_t port) {
    setValue(PORT_KEY, std::to_string(port));
}

EditorMode ConfigLoader::getEditorMode() const {
    const auto& modeStr = getValue(EDITOR_MODE_KEY).value_or(editorModeToString(DEFAULT_EDITOR_MODE));
    return editorModeFromString(modeStr);
}

void ConfigLoader::setEditorMode(EditorMode mode) {
    setValue(EDITOR_MODE_KEY, editorModeToString(mode));
}

std::chrono::seconds ConfigLoader::getAutosaveInterval() const {
    const auto& intervalStr = getValue(AUTOSAVE_INTERVAL_KEY).value_or(
                                std::to_string(DEFAULT_AUTOSAVE_INTERVAL.count()));
    
    try {
        auto seconds = std::chrono::seconds(std::stoi(intervalStr));
        return seconds;
    } catch (const std::exception& e) {
        // Log error and return default
        std::cerr << "Error parsing autosave interval: " << e.what() << std::endl;
        return DEFAULT_AUTOSAVE_INTERVAL;
    }
}

void ConfigLoader::setAutosaveInterval(std::chrono::seconds seconds) {
    setValue(AUTOSAVE_INTERVAL_KEY, std::to_string(seconds.count()));
}

std::optional<std::string> ConfigLoader::getValue(const std::string& key) const {
    auto it = configValues_.find(key);
    if (it != configValues_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ConfigLoader::setValue(const std::string& key, const std::string& value) {
    configValues_[key] = value;
}

void ConfigLoader::parseLine(const std::string& line) {
    // Match KEY=VALUE pattern, allowing for spaces around the = sign
    std::regex configPattern(R"(^\s*([A-Za-z][A-Za-z0-9_]*)\s*=\s*(.*)$)");
    std::smatch matches;
    
    if (std::regex_match(line, matches, configPattern) && matches.size() == 3) {
        std::string key = matches[1].str();
        std::string value = trim(matches[2].str());
        
        // Remove quotes if present
        if ((value.front() == '"' && value.back() == '"') || 
            (value.front() == '\'' && value.back() == '\'')) {
            value = value.substr(1, value.length() - 2);
        }
        
        setValue(key, value);
    }
}

std::string ConfigLoader::trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    
    return (start < end) ? std::string(start, end) : std::string();
}

} // namespace util
} // namespace collab