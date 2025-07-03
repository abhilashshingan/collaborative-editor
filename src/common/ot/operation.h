// FILE: include/common/ot/operation.h
// Description: Enhanced Operation base class with undo/redo capabilities

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <optional>

namespace collab {
namespace ot {

// Forward declaration
class Operation;
using OperationPtr = std::shared_ptr<Operation>;

/**
 * Enumeration of operation sources
 * Used to track the origin of operations for undo/redo management
 */
enum class OperationSource {
    LOCAL,       // Operation originated from the local user
    REMOTE,      // Operation came from a remote user
    LOCAL_UNDO,  // Operation is an undo of a local operation
    LOCAL_REDO,  // Operation is a redo of a previously undone operation
    SYSTEM       // Operation originated from the system (not user-initiated)
};

/**
 * Base Operation class for Operational Transformation
 * All operations must be able to:
 * 1. Apply their changes to a document
 * 2. Transform against other operations
 * 3. Compute their inverse operation for undo functionality
 */
class Operation {
public:
    virtual ~Operation() = default;
    
    /**
     * Apply this operation to the given document
     * 
     * @param document The document to apply the operation to
     * @return true if successful, false otherwise
     */
    virtual bool apply(std::string& document) const = 0;
    
    /**
     * Transform this operation against another operation
     * 
     * @param other The operation to transform against
     * @return A new operation that has been transformed
     */
    virtual OperationPtr transform(const OperationPtr& other) const = 0;
    
    /**
     * Create an inverse operation that undoes this operation
     * 
     * @return An operation that will undo this operation
     */
    virtual OperationPtr inverse() const = 0;
    
    /**
     * Creates a deep copy of this operation
     * 
     * @return A new operation that is a copy of this one
     */
    virtual OperationPtr clone() const = 0;
    
    /**
     * Serializes this operation to a JSON string
     * 
     * @return JSON string representation of this operation
     */
    virtual std::string serialize() const = 0;
    
    /**
     * Get the type of this operation for type checking
     * 
     * @return String identifier for operation type
     */
    virtual std::string getType() const = 0;
    
    /**
     * Set the source of this operation
     * 
     * @param source The source of the operation
     */
    void setSource(OperationSource source) {
        source_ = source;
    }
    
    /**
     * Get the source of this operation
     * 
     * @return The source of the operation
     */
    OperationSource getSource() const {
        return source_;
    }
    
    /**
     * Set the related operation ID
     * This is useful for tracking the original operation that an undo/redo relates to
     * 
     * @param id The ID of the related operation
     */
    void setRelatedOperationId(int64_t id) {
        relatedOperationId_ = id;
    }
    
    /**
     * Get the related operation ID
     * 
     * @return The ID of the related operation, if any
     */
    std::optional<int64_t> getRelatedOperationId() const {
        return relatedOperationId_;
    }
    
    /**
     * Set the unique ID for this operation
     * 
     * @param id The unique ID for this operation
     */
    void setId(int64_t id) {
        id_ = id;
    }
    
    /**
     * Get the unique ID for this operation
     * 
     * @return The unique ID
     */
    int64_t getId() const {
        return id_;
    }

private:
    OperationSource source_ = OperationSource::LOCAL; // Default to local operation
    std::optional<int64_t> relatedOperationId_; // ID of related operation (for undo/redo)
    int64_t id_ = 0; // Unique identifier for this operation
};

/**
 * Insert text operation
 */
class InsertOperation : public Operation {
public:
    InsertOperation(size_t position, const std::string& text);
    
    bool apply(std::string& document) const override;
    OperationPtr transform(const OperationPtr& other) const override;
    OperationPtr inverse() const override;
    OperationPtr clone() const override;
    std::string serialize() const override;
    std::string getType() const override;
    
    // Accessors
    size_t getPosition() const { return position_; }
    const std::string& getText() const { return text_; }
    
private:
    size_t position_;
    std::string text_;
};

/**
 * Delete text operation
 */
class DeleteOperation : public Operation {
public:
    DeleteOperation(size_t position, size_t length);
    DeleteOperation(size_t position, size_t length, const std::string& deleted_text);
    
    bool apply(std::string& document) const override;
    OperationPtr transform(const OperationPtr& other) const override;
    OperationPtr inverse() const override;
    OperationPtr clone() const override;
    std::string serialize() const override;
    std::string getType() const override;
    
    // Accessors
    size_t getPosition() const { return position_; }
    size_t getLength() const { return length_; }
    const std::string& getDeletedText() const { return deleted_text_; }
    
private:
    size_t position_;
    size_t length_;
    std::string deleted_text_;
};

/**
 * Composite operation that combines multiple operations into one atomic unit
 * Useful for complex edits that should be undone/redone as a single operation
 */
class CompositeOperation : public Operation {
public:
    CompositeOperation() = default;
    
    /**
     * Add an operation to this composite
     * 
     * @param op Operation to add
     */
    void addOperation(const OperationPtr& op);
    
    /**
     * Get all operations in this composite
     * 
     * @return Vector of operations
     */
    const std::vector<OperationPtr>& getOperations() const { return operations_; }
    
    bool apply(std::string& document) const override;
    OperationPtr transform(const OperationPtr& other) const override;
    OperationPtr inverse() const override;
    OperationPtr clone() const override;
    std::string serialize() const override;
    std::string getType() const override;
    
private:
    std::vector<OperationPtr> operations_;
};

/**
 * Factory for creating operations from serialized representation
 */
class OperationFactory {
public:
    static OperationPtr deserialize(const std::string& json);
};

} // namespace ot
} // namespace collab