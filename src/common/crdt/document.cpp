#include "common/crdt/document.h"
#include <spdlog/spdlog.h>

namespace collab {
namespace crdt {

Document::Document() {
    // Initialize with empty content
    content_ = "";
}

std::string Document::getContent() const {
    return content_;
}

void Document::updateContent(const std::string& content) {
    content_ = content;
    spdlog::debug("Document content updated, new length: {}", content_.size());
}

void Document::insertCharacter(size_t position, char character, int siteId) {
    if (position > content_.size()) {
        position = content_.size();
    }
    
    // Log the operation for debugging
    spdlog::debug("Site {} inserting '{}' at position {}", siteId, character, position);
    
    // Insert the character at the specified position
    content_.insert(position, 1, character);
}

bool Document::deleteCharacter(size_t position) {
    if (position >= content_.size()) {
        return false;
    }
    
    // Log the operation for debugging
    spdlog::debug("Deleting character at position {}", position);
    
    // Delete the character at the specified position
    content_.erase(position, 1);
    return true;
}

size_t Document::size() const {
    return content_.size();
}

nlohmann::json Document::toJson() const {
    nlohmann::json j;
    j["content"] = content_;
    return j;
}

void Document::fromJson(const nlohmann::json& json) {
    content_ = json["content"].get<std::string>();
}

} // namespace crdt
} // namespace collab