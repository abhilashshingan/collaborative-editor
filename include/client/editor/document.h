#ifndef COLLABORATIVE_EDITOR_DOCUMENT_H
#define COLLABORATIVE_EDITOR_DOCUMENT_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <optional>
#include <mutex>
#include <deque>
#include <functional>
#include <utility>

namespace collab {
namespace document {

/**
 * Types of operations that can be performed on a document
 */
enum class OperationType {
    INSERT,
    DELETE,
    REPLACE
};

/**
 * Represents a cursor position with line and column
 */
struct CursorPosition {
    size_t line;
    size_t column;
    
    // Constructors
    CursorPosition() : line(0), column(0) {}
    CursorPosition(size_t l, size_t c) : line(l), column(c) {}
    
    // Comparison operators
    bool operator==(const CursorPosition& other) const {
        return line == other.line && column == other.column;
    }
    
    bool operator!=(const CursorPosition& other) const {
        return !(*this == other);
    }
    
    bool operator<(const CursorPosition& other) const {
        return (line < other.line) || (line == other.line && column < other.column);
    }
    
    bool operator>(const CursorPosition& other) const {
        return other < *this;
    }
    
    bool operator<=(const CursorPosition& other) const {
        return *this < other || *this == other;
    }
    
    bool operator>=(const CursorPosition& other) const {
        return *this > other || *this == other;
    }
};

/**
 * Represents a text selection with start and end positions
 */
struct Selection {
    CursorPosition start;
    CursorPosition end;
    
    // Default constructor
    Selection() : start(), end() {}
    
    // Constructor with start and end positions
    Selection(const CursorPosition& s, const CursorPosition& e) 
        : start(s), end(e) {}
    
    // Check if selection is empty (start == end)
    bool isEmpty() const {
        return start == end;
    }
    
    // Check if selection is valid (start <= end)
    bool isValid() const {
        return start <= end;
    }
    
    // Normalize selection so that start <= end
    void normalize() {
        if (start > end) {
            std::swap(start, end);
        }
    }
};

/**
 * Represents an operation performed on the document.
 * This can be an insert, delete, or replace operation.
 */
class DocumentOperation {
public:
    DocumentOperation(
        OperationType type,
        const CursorPosition& position,
        const std::string& text = "",
        size_t length = 0,
        const std::string& userId = "",
        uint64_t timestamp = 0
    ) : type_(type),
        position_(position),
        text_(text),
        length_(length),
        userId_(userId),
        timestamp_(timestamp == 0 ? 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count() : timestamp) {}
    
    // Getters
    OperationType getType() const { return type_; }
    const CursorPosition& getPosition() const { return position_; }
    const std::string& getText() const { return text_; }
    size_t getLength() const { return length_; }
    const std::string& getUserId() const { return userId_; }
    uint64_t getTimestamp() const { return timestamp_; }
    
    // Create an inverse operation for undo
    std::unique_ptr<DocumentOperation> createInverse(const std::string& deletedText = "") const {
        switch (type_) {
            case OperationType::INSERT:
                // Inverse of INSERT is DELETE with same position and length of inserted text
                return std::make_unique<DocumentOperation>(
                    OperationType::DELETE,
                    position_,
                    "",
                    text_.length(),
                    userId_,
                    timestamp_
                );
                
            case OperationType::DELETE:
                // Inverse of DELETE is INSERT with the deleted text
                return std::make_unique<DocumentOperation>(
                    OperationType::INSERT,
                    position_,
                    deletedText,
                    0,
                    userId_,
                    timestamp_
                );
                
            case OperationType::REPLACE:
                // Inverse of REPLACE is another REPLACE that puts back the original text
                return std::make_unique<DocumentOperation>(
                    OperationType::REPLACE,
                    position_,
                    deletedText,
                    text_.length(),
                    userId_,
                    timestamp_
                );
            
            default:
                return nullptr;
        }
    }

private:
    OperationType type_;     // Type of operation
    CursorPosition position_; // Position where the operation is applied
    std::string text_;       // Text for insert or replace operations
    size_t length_;          // Length for delete operations
    std::string userId_;     // User who performed the operation
    uint64_t timestamp_;     // When the operation was performed
};

/**
 * Main document class that stores text with line-by-line access,
 * cursor positions for multiple users, and operation history.
 */
class Document {
public:
    // Callback type for document change notifications
    using ChangeCallback = std::function<void(const DocumentOperation&)>;
    
    // Constructor
    Document(const std::string& id = "", const std::string& name = "")
        : id_(id), name_(name), version_(0) {
        // Initialize with an empty line
        lines_.push_back("");
    }
    
    // Document metadata
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    uint64_t getVersion() const { return version_; }
    
    // Text access methods
    
    // Get the entire document text
    std::string getText() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string result;
        for (size_t i = 0; i < lines_.size(); ++i) {
            result += lines_[i];
            if (i < lines_.size() - 1) {
                result += '\n';
            }
        }
        return result;
    }
    
    // Set the entire document text
    void setText(const std::string& text, const std::string& userId = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Parse text into lines
        lines_.clear();
        size_t start = 0;
        size_t end = text.find('\n');
        
        while (end != std::string::npos) {
            lines_.push_back(text.substr(start, end - start));
            start = end + 1;
            end = text.find('\n', start);
        }
        
        // Add the last line
        lines_.push_back(text.substr(start));
        
        // If the document ends with a newline, add an empty last line
        if (!text.empty() && text.back() == '\n') {
            lines_.push_back("");
        }
        
        // If no lines were added, add an empty line
        if (lines_.empty()) {
            lines_.push_back("");
        }
        
        // Record this as a replace operation
        recordOperation(DocumentOperation(
            OperationType::REPLACE,
            CursorPosition(0, 0),
            text,
            0,
            userId
        ));
        
        // Increment version
        version_++;
        
        // Update timestamps
        modifiedTime_ = std::chrono::system_clock::now();
        if (createdTime_ == std::chrono::system_clock::time_point()) {
            createdTime_ = modifiedTime_;
        }
    }
    
    // Get a specific line
    std::string getLine(size_t lineIndex) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (lineIndex < lines_.size()) {
            return lines_[lineIndex];
        }
        return "";
    }
    
    // Get the number of lines
    size_t getLineCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lines_.size();
    }
    
    // Insert text at the given position
    bool insertText(const CursorPosition& position, const std::string& text, const std::string& userId = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!isValidPosition(position)) {
            return false;
        }
        
        // If text contains newlines, we need to split it and handle line insertions
        if (text.find('\n') != std::string::npos) {
            return insertMultilineText(position, text, userId);
        }
        
        // Single line insertion
        lines_[position.line].insert(position.column, text);
        
        // Record the operation
        recordOperation(DocumentOperation(
            OperationType::INSERT,
            position,
            text,
            0,
            userId
        ));
        
        // Increment version
        version_++;
        
        // Update modified time
        modifiedTime_ = std::chrono::system_clock::now();
        
        // Notify listeners
        notifyChangeListeners(DocumentOperation(
            OperationType::INSERT,
            position,
            text,
            0,
            userId
        ));
        
        return true;
    }
    
    // Delete text starting at the given position
    bool deleteText(const CursorPosition& position, size_t length, const std::string& userId = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!isValidPosition(position)) {
            return false;
        }
        
        // If length extends beyond the current line, we need to handle multi-line deletion
        const size_t lineLength = lines_[position.line].length();
        
        if (position.column + length <= lineLength) {
            // Simple single-line deletion
            std::string deletedText = lines_[position.line].substr(position.column, length);
            lines_[position.line].erase(position.column, length);
            
            // Record the operation with the deleted text for potential undo
            auto op = DocumentOperation(
                OperationType::DELETE,
                position,
                "",
                length,
                userId
            );
            
            // Store the actual deleted text for undo
            deletedTexts_[op.getTimestamp()] = deletedText;
            
            recordOperation(op);
            
            // Increment version
            version_++;
            
            // Update modified time
            modifiedTime_ = std::chrono::system_clock::now();
            
            // Notify listeners
            notifyChangeListeners(DocumentOperation(
                OperationType::DELETE,
                position,
                "",
                length,
                userId
            ));
            
            return true;
        } else {
            // Multi-line deletion
            return deleteMultilineText(position, length, userId);
        }
    }
    
    // Replace text at the given position
    bool replaceText(const CursorPosition& position, size_t length, const std::string& newText, const std::string& userId = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!isValidPosition(position)) {
            return false;
        }
        
        // Keep track of the text being replaced for undo
        std::string replacedText;
        
        // Handle multi-line replace
        if (length > lines_[position.line].length() - position.column || newText.find('\n') != std::string::npos) {
            // Store the text being replaced
            replacedText = getTextRange(position, length);
            
            // Implement as a delete followed by an insert
            bool success = deleteText(position, length, userId);
            if (success) {
                success = insertText(position, newText, userId);
            }
            return success;
        }
        
        // Single line replace
        replacedText = lines_[position.line].substr(position.column, length);
        lines_[position.line].replace(position.column, length, newText);
        
        // Record the operation
        auto op = DocumentOperation(
            OperationType::REPLACE,
            position,
            newText,
            length,
            userId
        );
        
        // Store the replaced text for undo
        deletedTexts_[op.getTimestamp()] = replacedText;
        
        recordOperation(op);
        
        // Increment version
        version_++;
        
        // Update modified time
        modifiedTime_ = std::chrono::system_clock::now();
        
        // Notify listeners
        notifyChangeListeners(DocumentOperation(
            OperationType::REPLACE,
            position,
            newText,
            length,
            userId
        ));
        
        return true;
    }
    
    // Cursor and selection management
    
    // Set cursor position for a user
    void setCursorPosition(const std::string& userId, const CursorPosition& position) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isValidPosition(position)) {
            userCursors_[userId] = position;
        }
    }
    
    // Get cursor position for a user
    CursorPosition getCursorPosition(const std::string& userId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userCursors_.find(userId);
        if (it != userCursors_.end()) {
            return it->second;
        }
        return CursorPosition(); // Default to (0, 0)
    }
    
    // Set selection for a user
    void setSelection(const std::string& userId, const Selection& selection) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isValidPosition(selection.start) && isValidPosition(selection.end)) {
            userSelections_[userId] = selection;
        }
    }
    
    // Get selection for a user
    Selection getSelection(const std::string& userId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userSelections_.find(userId);
        if (it != userSelections_.end()) {
            return it->second;
        }
        return Selection(); // Default to empty selection at (0, 0)
    }
    
    // Get all user cursors
    std::unordered_map<std::string, CursorPosition> getAllCursors() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return userCursors_;
    }
    
    // Get all user selections
    std::unordered_map<std::string, Selection> getAllSelections() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return userSelections_;
    }
    
    // Operation history management
    
    // Undo the last operation
    bool undo(const std::string& userId = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (operationHistory_.empty()) {
            return false;
        }
        
        // Get the last operation
        const DocumentOperation& lastOp = operationHistory_.back();
        
        // Create an inverse operation
        std::string deletedText;
        auto it = deletedTexts_.find(lastOp.getTimestamp());
        if (it != deletedTexts_.end()) {
            deletedText = it->second;
        }
        
        std::unique_ptr<DocumentOperation> inverseOp = lastOp.createInverse(deletedText);
        
        if (!inverseOp) {
            return false;
        }
        
        // Apply the inverse operation
        bool success = false;
        switch (inverseOp->getType()) {
            case OperationType::INSERT:
                success = insertText(inverseOp->getPosition(), inverseOp->getText(), userId);
                break;
                
            case OperationType::DELETE:
                success = deleteText(inverseOp->getPosition(), inverseOp->getLength(), userId);
                break;
                
            case OperationType::REPLACE:
                success = replaceText(inverseOp->getPosition(), inverseOp->getLength(), inverseOp->getText(), userId);
                break;
        }
        
        if (success) {
            // Remove the operation from history
            operationHistory_.pop_back();
            
            // Remove the deleted text entry if it exists
            deletedTexts_.erase(lastOp.getTimestamp());
            
            // Add to redo stack
            redoStack_.push_back(lastOp);
            
            // Increment version
            version_++;
            
            return true;
        }
        
        return false;
    }
    
    // Redo the last undone operation
    bool redo(const std::string& userId = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (redoStack_.empty()) {
            return false;
        }
        
        // Get the last undone operation
        const DocumentOperation& lastUndoneOp = redoStack_.back();
        
        // Apply the operation
        bool success = false;
        switch (lastUndoneOp.getType()) {
            case OperationType::INSERT:
                success = insertText(lastUndoneOp.getPosition(), lastUndoneOp.getText(), userId);
                break;
                
            case OperationType::DELETE:
                success = deleteText(lastUndoneOp.getPosition(), lastUndoneOp.getLength(), userId);
                break;
                
            case OperationType::REPLACE:
                success = replaceText(lastUndoneOp.getPosition(), lastUndoneOp.getLength(), lastUndoneOp.getText(), userId);
                break;
        }
        
        if (success) {
            // Remove the operation from the redo stack
            redoStack_.pop_back();
            
            // Increment version
            version_++;
            
            return true;
        }
        
        return false;
    }
    
    // Get operation history
    const std::deque<DocumentOperation>& getOperationHistory() const {
        return operationHistory_;
    }
    
    // Clear operation history
    void clearHistory() {
        std::lock_guard<std::mutex> lock(mutex_);
        operationHistory_.clear();
        redoStack_.clear();
        deletedTexts_.clear();
    }
    
    // Timestamp accessors
    std::chrono::system_clock::time_point getCreatedTime() const { return createdTime_; }
    std::chrono::system_clock::time_point getModifiedTime() const { return modifiedTime_; }
    
    // Change notification
    void addChangeListener(const ChangeCallback& callback) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        changeCallbacks_.push_back(callback);
    }
    
    void removeChangeListeners() {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        changeCallbacks_.clear();
    }
    
    // Position conversion utilities
    
    // Convert a linear position to a cursor position
    CursorPosition linearToCursor(size_t position) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t line = 0;
        size_t column = 0;
        size_t current = 0;
        
        for (const auto& lineText : lines_) {
            if (current + lineText.length() >= position) {
                column = position - current;
                return CursorPosition(line, column);
            }
            
            current += lineText.length() + 1; // +1 for the newline
            line++;
        }
        
        // If we get here, the position is beyond the end of the document
        // Return the last valid position
        if (lines_.empty()) {
            return CursorPosition(0, 0);
        } else {
            return CursorPosition(lines_.size() - 1, lines_.back().length());
        }
    }
    
    // Convert a cursor position to a linear position
    size_t cursorToLinear(const CursorPosition& cursor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (cursor.line >= lines_.size()) {
            // Beyond the end of the document
            return getTextLength();
        }
        
        size_t position = 0;
        
        // Add all complete lines before the cursor
        for (size_t i = 0; i < cursor.line; ++i) {
            position += lines_[i].length() + 1; // +1 for the newline
        }
        
        // Add the column within the line
        position += std::min(cursor.column, lines_[cursor.line].length());
        
        return position;
    }
    
    // Get the total length of the document text
    size_t getTextLength() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t length = 0;
        for (const auto& line : lines_) {
            length += line.length() + 1; // +1 for newline
        }
        
        // Don't count the final newline if it's the last line
        if (!lines_.empty()) {
            length--;
        }
        
        return length;
    }
    
    // Get a range of text from the document
    std::string getTextRange(const CursorPosition& start, size_t length) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!isValidPosition(start)) {
            return "";
        }
        
        // Get linear start position
        size_t startPos = cursorToLinear(start);
        
        // Get the full text
        std::string fullText = getText();
        
        // Extract the range
        if (startPos + length > fullText.length()) {
            return fullText.substr(startPos);
        } else {
            return fullText.substr(startPos, length);
        }
    }
    
    // Get text between two cursor positions
    std::string getTextRange(const CursorPosition& start, const CursorPosition& end) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!isValidPosition(start) || !isValidPosition(end)) {
            return "";
        }
        
        if (start > end) {
            return getTextRange(end, start);
        }
        
        // If they're on the same line
        if (start.line == end.line) {
            return lines_[start.line].substr(start.column, end.column - start.column);
        }
        
        // They're on different lines
        std::string result = lines_[start.line].substr(start.column) + "\n";
        
        // Add the complete lines in between
        for (size_t line = start.line + 1; line < end.line; ++line) {
            result += lines_[line] + "\n";
        }
        
        // Add the end line
        result += lines_[end.line].substr(0, end.column);
        
        return result;
    }

private:
    // Helper methods
    
    // Check if a position is valid within the document
    bool isValidPosition(const CursorPosition& position) const {
        if (position.line >= lines_.size()) {
            return false;
        }
        
        return position.column <= lines_[position.line].length();
    }
    
    // Record an operation in the history
    void recordOperation(const DocumentOperation& operation) {
        operationHistory_.push_back(operation);
        
        // Limit history size to prevent memory issues
        const size_t MAX_HISTORY_SIZE = 1000;
        if (operationHistory_.size() > MAX_HISTORY_SIZE) {
            operationHistory_.pop_front();
        }
        
        // Clear redo stack when a new operation is performed
        redoStack_.clear();
    }
    
    // Notify change listeners about an operation
    void notifyChangeListeners(const DocumentOperation& operation) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        for (const auto& callback : changeCallbacks_) {
            callback(operation);
        }
    }
    
    // Insert text that may contain newlines
    bool insertMultilineText(const CursorPosition& position, const std::string& text, const std::string& userId) {
        // Split the text by newlines
        std::vector<std::string> newLines;
        size_t start = 0;
        size_t end = text.find('\n');
        
        while (end != std::string::npos) {
            newLines.push_back(text.substr(start, end - start));
            start = end + 1;
            end = text.find('\n', start);
        }
        
        // Add the last segment
        newLines.push_back(text.substr(start));
        
        if (newLines.empty()) {
            return false;
        }
        
        // Get the text before and after the insertion point on the current line
        std::string beforeText = lines_[position.line].substr(0, position.column);
        std::string afterText = lines_[position.line].substr(position.column);
        
        // Update the current line with the first line of the inserted text
        lines_[position.line] = beforeText + newLines[0];
        
        // Insert the middle lines
        for (size_t i = 1; i < newLines.size() - 1; ++i) {
            lines_.insert(lines_.begin() + position.line + i, newLines[i]);
        }
        
        // Insert the last line combined with the rest of the original line
        if (newLines.size() > 1) {
            lines_.insert(lines_.begin() + position.line + newLines.size() - 1, 
                newLines.back() + afterText);
        } else {
            // If there's only one line, we need to add the after text
            lines_[position.line] += afterText;
        }
        
        // Record the operation
        recordOperation(DocumentOperation(
            OperationType::INSERT,
            position,
            text,
            0,
            userId
        ));
        
        // Increment version
        version_++;
        
        // Update modified time
        modifiedTime_ = std::chrono::system_clock::now();
        
        // Notify listeners
        notifyChangeListeners(DocumentOperation(
            OperationType::INSERT,
            position,
            text,
            0,
            userId
        ));
        
        return true;
    }
    
    // Delete text that spans multiple lines
    bool deleteMultilineText(const CursorPosition& position, size_t length, const std::string& userId) {
        // Calculate the end position
        CursorPosition endPos = linearToCursor(cursorToLinear(position) + length);
        
        // Store the text being deleted for potential undo
        std::string deletedText = getTextRange(position, length);
        
        // If the end position is invalid, adjust it
        if (endPos.line >= lines_.size()) {
            endPos.line = lines_.size() - 1;
            endPos.column = lines_[endPos.line].length();
        }
        
        // Handle the case where deletion is within the same line
        if (position.line == endPos.line) {
            lines_[position.line].erase(position.column, endPos.column - position.column);
        } else {
            // Get the text before the deletion on the first line
            std::string beforeText = lines_[position.line].substr(0, position.column);
            
            // Get the text after the deletion on the last line
            std::string afterText = lines_[endPos.line].substr(endPos.column);
            
            // Combine the remaining parts
            lines_[position.line] = beforeText + afterText;
            
            // Remove the lines in between
            lines_.erase(lines_.begin() + position.line + 1, lines_.begin() + endPos.line + 1);
        }
        
        // Record the operation
        auto op = DocumentOperation(
            OperationType::DELETE,
            position,
            "",
            length,
            userId
        );
        
        // Store the deleted text for undo
        deletedTexts_[op.getTimestamp()] = deletedText;
        
        recordOperation(op);
        
        // Increment version
        version_++;
        
        // Update modified time
        modifiedTime_ = std::chrono::system_clock::now();
        
        // Notify listeners
        notifyChangeListeners(DocumentOperation(
            OperationType::DELETE,
            position,
            "",
            length,
            userId
        ));
        
        return true;
    }

private:
    std::string id_;                                         // Document ID
    std::string name_;                                       // Document name
    std::vector<std::string> lines_;                         // Document content as lines
    uint64_t version_;                                       // Document version
    
    std::unordered_map<std::string, CursorPosition> userCursors_;  // Map of user IDs to cursor positions
    std::unordered_map<std::string, Selection> userSelections_;    // Map of user IDs to selections
    
    std::deque<DocumentOperation> operationHistory_;         // History of operations for undo
    std::deque<DocumentOperation> redoStack_;                // Stack of operations for redo
    std::unordered_map<uint64_t, std::string> deletedTexts_; // Map of operation timestamps to deleted texts (for undo)
    
    std::chrono::system_clock::time_point createdTime_;     // When the document was created
    std::chrono::system_clock::time_point modifiedTime_;    // When the document was last modified
    
    std::vector<ChangeCallback> changeCallbacks_;           // Callbacks for document changes
    
    mutable std::mutex mutex_;                              // Mutex for thread safety
    mutable std::mutex callbackMutex_;                      // Separate mutex for callbacks
};

} // namespace document
} // namespace collab

#endif // COLLABORATIVE_EDITOR_DOCUMENT_H