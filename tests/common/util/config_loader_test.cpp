#include "common/util/config_loader.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace {

using namespace collab::util;

class ConfigLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary config file for testing
        configPath = std::filesystem::temp_directory_path() / "collabedit_test_config.env";
        
        std::ofstream configFile(configPath);
        configFile << "# Test Configuration\n"
                   << "SERVER_PORT=9090\n"
                   << "EDITOR_MODE=CODE\n"
                   << "AUTOSAVE_INTERVAL_SECONDS=60\n"
                   << "CUSTOM_SETTING=\"Custom Value\"\n";
        configFile.close();
    }
    
    void TearDown() override {
        // Remove the temporary config file
        std::filesystem::remove(configPath);
    }
    
    std::filesystem::path configPath;
    ConfigLoader config;
};

TEST_F(ConfigLoaderTest, DefaultValues) {
    ConfigLoader defaultConfig;
    EXPECT_EQ(defaultConfig.getServerPort(), 8080);
    EXPECT_EQ(defaultConfig.getEditorMode(), EditorMode::TEXT);
    EXPECT_EQ(defaultConfig.getAutosaveInterval(), std::chrono::seconds(30));
}

TEST_F(ConfigLoaderTest, LoadFromFile) {
    ASSERT_TRUE(config.loadFromFile(configPath));
    
    EXPECT_EQ(config.getServerPort(), 9090);
    EXPECT_EQ(config.getEditorMode(), EditorMode::CODE);
    EXPECT_EQ(config.getAutosaveInterval(), std::chrono::seconds(60));
    EXPECT_EQ(config.getValue("CUSTOM_SETTING"), "Custom Value");
}

TEST_F(ConfigLoaderTest, SaveToFile) {
    ConfigLoader newConfig;
    newConfig.setServerPort(7070);
    newConfig.setEditorMode(EditorMode::MARKDOWN);
    newConfig.setAutosaveInterval(std::chrono::seconds(120));
    newConfig.setValue("NEW_SETTING", "New Value");
    
    // Save to a different file
    auto savePath = std::filesystem::temp_directory_path() / "collabedit_save_test.env";
    ASSERT_TRUE(newConfig.saveToFile(savePath));
    
    // Load from the saved file
    ConfigLoader loadedConfig;
    ASSERT_TRUE(loadedConfig.loadFromFile(savePath));
    
    // Verify values
    EXPECT_EQ(loadedConfig.getServerPort(), 7070);
    EXPECT_EQ(loadedConfig.getEditorMode(), EditorMode::MARKDOWN);
    EXPECT_EQ(loadedConfig.getAutosaveInterval(), std::chrono::seconds(120));
    EXPECT_EQ(loadedConfig.getValue("NEW_SETTING"), "New Value");
    
    // Clean up
    std::filesystem::remove(savePath);
}

TEST_F(ConfigLoaderTest, EdgeCases) {
    // Create config file with edge cases
    auto edgePath = std::filesystem::temp_directory_path() / "collabedit_edge_test.env";
    std::ofstream edgeFile(edgePath);
    edgeFile << "# Edge case tests\n"
             << "SERVER_PORT=abc\n"  // Invalid port
             << "EDITOR_MODE=INVALID_MODE\n"  // Invalid mode
             << "AUTOSAVE_INTERVAL_SECONDS=-10\n"  // Negative interval
             << "  SPACES_KEY  =  Spaces Value  \n"  // Spaces around key/value
             << "EMPTY_VALUE=\n";  // Empty value
    edgeFile.close();
    
    ConfigLoader edgeConfig;
    ASSERT_TRUE(edgeConfig.loadFromFile(edgePath));
    
    // Invalid values should fall back to defaults
    EXPECT_EQ(edgeConfig.getServerPort(), 8080);
    EXPECT_EQ(edgeConfig.getEditorMode(), EditorMode::TEXT);
    EXPECT_EQ(edgeConfig.getAutosaveInterval().count(), -10); // This is a valid int but nonsensical
    
    // Check handling of spaces and empty values
    EXPECT_EQ(edgeConfig.getValue("SPACES_KEY"), "Spaces Value");
    EXPECT_EQ(edgeConfig.getValue("EMPTY_VALUE"), "");
    
    // Clean up
    std::filesystem::remove(edgePath);
}

TEST_F(ConfigLoaderTest, EditorModeConversion) {
    EXPECT_EQ(editorModeFromString("TEXT"), EditorMode::TEXT);
    EXPECT_EQ(editorModeFromString("CODE"), EditorMode::CODE);
    EXPECT_EQ(editorModeFromString("MARKDOWN"), EditorMode::MARKDOWN);
    EXPECT_EQ(editorModeFromString("RICH_TEXT"), EditorMode::RICH_TEXT);
    
    // Case insensitivity
    EXPECT_EQ(editorModeFromString("code"), EditorMode::CODE);
    EXPECT_EQ(editorModeFromString("MarkDown"), EditorMode::MARKDOWN);
    
    // Invalid values default to TEXT
    EXPECT_EQ(editorModeFromString("INVALID"), EditorMode::TEXT);
    
    // Round trip
    EXPECT_EQ(editorModeFromString(editorModeToString(EditorMode::CODE)), EditorMode::CODE);
}

} // namespace