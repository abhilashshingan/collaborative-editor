#ifndef COLLABORATIVE_EDITOR_CRDT_DOCUMENT_H
#define COLLABORATIVE_EDITOR_CRDT_DOCUMENT_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <random>
#include <chrono>
#include <mutex>

namespace collab {
namespace crdt {

// Forward declarations
class CrdtChar;
class CrdtDocument;

/**
 * Class representing a character in a CRDT-based document
 * Each character has a unique position identifier
 */
class CrdtChar {
public:
    using Position = std::vector<int>;
    
    CrdtChar(char value, const std::string& authorId, const Position& position)
        : value_(value)
        , authorId_(authorId)
        , position_(position)
        , timestamp_(std::chrono::system_clock::now().time_since_epoch().count()) {}
    
    // Getters
    char getValue() const { return value_; }
    const std::string& getAuthorId() const { return authorId_; }
    const Position& getPosition() const { return position_; }
    int64_t getTimestamp() const { return timestamp_; }
    
    // Position comparison
    bool operator<(const CrdtChar& other) const {
        return compareTo(other) < 0;
    }
    
    bool operator==(const CrdtChar& other) const {
        return compareTo(other) == 0;
    }
    
    int compareTo(const CrdtChar& other) const {
        // Compare positions element by element
        const Position& otherPos = other.position_;
        const size_t minSize = std::min(position_.size(), otherPos.size());
        
        for (size_t i = 0; i < minSize; ++i) {
            if (position_[i] < otherPos[i]) return -1;
            if (position_[i] > otherPos[i]) return 1;
        }
        
        // If we get here, one position is a prefix of the other
        if (position_.size() < otherPos.size()) return -1;
        if (position_.size() > otherPos.size()) return 1;
        
        // Positions are equal, compare timestamps
        if (timestamp_ < other.timestamp_) return -1;
        if (timestamp_ > other.timestamp_) return 1;
        
        // Timestamps are equal, compare author IDs
        return authorId_.compare(other.authorId_);
    }

private:
    char value_;
    std::string authorId_;
    Position position_;
    int64_t timestamp_;
};

/**
 * CRDT document class that manages a set of characters
 */
class CrdtDocument {
public:
    CrdtDocument(const std::string& authorId)
        : authorId_(authorId)
        , strategy_(Strategy::LOGOOT) {
        // Initialize random engine with seed
        std::random_device rd;
        random_engine_.seed(rd());
    }
    
    enum class Strategy {
        LOGOOT,
        WOOT,
        LSEQ
    };
    
    void setStrategy(Strategy strategy) {
        strategy_ = strategy;
    }
    
    // Insert a character at a specific index
    void localInsert(char value, size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        CrdtChar::Position position;
        
        if (chars_.empty() && index == 0) {
            // First character
            position = generatePositionBetween({}, {});
        } else if (index == 0) {
            // Insert at beginning
            position = generatePositionBetween({}, chars_[0].getPosition());
        } else if (index >= chars_.size()) {
            // Insert at end
            position = generatePositionBetween(chars_.back().getPosition(), {});
        } else {
            // Insert between two characters
            position = generatePositionBetween(chars_[index - 1].getPosition(), chars_[index].getPosition());
        }
        
        CrdtChar newChar(value, authorId_, position);
        insertChar(newChar);
    }
    
    // Remote insertion of a character
    void remoteInsert(const CrdtChar& ch) {
        std::lock_guard<std::mutex> lock(mutex_);
        insertChar(ch);
    }
    
    // Delete a character at a specific index
    void localDelete(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index < chars_.size()) {
            chars_.erase(chars_.begin() + index);
        }
    }
    
    // Remote deletion of a character with a specific position
    void remoteDelete(const CrdtChar::Position& position) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(chars_.begin(), chars_.end(), 
            [&position](const CrdtChar& ch) { return ch.getPosition() == position; });
        
        if (it != chars_.end()) {
            chars_.erase(it);
        }
    }
    
    // Get the document text
    std::string getText() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string text;
        for (const auto& ch : chars_) {
            text += ch.getValue();
        }
        return text;
    }
    
    // Get the size of the document
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return chars_.size();
    }
    
    // Get character at index
    const CrdtChar& at(size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return chars_.at(index);
    }

private:
    // Insert a character in the correct sorted position
    void insertChar(const CrdtChar& ch) {
        auto it = std::lower_bound(chars_.begin(), chars_.end(), ch);
        chars_.insert(it, ch);
    }
    
    // Generate a position between two positions
    CrdtChar::Position generatePositionBetween(
        const CrdtChar::Position& p1, 
        const CrdtChar::Position& p2) {
        
        switch (strategy_) {
            case Strategy::LOGOOT:
                return generateLogootPosition(p1, p2);
            case Strategy::WOOT:
                return generateWootPosition(p1, p2);
            case Strategy::LSEQ:
                return generateLseqPosition(p1, p2);
            default:
                return generateLogootPosition(p1, p2);
        }
    }
    
    // Logoot strategy for position generation
    CrdtChar::Position generateLogootPosition(
        const CrdtChar::Position& p1, 
        const CrdtChar::Position& p2) {
        
        // Base case: if p1 is empty, generate a position before p2
        if (p1.empty()) {
            if (p2.empty()) {
                // Both are empty, create a new position
                return {randomInt(1, 100)};
            }
            // Before the first position
            return {p2[0] / 2};
        }
        
        // Base case: if p2 is empty, generate a position after p1
        if (p2.empty()) {
            return {p1[0] + randomInt(1, 10)};
        }
        
        // Find the common prefix
        size_t commonPrefixLen = 0;
        while (commonPrefixLen < p1.size() && 
               commonPrefixLen < p2.size() && 
               p1[commonPrefixLen] == p2[commonPrefixLen]) {
            commonPrefixLen++;
        }
        
        // Case 1: One position is a prefix of the other
        if (commonPrefixLen == p1.size()) {
            CrdtChar::Position newPos = p1;
            newPos.push_back(p2[commonPrefixLen] / 2);
            return newPos;
        }
        
        if (commonPrefixLen == p2.size()) {
            CrdtChar::Position newPos = p2;
            newPos.push_back(randomInt(1, 10));
            return newPos;
        }
        
        // Case 2: Positions differ at some point
        if (p1[commonPrefixLen] + 1 < p2[commonPrefixLen]) {
            // There's space between the two positions
            CrdtChar::Position newPos(p1.begin(), p1.begin() + commonPrefixLen);
            newPos.push_back(p1[commonPrefixLen] + randomInt(1, p2[commonPrefixLen] - p1[commonPrefixLen] - 1));
            return newPos;
        } else {
            // No space, need to add a level
            CrdtChar::Position newPos = p1;
            newPos.push_back(randomInt(1, 10));
            return newPos;
        }
    }
    
    // WOOT strategy for position generation (simplified)
    CrdtChar::Position generateWootPosition(
        const CrdtChar::Position& p1, 
        const CrdtChar::Position& p2) {
        
        // For WOOT, we'll use a simpler approach
        if (p1.empty() && p2.empty()) {
            return {randomInt(1, 1000)};
        } else if (p1.empty()) {
            return {p2[0] - randomInt(1, 10)};
        } else if (p2.empty()) {
            return {p1[0] + randomInt(1, 10)};
        } else {
            // Need to ensure the new position is between p1 and p2
            int minVal = p1[0] + 1;
            int maxVal = p2[0] - 1;
            
            if (minVal <= maxVal) {
                // There's space, use it
                return {randomInt(minVal, maxVal)};
            } else {
                // No space, create a new layer
                CrdtChar::Position newPos = p1;
                newPos.push_back(randomInt(1, 1000));
                return newPos;
            }
        }
    }
    
    // LSEQ strategy for position generation (simplified)
    CrdtChar::Position generateLseqPosition(
        const CrdtChar::Position& p1, 
        const CrdtChar::Position& p2) {
        
        // For LSEQ, we'll use a strategy that allocates more space for earlier positions
        static const int BASE = 1000;
        
        if (p1.empty() && p2.empty()) {
            return {BASE / 2};
        } else if (p1.empty()) {
            return {p2[0] / 2};
        } else if (p2.empty()) {
            return {p1[0] + randomInt(1, 10)};
        } else {
            int interval = p2[0] - p1[0];
            
            if (interval > 1) {
                // There's space, use it
                return {p1[0] + interval/2};
            } else {
                // No space, add a deeper level
                CrdtChar::Position newPos = p1;
                newPos.push_back(BASE / 2);
                return newPos;
            }
        }
    }
    
    // Generate a random integer within the range [min, max]
    int randomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(random_engine_);
    }

private:
    std::string authorId_;
    std::vector<CrdtChar> chars_;
    std::mt19937 random_engine_;
    Strategy strategy_;
    mutable std::mutex mutex_;
};

} // namespace crdt
} // namespace collab

#endif // COLLABORATIVE_EDITOR_CRDT_DOCUMENT_H