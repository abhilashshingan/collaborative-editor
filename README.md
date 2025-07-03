# Collaborative Text Editor

A real-time collaborative text editor written in C++20 with Boost.Asio for networking.

## Overview

This repository contains a C++ Collaborative Text Editor with real-time editing capabilities. Key components include:

1. File System Data Structures
2. Network Message Protocol
3. TCP Communication using Boost.Asio
4. User Session Management with RAII
5. Operational Transformation (OT) and CRDT support

## File Structure

### Common Library (`src/common/`)
- `crdt/document.cpp`: CRDT-based document management
- `ot/operation.cpp`: OT operation handling
- `ot/history.cpp`: OT history management
- `ot/editor.cpp`: OT editor logic
- `document/document_controller.cpp`: Document controller
- `document/history_manager.cpp`: History management
- `document/operation_manager.cpp`: Operation management

### Server Module (`include/server/`, `src/server/`)
- `server.h`: TCP server with graceful shutdown
- `session_handler.h`: User session and socket management
- `main.cpp`: Server entry point

### Client Module (`src/client/`)
- `main.cpp`: Qt-based GUI client
- `document_client.cpp`: Document handling logic
- `terminal_client.cpp`: Terminal client implementation
- `ncurses_client.cpp`: NCurses-based terminal UI
- `document_editor.cpp`: Document editing logic
- `session_manager.cpp`: Client session management

### Tests (`tests/`)
- `common/`: Tests for CRDT and OT components
- `client/`: Client-side tests
- `server/`: Server-side tests, including session handling
- `utils/test_helpers.cpp`: Test utilities

## Key Features

- Real-time collaborative editing with OT and CRDT
- Robust client-server communication via TCP
- Thread-safe session management
- RAII-based socket lifecycle management
- Automatic idle session cleanup
- Configurable thread pool for concurrent processing
- Graceful shutdown on SIGINT/SIGTERM
- Support for Qt GUI and NCurses terminal clients

## Prerequisites

- C++20 compiler (e.g., GCC 10+, Clang 10+)
- CMake 3.16+
- Boost (system, thread, uuid)
- nlohmann_json
- spdlog
- OpenSSL (for server)
- Qt6 (for GUI client)
- NCurses (for terminal client)
- Google Test (for tests)
- WebSocket++ (optional, for websocket support)

## Build Instructions

1. Run the build script:
   ```bash
   ./build.sh [--tests]
   ```
   Use `--tests` to run tests after building.

2. Executables are located in `install/bin/`.

## Running

- **Server**: `./install/bin/server`
- **Qt Client**: `./install/bin/client`
- **Terminal Client**: `./install/bin/terminal_client`

## Testing

Run tests with:
```bash
cd build
ctest --output-on-failure
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit changes (`git commit -am 'Add your feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request

## License

MIT License. See `LICENSE` for details.
