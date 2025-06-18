# Collaborative Text Editor

A real-time collaborative text editor written in C++20 with networked multi-user support.

## Features

* **Real-time Collaboration**: Multiple users can edit the same document simultaneously
* **CRDT-Based Conflict Resolution**: Uses Conflict-Free Replicated Data Types for reliable merging of concurrent edits
* **Cross-Platform**: Works on Linux, macOS, and Windows
* **Offline Capability**: Edit documents offline and synchronize when reconnected
* **Syntax Highlighting**: Supports code editing with syntax highlighting
* **User Presence**: See who's editing and where their cursor is located
* **Permissions System**: Control who can view and edit documents
* **Chat & Comments**: Communicate with other users within the editor

## Architecture

The application follows a client-server architecture:

1. **Client**:
   - GUI Layer (Qt-based interface)
   - Editor Engine (text editing core)
   - Collaboration Layer (CRDT implementation, synchronization)
   - Network Layer (communication with server)

2. **Server**:
   - Session Manager (user sessions, authentication)
   - Document Store (storage and retrieval)
   - Synchronization Engine (coordinates changes)
   - Authentication Service (user management)

## Technology Stack

* **C++20**: Modern C++ features for robust implementation
* **Qt 6**: Cross-platform GUI framework
* **Boost.Asio**: Networking and async operations
* **nlohmann::json**: JSON handling for protocol
* **WebSocket++**: WebSocket communication
* **OpenSSL**: Secure communication
* **spdlog**: Logging facility
* **Google Test**: Testing framework

## Building

### Prerequisites

* C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 19.28+)
* CMake 3.16+
* Qt 6
* Boost (system, thread components)
* nlohmann_json
* spdlog
* WebSocket++
* OpenSSL

### Build Instructions

Use the provided build script to compile the project:

```bash
./build.sh