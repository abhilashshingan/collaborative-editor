// FILE: src/client/session_manager.h
// Description: Client session manager with undo/redo functionality

#pragma once

#include "common/document/document_controller.h"
#include <memory>
#include <string>
#include <functional>

namespace collab {
namespace client {

/**
 * Manages the client's editing session with undo/redo support
 */
class SessionManager {
public:
    /**
     * Callback for operation sending
     */
    using SendOperationCallback = std::function<void(const std::string&, int64_t)>;
    
    /**
     * Constructor
     * 
     * @param userId ID of the local user
     */
    explicit SessionManager(const std::string& userId);
    
    /**
     * Get the document controller
     * 
     * @return Document controller
     */
    std::shared_ptr<DocumentController> getDocumentController();
    
    /**
     * Get the local user ID
     * 
     * @return User ID
     */
    const std::string& getUserId() const;
    
    /**
     * Set callback for sending operations to the server
     * 
     * @param callback Function to call for sending operations
     */
    void setSendOperationCallback(SendOperationCallback callback);
    
    /**
     * Handle a local operation (from user input)
     * 
     * @param op Operation to handle
     * @return true if the operation was applied successfully
     */
    bool handleLocalOperation(const ot::OperationPtr& op);
    
    /**
     * Handle a remote operation (from server)
     * 
     * @param op Operation to handle
     * @param userId ID of the user who created the operation
     * @return true if the operation was applied successfully
     */
    bool handleRemoteOperation(const ot::OperationPtr& op, const std::string& userId);
    
    /**
     * Undo the last local operation
     * 
     * @return true if an operation was undone
     */
    bool undo();
    
    /**
     * Redo a previously undone operation
     * 
     * @return true if an operation was redone
     */
    bool redo();
    
    /**
     * Get the current revision number
     * 
     * @return Current revision
     */
    int64_t getCurrentRevision() const;
    
    /**
     * Set the document's initial state
     * 
     * @param content Initial document content
     * @param revision Initial revision number
     */
    void setInitialState(const std::string& content, int64_t revision);

private:
    std::string userId_;
    std::shared_ptr<DocumentController> documentController_;
    SendOperationCallback sendCallback_;
    
    /**
     * Sends an operation to the server
     * 
     * @param op Operation to send
     */
    void sendOperation(const ot::OperationPtr& op);
};

} // namespace client
} // namespace collab