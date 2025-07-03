#pragma once

#include "history.h"
#include "operation.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <optional>

namespace collab {
namespace ot {

/**
 * Document editor that manages OT operations and history
 * Provides undo/redo functionality and handles remote operation integration
 */
class Editor {
public:
    /**
     * Callback type for when operations are generated
     * @param op The generated operation
     * @param version The document version the operation was generated against
     */
    using OperationCallback = std::function<void(const OperationPtr&, int64_t)>;
    
    /**
     * Callback for document content changes
     * @param content The new document content
     */
    using ContentCallback = std::function<void(const std::string&)>;
    
    /**
     * Constructor
     * 
     * @param initialContent Initial content of the document
     */
    explicit Editor(const std::string& initialContent = "");
    
    /**
     * Destructor
     */
    ~Editor() = default;
    
    // No copy
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    
    /**
     * Set callback for local operations
     * 
     * @param callback Function to call when local operations are generated
     */
    void setOperationCallback(OperationCallback callback);
    
    /**
     * Set callback for content changes
     * 
     * @param callback Function to call when document content changes
     */
    void setContentCallback(ContentCallback callback);
    
    /**
     * Insert text at the specified position
     * 
     * @param position Position to insert text
     * @param text Text to insert
     * @return True if successful
     */
    bool insert(size_t position, const std::string& text);
    
    /**
     * Delete text from the specified position
     * 
     * @param position Start position for deletion
     * @param length Number of characters to delete
     * @return True if successful
     */
    bool deleteText(size_t position, size_t length);
    
    /**
     * Handle a remote operation received from another client
     * 
     * @param operation Remote operation
     * @param sourceVersion Version the operation was created against
     * @return True if successful
     */
    bool handleRemoteOperation(const OperationPtr& operation, int64_t sourceVersion);
    
    /**
     * Undo the last local operation
     * 
     * @return True if an operation was undone
     */
    bool undo();
    
    /**
     * Redo the last undone operation
     * 
     * @return True if an operation was redone
     */
    bool redo();
    
    /**
     * Check if undo is possible
     * 
     * @return True if undo is possible
     */
    bool canUndo() const;
    
    /**
     * Check if redo is possible
     * 
     * @return True if redo is possible
     */
    bool canRedo() const;
    
    /**
     * Get the current document content
     * 
     * @return Current content
     */
    std::string getContent() const;
    
    /**
     * Get the current document version
     * 
     * @return Current version number
     */
    int64_t getVersion() const;
    
    /**
     * Save a snapshot of the current document state
     * 
     * @return Document state as a snapshot
     */
    DocumentState createSnapshot() const;
    
    /**
     * Restore from a snapshot
     * 
     * @param snapshot Document state to restore to
     * @return True if successful
     */
    bool restoreFromSnapshot(const DocumentState& snapshot);

private:
    /**
     * Forward content change to callback if set
     * 
     * @param content New content
     */
    void onContentChanged(const std::string& content);
    
    /**
     * Forward operation to callback if set
     * 
     * @param op Operation to forward
     * @param version Version the operation was created against
     */
    void notifyOperationGenerated(const OperationPtr& op, int64_t version);

private:
    History history_;                  // Document history
    OperationCallback opCallback_;     // Callback for operations
    ContentCallback contentCallback_;  // Callback for content changes
    mutable std::mutex mutex_;         // Mutex for thread safety
};

} // namespace ot
} // namespace collab