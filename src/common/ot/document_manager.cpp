#include "document_manager.h"

namespace collab {
    namespace ot {
DocumentManager::DocumentManager(const std::string& initial_content) 
    : document_(initial_content) {
    
    // Set up the operation callback from the undo/redo manager
    undo_redo_manager_.setOperationCallback([this](const OperationPtr& op) {
        if (operation_callback_) {
            operation_callback_(op);
        }
    });
}

std::string DocumentManager::getContent() const {
    std::lock_guard<std::mutex> lock(document_mutex_);
    return document_;
}

void DocumentManager::setContent(const std::string& content) {
    std::lock_guard<std::mutex> lock(document_mutex_);
    document_ = content;
    
    // Clear undo/redo history as it's no longer valid
    undo_redo_manager_.clear();
    
    // Notify document change
    if (document_change_callback_) {
        document_change_callback_(document_);
    }
}

bool DocumentManager::applyLocalOperation(const OperationPtr& operation) {
    if (!operation) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(document_mutex_);
    
    // Set the operation source if not already set
    if (operation->getSource() != OperationSource::LOCAL_UNDO &&
        operation->getSource() != OperationSource::LOCAL_REDO) {
        operation->setSource(OperationSource::LOCAL);
    }
    
    // Set a unique ID if not already set
    if (operation->getId() == 0) {
        operation->setId(generateOperationId());
    }
    
    // Apply the operation
    if (!operation->apply(document_)) {
        return false;
    }
    
    // Only add to undo history if it's a local user operation
    // (not an undo/redo operation itself)
    if (operation->getSource() == OperationSource::LOCAL) {
        undo_redo_manager_.addOperation(operation);
    }
    
    // Notify document change
    if (document_change_callback_) {
        document_change_callback_(document_);
    }
    
    // Notify operation applied
    if (operation_callback_) {
        operation_callback_(operation);
    }
    
    return true;
}

bool DocumentManager::applyRemoteOperation(const OperationPtr& operation) {
    if (!operation) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(document_mutex_);
    
    // Mark as remote operation
    operation->setSource(OperationSource::REMOTE);
    
    // Apply the operation
    if (!operation->apply(document_)) {
        return false;
    }
    
    // Transform the history operations
    undo_redo_manager_.transformHistory(operation);
    
    // Notify document change
    if (document_change_callback_) {
        document_change_callback_(document_);
    }
    
    // Notify operation applied
    if (operation_callback_) {
        operation_callback_(operation);
    }
    
    return true;
}

bool DocumentManager::undo() {
    std::lock_guard<std::mutex> lock(document_mutex_);
    
    auto undo_op = undo_redo_manager_.undo(document_);
    if (!undo_op) {
        return false;
    }
    
    // Notify document change
    if (document_change_callback_) {
        document_change_callback_(document_);
    }
    
    return true;
}

bool DocumentManager::redo() {
    std::lock_guard<std::mutex> lock(document_mutex_);
    
    auto redo_op = undo_redo_manager_.redo(document_);
    if (!redo_op) {
        return false;
    }
    
    // Notify document change
    if (document_change_callback_) {
        document_change_callback_(document_);
    }
    
    return true;
}

void DocumentManager::setDocumentChangeCallback(std::function<void(const std::string&)> callback) {
    document_change_callback_ = callback;
}

void DocumentManager::setOperationCallback(std::function<void(const OperationPtr&)> callback) {
    operation_callback_ = callback;
}

bool DocumentManager::canUndo() const {
    return undo_redo_manager_.undoCount() > 0;
}

bool DocumentManager::canRedo() const {
    return undo_redo_manager_.redoCount() > 0;
}

size_t DocumentManager::undoCount() const {
    return undo_redo_manager_.undoCount();
}

size_t DocumentManager::redoCount() const {
    return undo_redo_manager_.redoCount();
}

int64_t DocumentManager::generateOperationId() {
    return ++operation_counter_;
}

}
}