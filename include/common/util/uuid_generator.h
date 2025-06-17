#ifndef COLLABORATIVE_EDITOR_UUID_GENERATOR_H
#define COLLABORATIVE_EDITOR_UUID_GENERATOR_H

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <mutex>

namespace collab {
namespace util {

/**
 * A simple UUID generator class
 */
class UuidGenerator {
public:
    // Get a singleton instance
    static UuidGenerator& getInstance() {
        static UuidGenerator instance;
        return instance;
    }
    
    // Generate a new UUID
    std::string generateUuid() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::stringstream ss;
        
        // Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
        // where y is one of: 8, 9, a, b
        
        // First 8 hex digits
        ss << std::hex << std::setfill('0') << std::setw(8) << random();
        ss << "-";
        
        // Next 4 hex digits
        ss << std::hex << std::setfill('0') << std::setw(4) << (random() & 0xFFFF);
        ss << "-";
        
        // Version 4 (random) UUID
        ss << "4" << std::hex << std::setfill('0') << std::setw(3) << (random() & 0x0FFF);
        ss << "-";
        
        // Variant bits (y)
        uint32_t variantBits = random();
        ss << std::hex << std::setfill('0') << std::setw(1) << ((variantBits & 0x3) + 8);
        ss << std::hex << std::setfill('0') << std::setw(3) << ((variantBits >> 2) & 0x0FFF);
        ss << "-";
        
        // Last 12 hex digits
        ss << std::hex << std::setfill('0') << std::setw(12) << random();
        
        return ss.str();
    }
    
private:
    // Private constructor for singleton
    UuidGenerator() {
        std::random_device rd;
        rng_.seed(rd());
    }
    
    // Disallow copying
    UuidGenerator(const UuidGenerator&) = delete;
    UuidGenerator& operator=(const UuidGenerator&) = delete;
    
    // Generate a random 32-bit integer
    uint32_t random() {
        return dist_(rng_);
    }
    
private:
    std::mt19937 rng_;
    std::uniform_int_distribution<uint32_t> dist_{0, UINT32_MAX};
    std::mutex mutex_;
};

} // namespace util
} // namespace collab

#endif // COLLABORATIVE_EDITOR_UUID_GENERATOR_H