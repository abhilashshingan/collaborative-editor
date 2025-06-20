#ifndef COLLABORATIVE_EDITOR_DOCUMENT_H
#define COLLABORATIVE_EDITOR_DOCUMENT_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace collab {
namespace crdt {

/**
 * Document class for CRDT-based collaborative text editing
 */
class Document {
public:
    Document();
    
    /**
     * Get the current content of the document as a string
     */
    std::string getContent() const;
    
    /**
     * Update the entire document content (simple implementation for now)
     * In a full CRDT implementation, this would use operations instead
     */
    void updateContent(const std::string& content);
    
    /**
     * Insert a character at a specific position
     *
     * @param position Position to insert at (0-based)
     * @param character Character to insert
     * @param siteId Unique identifier for the site (client) making the edit
     */
    void insertCharacter(size_t position, char character, int siteId);
    
    /**
     * Delete a character at a specific position
     *
     * @param position Position to delete from (0-based)
     */
    bool deleteCharacter(size_t position);
    
    /**
     * Get the current size of the document (number of characters)
     */
    size_t size() const;
    
    /**
     * Serialize the document to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * Deserialize the document from JSON
     */
    void fromJson(const nlohmann::json& json);

private:
    // Implementation details will be added as the project develops
    std::string content_;
};

} // namespace crdt
} // namespace collab

#endif // COLLABORATIVE_EDITOR_DOCUMENT_H