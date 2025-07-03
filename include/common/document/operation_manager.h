// FILE: include/common/document/operation_manager.h
// Description: Manages operation sequencing for collaborative editing

#pragma once

#include "common/ot/operation.h"
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace collab {

/**
 * Manages operations for collaborative editing
 * Handles operation sequencing, transformation, and conflict resolution
 */
class OperationManager {
public:
    /**
     * Constructor
     */
    OperationManager();
    
    /**
     * Process an incoming operation, transform it if necessary,
     * and return the operation to apply
     * 
     * @param op The incoming operation
     * @param clientId ID of the client that sent the operation
     * @param baseRevision The revision the operation was created on
     * @return Transformed operation ready for application
     */
    ot::OperationPtr processOperation(
        const ot::OperationPtr& op, 
        const std::string& clientId,
        int64_t baseRevision);
    
    /**
     * Record an applied operation
     * 
     * @param op The operation that was applied
     */
    void recordOperation(const ot::OperationPtr& op);
    
    /**
     * Get current revision number
     * 
     * @return Current revision
     */
    int64_t getCurrentRevision() const;

private:
    std::vector<ot::OperationPtr> operationHistory_;
    int64_t currentRevision_;
    std::unordered_map<std::string, int64_t> clientRevisions_;
    mutable std::mutex mutex_;
    
    /**
     * Transform an operation against all operations between
     * baseRevision and currentRevision
     * 
     * @param op Operation to transform
     * @param baseRevision Base revision of the operation
     * @return Transformed operation
     */
    ot::OperationPtr transformOperation(
        const ot::OperationPtr& op, 
        int64_t baseRevision);
};

} // namespace collab