# Collaborative Text Editor

A real-time collaborative text editor written in C++20 with Boost.Asio for networking.

## Overview

This repository contains the implementation of a C++ Collaborative Text Editor with real-time editing capabilities. The implementation focuses on:

1. File System Data Structures
2. Network Message Protocol
3. TCP Communication Channel using Boost.Asio

## File Structure

### Server Module (`include/server/server.h`)

Implements a robust TCP server that handles client connections:

- `Server`: Main server class with graceful shutdown on SIGINT/SIGTERM
- `ThreadPool`: A thread pool for handling client sessions concurrently
- Nested `Connection` class for managing individual client connections
- Asynchronous I/O for high performance
- Thread-safe connection management

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

5. **Robust Error Handling**
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

## Usage Example

The `examples/collaborative_example.cpp` demonstrates:

1. Setting up a server
2. Connecting clients
3. Authenticating users
4. Opening and editing documents
5. Synchronizing changes

## Building and Running

### Prerequisites

- C++20 compliant compiler
- Boost libraries (Asio, Bind, Program_Options)
- nlohmann_json
- CMake 3.16+

### Build Commands

```bash
mkdir build && cd build
cmake ..
make
