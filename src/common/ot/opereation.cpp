#include "operation.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <algorithm>
#include <memory>

namespace collab {
namespace ot {

//
// InsertOperation Implementation
//

InsertOperation::InsertOperation(size_t position, const std::string& text)
    : position_(position), text_(text) {
}

bool InsertOperation::apply(std::string& document) const {
    if (position_ > document.length()) {
        return false;
    }
    
    document.insert(position_, text_);
    return true;
}

OperationPtr InsertOperation::transform(const OperationPtr& other) const {
    if (auto otherInsert = std::dynamic_pointer_cast<InsertOperation>(other)) {
        // Transform against another insert operation
        auto result = std::make_shared<InsertOperation>(*this);
        
        // If the other insert is before or at our position, shift our position right
        if (otherInsert->getPosition() <= position_) {
            result->position_ += otherInsert->getText().length();
        }
        
        return result;
    }
    else if (auto otherDelete = std::dynamic_pointer_cast<DeleteOperation>(other)) {
        // Transform against a delete operation
        auto result = std::make_shared<InsertOperation>(*this);
        
        size_t deleteEnd = otherDelete->getPosition() + otherDelete->getLength();
        
        if (otherDelete->getPosition() < position_) {
            // The delete affects our position
            if (deleteEnd <= position_) {
                // Delete is entirely before our position, shift left
                result->position_ -= otherDelete->getLength();
            } else {
                // Delete overlaps with or contains our position
                result->position_ = otherDelete->getPosition();
            }
        }
        
        return result;
    }
    
    // Unknown operation type, return a clone
    return clone();
}

OperationPtr InsertOperation::inverse() const {
    // The inverse of an insert is a delete of the same text
    return std::make_shared<DeleteOperation>(position_, text_.length(), text_);
}

OperationPtr InsertOperation::clone() const {
    return std::make_shared<InsertOperation>(position_, text_);
}

std::string InsertOperation::serialize() const {
    nlohmann::json j;
    j["type"] = "insert";
    j["position"] = position_;
    j["text"] = text_;
    return j.dump();
}

std::string InsertOperation::getType() const {
    return "insert";
}

//
// DeleteOperation Implementation
//

DeleteOperation::DeleteOperation(size_t position, size_t length)
    : position_(position), length_(length) {
}

DeleteOperation::DeleteOperation(size_t position, size_t length, const std::string& deleted_text)
    : position_(position), length_(length), deleted_text_(deleted_text) {
}

bool DeleteOperation::apply(std::string& document) const {
    if (position_ + length_ > document.length()) {
        return false;
    }
    
    // Store the deleted text if it wasn't provided
    if (deleted_text_.empty()) {
        const_cast<std::string&>(deleted_text_) = document.substr(position_, length_);
    }
    
    document.erase(position_, length_);
    return true;
}

OperationPtr DeleteOperation::transform(const OperationPtr& other) const {
    if (auto otherInsert = std::dynamic_pointer_cast<InsertOperation>(other)) {
        // Transform against an insert operation
        auto result = std::make_shared<DeleteOperation>(*this);
        
        // If the insert is before or at our position, shift our position right
        if (otherInsert->getPosition() <= position_) {
            result->position_ += otherInsert->getText().length();
        }
        // If the insert is in the middle of our delete range, we need to increase length
        else if (otherInsert->getPosition() < position_ + length_) {
            // No change to position, but increase length
            result->length_ += otherInsert->getText().length();
        }
        
        return result;
    }
    else if (auto otherDelete = std::dynamic_pointer_cast<DeleteOperation>(other)) {
        // Transform against a delete operation
        size_t otherStart = otherDelete->getPosition();
        size_t otherEnd = otherStart + otherDelete->getLength();
        size_t thisStart = position_;
        size_t thisEnd = thisStart + length_;
        
        // Case 1: Other delete is completely before this one
        if (otherEnd <= thisStart) {
            // Shift position left
            return std::make_shared<DeleteOperation>(
                thisStart - otherDelete->getLength(),
                length_,
                deleted_text_
            );
        }
        // Case 2: Other delete completely contains this one
        else if (otherStart <= thisStart && otherEnd >= thisEnd) {
            // This delete has been completely deleted by the other
            return std::make_shared<DeleteOperation>(otherStart, 0, "");
        }
        // Case 3: Other delete overlaps the beginning of this one
        else if (otherStart <= thisStart && otherEnd < thisEnd) {
            // Adjust position and length
            size_t newPosition = otherStart;
            size_t newLength = thisEnd - otherEnd;
            std::string newDeletedText = (deleted_text_.length() >= newLength) 
                ? deleted_text_.substr(deleted_text_.length() - newLength) 
                : "";
            
            return std::make_shared<DeleteOperation>(newPosition, newLength, newDeletedText);
        }
        // Case 4: Other delete overlaps the end of this one
        else if (otherStart > thisStart && otherStart < thisEnd && otherEnd >= thisEnd) {
            // Keep same position, reduce length
            size_t newLength = otherStart - thisStart;
            std::string newDeletedText = (deleted_text_.length() >= newLength) 
                ? deleted_text_.substr(0, newLength) 
                : "";
            
            return std::make_shared<DeleteOperation>(thisStart, newLength, newDeletedText);
        }
        // Case 5: Other delete is in the middle of this one
        else if (otherStart > thisStart && otherEnd < thisEnd) {
            // Reduce length by the other delete length
            size_t newLength = length_ - otherDelete->getLength();
            std::string newDeletedText;
            
            if (deleted_text_.length() >= length_) {
                // Create new deleted text by removing the middle part
                newDeletedText = deleted_text_.substr(0, otherStart - thisStart) + 
                                deleted_text_.substr(otherEnd - thisStart);
            }
            
            return std::make_shared<DeleteOperation>(thisStart, newLength, newDeletedText);
        }
    }
    
    // Unknown operation type or no transformation needed, return a clone
    return clone();
}

OperationPtr DeleteOperation::inverse() const {
    // The inverse of a delete is an insert of the deleted text
    if (deleted_text_.empty()) {
        throw std::runtime_error("Cannot invert delete operation without deleted text");
    }
    
    return std::make_shared<InsertOperation>(position_, deleted_text_);
}

OperationPtr DeleteOperation::clone() const {
    return std::make_shared<DeleteOperation>(position_, length_, deleted_text_);
}

std::string DeleteOperation::serialize() const {
    nlohmann::json j;
    j["type"] = "delete";
    j["position"] = position_;
    j["length"] = length_;
    j["text"] = deleted_text_;
    return j.dump();
}

std::string DeleteOperation::getType() const {
    return "delete";
}

//
// OperationFactory Implementation
//

OperationPtr OperationFactory::deserialize(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        std::string type = j["type"];
        
        if (type == "insert") {
            size_t position = j["position"];
            std::string text = j["text"];
            return std::make_shared<InsertOperation>(position, text);
        }
        else if (type == "delete") {
            size_t position = j["position"];
            size_t length = j["length"];
            std::string text = j.contains("text") ? j["text"].get<std::string>() : "";
            return std::make_shared<DeleteOperation>(position, length, text);
        }
        
        throw std::runtime_error("Unknown operation type: " + type);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error deserializing operation: ") + e.what());
    }
}

} // namespace ot
} // namespace collab