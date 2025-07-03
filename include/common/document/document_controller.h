// FILE: include/common/document/document_controller.h
// Description: Controller for document operations with undo/redo support

#pragma once

#include "common/ot/operation.h"
#include "common/document/history_manager.h"
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>

namespace collab {

/**
 * Controller for document state and operations with undo/redo support
 */
class DocumentController {
public:
    /**
     * Callback for document changes
     * string: document content
     * int64_t: revision number
     */
    using DocumentChangeCallback = std::function<void(const std::string&, int64_t)>;
    
    /**
     * Constructor
     * 
     * @param initialContent Initial content of the document
     */
    explicit DocumentController(const std::string& initialContent = "");
    
    /**
     * Apply an operation to the document
     * 
     * @param op Operation to apply
     * @param userId ID of the user performing the operation
     * @param recordForUndo Whether to record this operation for undo (default: true)
     * @return true if successful, false otherwise
     */
    bool applyOperation(const ot::OperationPtr& op, const std::string& userId, bool recordForUndo = true);
    
    /**
     * Undo the last operation for a specific user
     * 
     * @param userId ID of the user performing the undo
     * @return true if successful, false if nothing to undo
     */
    bool undo(const std::string& userId);
    
    /**
     * Redo a previously undone operation for a specific user
     * 
     * @param userId ID of the user performing the redo
     * @return true if successful, false if nothing to redo
     */
    bool redo(const std::string& userId);
    
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
     * Get the current document text
     * 
     * @return Current document content
     */
    std::string getDocument() const;
    
    /**
     * Get the current revision number
     * 
     * @return Current revision
     */
    int64_t getRevision() const;
    
    /**
     * Register a callback for document changes
     * 
     * @param callback Function to call when document changes
     */
    void registerChangeCallback(DocumentChangeCallback callback);
    
    /**
     * Generate a unique operation ID
     * 
     * @return A unique ID for an operation
     */
    int64_t generateOperationId();
    
    /**
     * Transform an operation against all operations in history
     * to make it applicable to the current document state
     * 
     * @param op Operation to transform
     * @param baseRevision Revision on which the operation was created
     * @return Transformed operation
     */
    ot::OperationPtr transformOperation(const ot::OperationPtr& op, int64_t baseRevision);

private:
    std::string document_;
    HistoryManager historyManager_;
    std::vector<ot::OperationPtr> operationLog_;
    std::mutex documentMutex_;
    int64_t revision_;
    int64_t nextOperationId_;
    DocumentChangeCallback changeCallback_;
    
    // Notify about document changes
    void notifyDocumentChanged();
};

} // namespace collab