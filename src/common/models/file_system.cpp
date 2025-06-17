#include "common/models/file_system.h"

namespace collab {
namespace fs {

// File class method implementations
std::string File::getPath() const {
    auto parent = getParent().lock();
    if (parent) {
        std::string parentPath = parent->getPath();
        if (parentPath == "/") {
            return parentPath + getName();
        } else {
            return parentPath + "/" + getName();
        }
    }
    return getName();
}

// Directory class method implementations
std::size_t Directory::getSize() const {
    std::size_t totalSize = 0;
    for (const auto& pair : children_) {
        totalSize += pair.second->getSize();
    }
    return totalSize;
}

std::string Directory::getPath() const {
    auto parent = getParent().lock();
    if (parent) {
        std::string parentPath = parent->getPath();
        if (parentPath == "/") {
            return parentPath + getName();
        } else {
            return parentPath + "/" + getName();
        }
    }
    return "/" + getName();
}

bool Directory::addNode(std::shared_ptr<FileSystemNode> node) {
    if (children_.find(node->getName()) != children_.end()) {
        return false; // Name conflict
    }
    
    node->setParent(std::static_pointer_cast<Directory>(shared_from_this()));
    children_[node->getName()] = node;
    updateModifiedTime();
    return true;
}

bool Directory::renameNode(const std::string& oldName, const std::string& newName) {
    if (children_.find(newName) != children_.end()) {
        return false; // New name already exists
    }
    
    auto it = children_.find(oldName);
    if (it != children_.end()) {
        auto node = it->second;
        children_.erase(it);
        node->setName(newName);
        children_[newName] = node;
        updateModifiedTime();
        return true;
    }
    return false;
}

std::shared_ptr<File> Directory::createFile(const std::string& name, const std::string& owner, 
                                  const std::string& content) {
    auto file = std::make_shared<File>(name, owner, content);
    if (addNode(file)) {
        return file;
    }
    return nullptr;
}

std::shared_ptr<Directory> Directory::createDirectory(const std::string& name, const std::string& owner) {
    auto directory = std::make_shared<Directory>(name, owner);
    if (addNode(directory)) {
        return directory;
    }
    return nullptr;
}

std::vector<std::shared_ptr<File>> Directory::getFiles() const {
    std::vector<std::shared_ptr<File>> files;
    for (const auto& pair : children_) {
        if (pair.second->isFile()) {
            files.push_back(std::static_pointer_cast<File>(pair.second));
        }
    }
    return files;
}

std::vector<std::shared_ptr<Directory>> Directory::getSubdirectories() const {
    std::vector<std::shared_ptr<Directory>> directories;
    for (const auto& pair : children_) {
        if (pair.second->isDirectory()) {
            directories.push_back(std::static_pointer_cast<Directory>(pair.second));
        }
    }
    return directories;
}

std::shared_ptr<FileSystemNode> Directory::getNodeByPath(const std::string& path) {
    if (path.empty()) return nullptr;
    
    // Handle absolute paths
    if (path[0] == '/') {
        // Find the root directory
        std::shared_ptr<Directory> root = std::static_pointer_cast<Directory>(shared_from_this());
        while (auto parent = root->getParent().lock()) {
            root = parent;
        }
        
        // If this is just the root directory
        if (path == "/") return root;
        
        // Otherwise, resolve the path relative to the root
        return root->getNodeByPath(path.substr(1));
    }
    
    // Handle relative paths
    size_t pos = path.find('/');
    if (pos == std::string::npos) {
        // This is the final component of the path
        return getNode(path);
    } else {
        // We need to traverse deeper
        std::string dirName = path.substr(0, pos);
        std::string remainingPath = path.substr(pos + 1);
        
        auto node = getNode(dirName);
        if (node && node->isDirectory()) {
            return std::static_pointer_cast<Directory>(node)->getNodeByPath(remainingPath);
        }
        return nullptr;
    }
}

} // namespace fs
} // namespace collab