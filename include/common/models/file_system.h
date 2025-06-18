#ifndef COLLABORATIVE_EDITOR_FILE_SYSTEM_H
#define COLLABORATIVE_EDITOR_FILE_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <filesystem>

namespace collab {
namespace fs {

// Forward declarations
class FileSystemNode;
class File;
class Directory;

/**
 * Base class for file system nodes (files and directories)
 */
class FileSystemNode : public std::enable_shared_from_this<FileSystemNode> {
public:
    enum class Type {
        File,
        Directory
    };

    FileSystemNode(const std::string& name, 
                  const std::string& owner,
                  Type type)
        : name_(name)
        , owner_(owner)
        , type_(type)
        , createdTime_(std::chrono::system_clock::now())
        , modifiedTime_(createdTime_) {}
    
    virtual ~FileSystemNode() = default;

    // Common properties
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    const std::string& getOwner() const { return owner_; }
    void setOwner(const std::string& owner) { owner_ = owner; }
    
    Type getType() const { return type_; }
    
    std::chrono::system_clock::time_point getCreatedTime() const { return createdTime_; }
    
    std::chrono::system_clock::time_point getModifiedTime() const { return modifiedTime_; }
    void updateModifiedTime() { modifiedTime_ = std::chrono::system_clock::now(); }
    
    // Polymorphic methods
    virtual std::size_t getSize() const = 0;
    virtual std::string getPath() const = 0;
    
    // Type checking helpers
    bool isFile() const { return type_ == Type::File; }
    bool isDirectory() const { return type_ == Type::Directory; }
    
    // Type casting helpers
    std::shared_ptr<File> asFile();
    std::shared_ptr<Directory> asDirectory();
    
    // Parent directory
    std::weak_ptr<Directory> getParent() const { return parent_; }
    void setParent(std::weak_ptr<Directory> parent) { parent_ = parent; }

protected:
    std::string name_;
    std::string owner_;
    Type type_;
    std::chrono::system_clock::time_point createdTime_;
    std::chrono::system_clock::time_point modifiedTime_;
    std::weak_ptr<Directory> parent_;
};

/**
 * Class representing a directory in the collaborative editor
 */
class Directory : public FileSystemNode {
public:
    Directory(const std::string& name, const std::string& owner)
        : FileSystemNode(name, owner, Type::Directory) {}
    
    // Implementation of abstract methods
    std::size_t getSize() const override;
    std::string getPath() const override;

    // Node management
    bool addNode(std::shared_ptr<FileSystemNode> node);
    
    bool removeNode(const std::string& name) {
        auto it = children_.find(name);
        if (it != children_.end()) {
            children_.erase(it);
            updateModifiedTime();
            return true;
        }
        return false;
    }
    
    std::shared_ptr<FileSystemNode> getNode(const std::string& name) const {
        auto it = children_.find(name);
        if (it != children_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    bool renameNode(const std::string& oldName, const std::string& newName);
    
    // File and directory creation helpers
    std::shared_ptr<File> createFile(const std::string& name, const std::string& owner, 
                                     const std::string& content = "");
    
    std::shared_ptr<Directory> createDirectory(const std::string& name, const std::string& owner);
    
    // Directory content access
    std::vector<std::shared_ptr<FileSystemNode>> getChildren() const {
        std::vector<std::shared_ptr<FileSystemNode>> result;
        for (const auto& pair : children_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    std::vector<std::shared_ptr<File>> getFiles() const;
    std::vector<std::shared_ptr<Directory>> getSubdirectories() const;
    
    bool isEmpty() const { return children_.empty(); }
    
    std::size_t getNodeCount() const { return children_.size(); }
    
    // Path-based node access
    std::shared_ptr<FileSystemNode> getNodeByPath(const std::string& path);

private:
    std::unordered_map<std::string, std::shared_ptr<FileSystemNode>> children_;
};

/**
 * Class representing a file in the collaborative editor
 */
class File : public FileSystemNode {
public:
    File(const std::string& name, const std::string& owner)
        : FileSystemNode(name, owner, Type::File)
        , content_("")
        , version_(0) {}
    
    File(const std::string& name, const std::string& owner, const std::string& content)
        : FileSystemNode(name, owner, Type::File)
        , content_(content)
        , version_(0) {}
    
    // Content management
    const std::string& getContent() const { return content_; }
    
    void setContent(const std::string& content) {
        content_ = content;
        version_++;
        updateModifiedTime();
    }
    
    // Incremental content modification
    void appendContent(const std::string& text) {
        content_ += text;
        version_++;
        updateModifiedTime();
    }
    
    void insertContent(std::size_t position, const std::string& text) {
        if (position <= content_.length()) {
            content_.insert(position, text);
            version_++;
            updateModifiedTime();
        }
    }
    
    bool deleteContent(std::size_t position, std::size_t length) {
        if (position < content_.length()) {
            content_.erase(position, length);
            version_++;
            updateModifiedTime();
            return true;
        }
        return false;
    }
    
    // Version management
    uint64_t getVersion() const { return version_; }
    
    // File metadata
    std::optional<std::string> getMimeType() const { return mimeType_; }
    void setMimeType(const std::string& mimeType) { mimeType_ = mimeType; }
    
    // Implementation of abstract methods
    std::size_t getSize() const override { return content_.size(); }
    std::string getPath() const override;
    
private:
    std::string content_;
    uint64_t version_;
    std::optional<std::string> mimeType_;
};

// Implementation of type-cast helpers
inline std::shared_ptr<File> FileSystemNode::asFile() {
    if (isFile()) {
        return std::static_pointer_cast<File>(shared_from_this());
    }
    return nullptr;
}

inline std::shared_ptr<Directory> FileSystemNode::asDirectory() {
    if (isDirectory()) {
        return std::static_pointer_cast<Directory>(shared_from_this());
    }
    return nullptr;
}

} // namespace fs
} // namespace collab

#endif // COLLABORATIVE_EDITOR_FILE_SYSTEM_H