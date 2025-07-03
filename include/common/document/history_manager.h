// FILE: include/common/document/history_manager.h
// Description: Manages operation history for undo/redo functionality

#pragma once

#include "common/ot/operation.h"
#include <vector>
#include <memory>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <string>

namespace collab {

/**
 * Manages operation history for undo/redo functionality
 * Thread-safe history management for collaborative editing
 */
class HistoryManager {
public:
    /**
     * Constructor
     * 
     * @param maxHistorySize Maximum number of operations to keep in history (default: 1000)
     */
    explicit HistoryManager(size_t maxHistorySize = 1000);
    
    /**
     * Record a new operation that was executed
     * 
     * @param op Operation to record
     * @param userId ID of the user who performed the operation
     * @param clearRedoHistory Whether to clear the redo stack (default: true)
     */
    void recordOperation(const ot::OperationPtr& op, const std::string& userId, bool clearRedoHistory = true);
    
    /**
     * Undo the last operation for a specific user
     * 
     * @param userId ID of the user performing the undo
     * @return Inverse operation to apply, or nullptr if no operations to undo
     */
    ot::OperationPtr undo(const std::string& userId);
    
    /**
     * Redo a previously undone operation for a specific user
     * 
     * @param userId ID of the user performing the redo
     * @return Operation to apply for redo, or nullptr if no operations to redo
     */
    ot::OperationPtr redo(const std::string& userId);
    
    /**
     * Check if undo is available for a specific user
     * 
     * @param userId ID of the user
     * @return true if there are operations that can be undone
     */
    bool canUndo(const std::string& userId) const;
    
    /**
     * Check if redo is available for a specific user
     * 
     * @param userId ID of the user
     * @return true if there are operations that can be redone
     */
    bool canRedo(const std::string& userId) const;
    
    /**
     * Get number of operations that can be undone for a specific user
     * 
     * @param userId ID of the user
     * @return Count of undoable operations
     */
    size_t undoCount(const std::string& userId) const;
    
    /**
     * Get number of operations that can be redone for a specific user
     * 
     * @param userId ID of the user
     * @return Count of redoable operations
     */
    size_t redoCount(const std::string& userId) const;
    
    /**
     * Clear history for a specific user
     * 
     * @param userId ID of the user
     */
    void clearUserHistory(const std::string& userId);
    
    /**
     * Clear all history
     */
    void clearAllHistory();
    
    /**
     * Get the total number of operations in the history
     * 
     * @return Total operation count
     */
    size_t totalOperationCount() const;

private:
    // Maximum history size per user
    const size_t maxHistorySize_;
    
    // Maps users to their undo and redo stacks
    std::unordered_map<std::string, std::deque<ot::OperationPtr>> userUndoStacks_;
    std::unordered_map<std::string, std::deque<ot::OperationPtr>> userRedoStacks_;
    
    // Mutex for thread safety
    mutable std::mutex historyMutex_;
    
    // Trim history if it exceeds the maximum size
    void trimUserHistory(const std::string& userId);
};

} // namespace collab