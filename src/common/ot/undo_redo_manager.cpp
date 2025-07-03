
#include "undo_redo_manager.h"
#include <algorithm>

namespace collab {
namespace ot {

UndoRedoManager::UndoRedoManager(size_t max_history_size) 
    : max_history_size_(max_history_size) {}

void UndoRedoManager::addOperation(const OperationPtr& op) {
    // Only local operations are added to the undo stack
    if (!op || op->getSource() != OperationSource::LOCAL) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear the redo stack when a new operation is added
    redo_stack_.clear();
    
    // Add the operation to the undo stack
    undo_stack_.push_back(op->clone());
    
    // Ensure we don't exceed the maximum history size
    trimUndoStack();
}

std::optional<OperationPtr> UndoRedoManager::undo(std::string& document) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if there are operations to undo
    if (undo_stack_.empty()) {
        return std::nullopt;
    }
    
    // Get the most recent operation
    OperationPtr op = undo_stack_.back();
    undo_stack_.pop_back();
    
    // Create an inverse operation
    OperationPtr inverse_op = op->inverse();
    
    // Mark it as an undo operation and link to original
    inverse_op->setSource(OperationSource::LOCAL_UNDO);
    inverse_op->setRelatedOperationId(op->getId());
    
    // Apply the inverse operation to the document
    if (!inverse_op->apply(document)) {
        // If application fails, put the original operation back
        undo_stack_.push_back(op);
        return std::nullopt;
    }
    
    // Add the original operation to the redo stack
    redo_stack_.push_back(op);
    
    // Trigger callback if set
    if (operation_callback_) {
        operation_callback_(inverse_op);
    }
    
    return inverse_op;
}

std::optional<OperationPtr> UndoRedoManager::redo(std::string& document) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if there are operations to redo
    if (redo_stack_.empty()) {
        return std::nullopt;
    }
    
    // Get the most recent undone operation
    OperationPtr op = redo_stack_.back();
    redo_stack_.pop_back();
    
    // Clone the operation for redo
    OperationPtr redo_op = op->clone();
    
    // Mark it as a redo operation and link to original
    redo_op->setSource(OperationSource::LOCAL_REDO);
    redo_op->setRelatedOperationId(op->getId());
    
    // Apply the redo operation to the document
    if (!redo_op->apply(document)) {
        // If application fails, put the original operation back
        redo_stack_.push_back(op);
        return std::nullopt;
    }
    
    // Add the operation back to the undo stack
    undo_stack_.push_back(op);
    
    // Trigger callback if set
    if (operation_callback_) {
        operation_callback_(redo_op);
    }
    
    return redo_op;
}

void UndoRedoManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    undo_stack_.clear();
    redo_stack_.clear();
}

size_t UndoRedoManager::undoCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return undo_stack_.size();
}

size_t UndoRedoManager::redoCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return redo_stack_.size();
}

void UndoRedoManager::transformHistory(const OperationPtr& op) {
    if (!op) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Transform all operations in the undo stack
    for (auto& undo_op : undo_stack_) {
        undo_op = undo_op->transform(op);
        
        // If transformation failed, we must clear the history as it's no longer valid
        if (!undo_op) {
            clear();
            return;
        }
    }
    
    // Transform all operations in the redo stack
    for (auto& redo_op : redo_stack_) {
        redo_op = redo_op->transform(op);
        
        // If transformation failed, we must clear the history as it's no longer valid
        if (!redo_op) {
            clear();
            return;
        }
    }
}

void UndoRedoManager::setOperationCallback(std::function<void(const OperationPtr&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    operation_callback_ = callback;
}

void UndoRedoManager::trimUndoStack() {
    while (undo_stack_.size() > max_history_size_) {
        undo_stack_.pop_front();
    }
}

} // namespace ot
} // namespace collab