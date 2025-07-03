#pragma once

#include "operation.h"
#include "undo_redo_manager.h"
#include <string>
#include <queue>
#include <mutex>
#include <functional>
#include <atomic>

namespace collab {
namespace ot {

/**
 * Class responsible for managing the document state
 * and operations, including undo/redo capabilities
 */
class DocumentManager {
public:
    /**
     * Constructor
     * 
     * @param initial_content Initial content of the document
     */
    DocumentManager(const std::string& initial_content = "");
    
    /**
     * Get the current document content
     * 
     * @return The current document content
     */
    std::string getContent() const;
    
    /**
     * Set the document content directly
     * This clears the undo/redo history
     * 
     * @param content The new content to set
     */
    void setContent(const std::string& content);
    
    /**
     * Apply a local operation to the document
     * The operation will be added to the undo stack
     * 
     * @param operation The local operation to apply
     * @return True if successful, false otherwise
     */
    bool applyLocalOperation(const OperationPtr& operation);
    
    /**
     * Apply a remote operation to the document
     * Remote operations are transformed against pending local operations
     * 
     * @param operation The remote operation to apply
     * @return True if successful, false otherwise
     */
    bool applyRemoteOperation(const OperationPtr& operation);
    
    /**
     * Undo the last local operation
     * 
     * @return True if an operation was undone, false otherwise
     */
    bool undo();
    
    /**
     * Redo a previously undone operation
     * 
     * @return True if an operation was redone, false otherwise
     */
    bool redo();
    
    /**
     * Register a callback function to be called when the document changes
     * 
     * @param callback Function to call when the document changes
     */
    void setDocumentChangeCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Register a callback function to be called when an operation is applied
     * 
     * @param callback Function to call when an operation is applied
     */
    void setOperationCallback(std::function<void(const OperationPtr&)> callback);
    
    /**
     * Check if undo is available
     * 
     * @return True if there are operations available to undo
     */
    bool canUndo() const;
    
    /**
     * Check if redo is available
     * 
     * @return True if there are operations available to redo
     */
    bool canRedo() const;
    
    /**
     * Get the number of operations that can be undone
     * 
     * @return Number of operations in the undo stack
     */
    size_t undoCount() const;
    
    /**
     * Get the number of operations that can be redone
     * 
     * @return Number of operations in the redo stack
     */
    size_t redoCount() const;
    
    /**
     * Generate a unique ID for operations
     * 
     * @return A unique ID
     */
    int64_t generateOperationId();
    
private:
    // The current document content
    std::string document_;
    
    // The undo/redo manager
    UndoRedoManager undo_redo_manager_;
    
    // Document change callback
    std::function<void(const std::string&)> document_change_callback_;
    
    // Operation callback
    std::function<void(const OperationPtr&)> operation_callback_;
    
    // Thread safety
    mutable std::mutex document_mutex_;
    
    // Operation counter for generating unique IDs
    std::atomic<int64_t> operation_counter_{0};
};

} // namespace ot
} // namespace collab