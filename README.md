# Collaborative Text Editor

A real-time collaborative text editor written in C++20 with Boost.Asio for networking.

## Overview

This repository contains the implementation of a C++ Collaborative Text Editor with real-time editing capabilities. The implementation focuses on:

1. File System Data Structures
2. Network Message Protocol
3. TCP Communication Channel using Boost.Asio
4. User Session Management with RAII

## File Structure

### Server Module (`include/server/server.h`)

Implements a robust TCP server that handles client connections:

- `Server`: Main server class with graceful shutdown on SIGINT/SIGTERM
- `ThreadPool`: A thread pool for handling client sessions concurrently
- Nested `Connection` class for managing individual client connections
- Asynchronous I/O for high performance
- Thread-safe connection management

### Session Handler (`include/server/session_handler.h`)

Manages user sessions and their connections:

- `SessionHandler`: Central manager for user sessions
- `UserSession`: Represents user state, authentication, and document access
- `SocketGuard`: RAII wrapper for socket lifecycle management
- Thread-safe session tracking
- Automatic idle session cleanup

### File System Data Structures (`include/common/models/file_system.h`)

Provides classes for representing files and directories:

- `FileSystemNode`: Abstract base class with common functionality
- `File`: For managing file content and version history
- `Directory`: For organizing files and managing hierarchical structure

### Protocol Message Formats (`include/common/protocol/protocol.h`)

Defines structured message types for network communication:

- Authentication messages
- Document management messages
- Edit operation messages
- Synchronization messages
- Presence tracking messages

All messages are serializable to JSON for network transmission.

### TCP Communication (`include/common/network/tcp_connection.h`)

Implements network communication using Boost.Asio:

- `TcpConnection`: Low-level TCP socket management
- `TcpClient`: Client-side connection handling
- `TcpServer`: Server-side connection acceptance and management
- `MessageChannel`: Higher-level protocol message handling

## Key Features

- Real-time collaborative editing
- File and directory management
- Robust client-server communication
- Message serialization/deserialization
- Error handling and recovery
- Thread-safe operations
- Graceful shutdown on SIGINT/SIGTERM
- Configurable thread pool for concurrent client session handling
- RAII-based socket lifecycle management
- User session tracking and authentication
- Document access control
- Automatic idle session cleanup

## Server Implementation

The server implementation in `include/server/server.h` and `src/server/server.cpp` provides:

1. **TCP Connection Management**
   - Accepting new client connections
   - Managing active connections
   - Handling connection errors

2. **Graceful Shutdown**
   - Signal handling for SIGINT (Ctrl+C) and SIGTERM
   - Proper cleanup of all connections
   - Orderly shutdown of server resources

3. **Asynchronous I/O**
   - Non-blocking operations using Boost.Asio
   - Efficient handling of multiple clients

4. **Thread Pool**
   - Configurable number of worker threads
   - Concurrent processing of client requests
   - Efficient load balancing across threads
   - Thread-safe task queue

5. **Session Management**
   - User identification and authentication
   - Session state tracking
   - Document access tracking
   - RAII-based resource management

6. **Robust Error Handling**
   - Comprehensive error detection
   - Recovery mechanisms
   - Detailed error reporting

## Thread Pool Implementation

The `ThreadPool` class provides:

1. **Configurable Size**
   - Set the number of worker threads at creation time
   - Defaults to the number of hardware threads available

2. **Task Queuing**
   - Tasks are queued and distributed to available worker threads
   - Thread-safe queue with mutex protection

3. **Future-based API**
   - Returns `std::future` objects for task results
   - Allows waiting for task completion and retrieving results

4. **Clean Shutdown**
   - Properly joins all worker threads on destruction
   - Handles remaining tasks before shutting down

## Session Handler Implementation

The `SessionHandler` class provides:

1. **User Session Management**
   - Creating and tracking user sessions
   - Authentication and username validation
   - Document access tracking
   - Session state management
   - Idle session cleanup

2. **RAII Socket Management**
   - Automatic socket lifecycle management via `SocketGuard`
   - Ensures sockets are properly closed when sessions end
   - Prevents resource leaks and zombie connections

3. **Thread Safety**
   - Mutex-protected data structures
   - Safe for concurrent access from multiple threads
   - Proper synchronization of session state changes

4. **Document Collaboration**
   - Tracking which users are active on which documents
   - Supporting document-specific operations
   - Managing document sharing and access

## Usage Example

The `examples/collaborative_example.cpp` demonstrates:

1. Setting up a server
2. Connecting clients
3. Authenticating users
4. Opening and editing documents
5. Synchronizing changes

## Building and Running

### Prerequisites

- C++20 compliant compiler (e.g., GCC 10+, Clang 10+)
- Boost libraries (Asio, Bind, Program_Options, UUID)
- nlohmann_json
- CMake 3.16+
- Optional: Qt6 for client GUI, Google Test for unit tests

### Build Commands

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Installation

```bash
cmake --install .
```

### Running the Server

```bash
./install/bin/server
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -am 'Add your feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
