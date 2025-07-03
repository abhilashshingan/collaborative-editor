#pragma once

#include "../common/ot/editor.h"
#include <string>
#include <mutex>
#include <optional>
#include <functional>
#include <deque>
#include <vector>

namespace collab {
namespace client {

/**
 * A client for editing documents with undo/redo functionality
 * Wraps the OT editor with network capabilities
 */
class DocumentClient {
public:
    /**
     * Callback for document content changes
     */
    using ContentCallback = std::function<void(const std::string&)>;
    
    /**
     * Callback for status messages
     */
    using StatusCallback = std::function<void(const std::string&)>;
    
    /**
     * Constructor
     * 
     * @param initialContent Initial document content
     */
    DocumentClient(const std::string& initialContent = "");
    
    /**
     * Destructor
     */
    ~DocumentClient() = default;
    
    /**
     * Set callbacks for document changes and status updates
     * 
     * @param contentCallback Called when document content changes
     * @param statusCallback Called when client status changes
     */
    void setCallbacks(ContentCallback contentCallback, StatusCallback statusCallback);
    
    /**
     * Connect to a collaborative editing server
     * 
     * @param host Server hostname
     * @param port Server port
     * @return True if connection successful
     */
    bool connect(const std::string& host, const std::string& port);
    
    /**
     * Disconnect from server
     */
    void disconnect();
    
    /**
     * Insert text at specified position
     * 
     * @param position Position to insert at
     * @param text Text to insert
     * @return True if successful
     */
    bool insert(size_t position, const std::string& text);
    
    /**
     * Delete text at specified position
     * 
     * @param position Position to delete from
     * @param length Length of text to delete
     * @return True if successful
     */
    bool deleteText(size_t position, size_t length);
    
    /**
     * Undo last operation
     * 
     * @return True if successful
     */
    bool undo();
    
    /**
     * Redo last undone operation
     * 
     * @return True if successful
     */
    bool redo();
    
    /**
     * Check if undo is available
     * 
     * @return True if undo is possible
     */
    bool canUndo() const;
    
    /**
     * Check if redo is available
     * 
     * @return True if redo is possible
     */
    bool canRedo() const;
    
    /**
     * Get current document content
     * 
     * @return Document content
     */
    std::string getContent() const;
    
    /**
     * Get client connection status
     * 
     * @return True if connected to server
     */
    bool isConnected() const;

private:
    /**
     * Handle operation generated locally
     * 
     * @param operation Operation to send
     * @param version Version operation was created against
     */
    void handleLocalOperation(const ot::OperationPtr& operation, int64_t version);
    
    /**
     * Process incoming operation from server
     * 
     * @param json JSON-serialized operation
     */
    void processRemoteOperation(const std::string& json);
    
    /**
     * Update status message
     * 
     * @param message Status message
     */
    void setStatus(const std::string& message);

private:
    ot::Editor editor_;                      // OT editor with history
    ContentCallback contentCallback_;        // Callback for document changes
    StatusCallback statusCallback_;          // Callback for status updates
    bool connected_;                         // Connection status
    std::mutex mutex_;                       // Mutex for thread safety
    std::deque<ot::OperationPtr> pending_;   // Pending operations to send
};

} // namespace client
} // namespace collab