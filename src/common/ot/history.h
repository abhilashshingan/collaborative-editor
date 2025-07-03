#pragma once

#include "operation.h"
#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <mutex>

namespace collab {
namespace ot {

/**
 * Represents a document state at a specific point in time with version tracking
 */
struct DocumentState {
    std::string content;
    int64_t version;
    
    DocumentState(const std::string& content = "", int64_t version = 0)
        : content(content), version(version) {}
};

/**
 * Manages document history with undo/redo functionality
 * Thread-safe implementation for collaborative editing
 */
class History {
public:
    /**
     * Constructor
     * 
     * @param initialContent Initial document content
     */
    explicit History(const std::string& initialContent = "");
    
    /**
     * Destructor
     */
    ~History() = default;

    // Disallow copying
    History(const History&) = delete;
    History& operator=(const History&) = delete;
    
    /**
     * Apply a local operation to the document
     * 
     * @param operation The operation to apply
     * @return True if successful, false otherwise
     */
    bool applyLocal(const OperationPtr& operation);
    
    /**
     * Apply a remote operation to the document after transformation
     * 
     * @param operation The operation from remote source
     * @param sourceVersion The version against which the operation was created
     * @return True if successful, false otherwise
     */
    bool applyRemote(const OperationPtr& operation, int64_t sourceVersion);
    
    /**
     * Undo the last local operation
     * 
     * @return The inverse operation that was applied, or nullptr if undo not possible
     */
    OperationPtr undo();
    
    /**
     * Redo the last undone operation
     * 
     * @return The redone operation, or nullptr if redo not possible
     */
    OperationPtr redo();
    
    /**
     * Get current document content
     * 
     * @return Current content of the document
     */
    std::string getContent() const;
    
    /**
     * Get current document version
     * 
     * @return Current version number
     */
    int64_t getVersion() const;
    
    /**
     * Check if undo is possible
     * 
     * @return True if we can undo an operation
     */
    bool canUndo() const;
    
    /**
     * Check if redo is possible
     * 
     * @return True if we can redo an operation
     */
    bool canRedo() const;
    
    /**
     * Set callback for content change
     * 
     * @param callback Function to call when document changes
     */
    void setChangeCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Snapshot current document state
     * 
     * @return Current document state
     */
    DocumentState snapshot() const;
    
    /**
     * Restore document to a specific state
     * 
     * @param state The state to restore to
     * @return True if successful, false otherwise
     */
    bool restore(const DocumentState& state);

private:
    /**
     * Transform an operation against all operations in the specified queue
     * 
     * @param op Operation to transform
     * @param queue Queue of operations to transform against
     * @return Transformed operation
     */
    OperationPtr transformAgainstQueue(const OperationPtr& op, 
                                       const std::deque<OperationPtr>& queue) const;
    
    /**
     * Transform a remote operation against all needed operations
     * 
     * @param operation Remote operation to transform
     * @param sourceVersion Version the operation was created against
     * @return Transformed operation
     */
    OperationPtr transformRemoteOperation(const OperationPtr& operation, 
                                          int64_t sourceVersion) const;
    
    /**
     * Notify change listeners of document update
     */
    void notifyChangeListeners() const;

private:
    mutable std::mutex mutex_;                  // Mutex for thread safety
    std::string currentContent_;                // Current document content
    int64_t version_;                           // Current document version
    
    std::deque<OperationPtr> undoStack_;        // Operations that can be undone
    std::deque<OperationPtr> redoStack_;        // Operations that can be redone
    std::deque<OperationPtr> appliedOperations_; // All operations that have been applied
    
    std::function<void(const std::string&)> changeCallback_; // Callback for document changes
};

} // namespace ot
} // namespace collab