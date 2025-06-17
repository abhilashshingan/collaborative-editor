#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Directory setup
PROJECT_ROOT=$(pwd)
BUILD_DIR="${PROJECT_ROOT}/build"
INSTALL_DIR="${PROJECT_ROOT}/install"

# Check if cmake is installed
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}${BOLD}CMake is not installed. Please install CMake first.${RESET}"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo -e "${YELLOW}${BOLD}Configuring project...${RESET}"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" ..

# Build
echo -e "${YELLOW}${BOLD}Building project...${RESET}"
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Install
echo -e "${YELLOW}${BOLD}Installing project...${RESET}"
cmake --install .

echo -e "${GREEN}${BOLD}Build complete!${RESET}"
echo -e "You can find the executables in: ${INSTALL_DIR}/bin/"