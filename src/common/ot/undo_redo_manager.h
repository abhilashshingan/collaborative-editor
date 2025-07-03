#pragma once

#include "operation.h"
#include <deque>
#include <vector>
#include <optional>
#include <memory>
#include <functional>
#include <mutex>

namespace collab {
namespace ot {

/**
 * Class responsible for managing the history of operations for undo/redo functionality
 * Thread-safe implementation
 */
class UndoRedoManager {
public:
    UndoRedoManager(size_t max_history_size = 100);
    
    /**
     * Add an operation to the history
     * 
     * @param op Operation to add to the history
     */
    void addOperation(const OperationPtr& op);
    
    /**
     * Create and execute an undo operation for the most recent local operation
     * 
     * @param document The current document state
     * @return An optional containing the undo operation if available, empty otherwise
     */
    std::optional<OperationPtr> undo(std::string& document);
    
    /**
     * Create and execute a redo operation for the most recently undone operation
     * 
     * @param document The current document state
     * @return An optional containing the redo operation if available, empty otherwise
     */
    std::optional<OperationPtr> redo(std::string& document);
    
    /**
     * Reset the history
     * Useful when loading a new document
     */
    void clear();
    
    /**
     * Get the number of available undo operations
     * 
     * @return Number of operations that can be undone
     */
    size_t undoCount() const;
    
    /**
     * Get the number of available redo operations
     * 
     * @return Number of operations that can be redone
     */
    size_t redoCount() const;
    
    /**
     * Transform all history operations against a new operation
     * Used when receiving remote operations to maintain consistency
     * 
     * @param op The operation to transform against
     */
    void transformHistory(const OperationPtr& op);
    
    /**
     * Set a callback to be invoked when an operation is executed
     * 
     * @param callback Function to call when operations are executed
     */
    void setOperationCallback(std::function<void(const OperationPtr&)> callback);
    
private:
    // Maximum number of operations to keep in history
    size_t max_history_size_;
    
    // Undo stack (operations that can be undone)
    std::deque<OperationPtr> undo_stack_;
    
    // Redo stack (operations that can be redone)
    std::deque<OperationPtr> redo_stack_;
    
    // Callback for when operations are executed
    std::function<void(const OperationPtr&)> operation_callback_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper method to remove oldest history entries when exceeding max size
    void trimUndoStack();
};

} // namespace ot
} // namespace collab