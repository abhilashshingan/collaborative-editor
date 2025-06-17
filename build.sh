#!/usr/bin/env bash

# build.sh - Cross-platform build script for Collaborative Text Editor
# Usage: ./build.sh [--test] [--clean] [--debug]

set -e  # Exit on error

# Text colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build options
BUILD_TYPE="Release"
BUILD_TESTS=0
CLEAN_BUILD=0
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        --test)
            BUILD_TESTS=1
            ;;
        --clean)
            CLEAN_BUILD=1
            ;;
        --debug)
            BUILD_TYPE="Debug"
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --test    Build and run tests"
            echo "  --clean   Perform a clean build"
            echo "  --debug   Build in debug mode"
            echo "  --help    Display this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Detect operating system
detect_os() {
    if [ "$(uname)" == "Darwin" ]; then
        echo "macos"
    elif [ "$(uname)" == "Linux" ]; then
        echo "linux"
    else
        echo "unsupported"
    fi
}

# Detect Linux distribution
detect_linux_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    elif [ -f /etc/arch-release ]; then
        echo "arch"
    else
        echo "unknown"
    fi
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install dependencies on macOS
install_macos_deps() {
    echo -e "${BLUE}Installing dependencies on macOS...${NC}"

    if ! command_exists brew; then
        echo -e "${YELLOW}Homebrew not found. Installing...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    brew update

    PACKAGES=(
        "cmake"
        "llvm"
        "qt@6"
        "boost"
        "nlohmann-json"
        "spdlog"
        "openssl@3"
        "googletest"
    )

    # Check which packages need to be installed
    PACKAGES_TO_INSTALL=()
    for pkg in "${PACKAGES[@]}"; do
        if ! brew list "$pkg" &>/dev/null; then
            PACKAGES_TO_INSTALL+=("$pkg")
        fi
    done

    # Install required packages
    if [ ${#PACKAGES_TO_INSTALL[@]} -gt 0 ]; then
        echo -e "${YELLOW}Installing required packages: ${PACKAGES_TO_INSTALL[*]}${NC}"
        brew install "${PACKAGES_TO_INSTALL[@]}"
    else
        echo -e "${GREEN}All required packages are already installed.${NC}"
    fi

    # Make sure we use the brew-installed LLVM
    export PATH="$(brew --prefix llvm)/bin:$PATH"
    export CC="$(brew --prefix llvm)/bin/clang"
    export CXX="$(brew --prefix llvm)/bin/clang++"

    # Special handling for websocketpp (headers-only library)
    if ! brew list websocketpp &>/dev/null && ! [ -d "/usr/local/include/websocketpp" ]; then
        echo -e "${YELLOW}Installing websocketpp from source...${NC}"
        TEMP_DIR=$(mktemp -d)
        git clone https://github.com/zaphoyd/websocketpp.git "$TEMP_DIR"
        cd "$TEMP_DIR"
        mkdir -p build && cd build
        cmake ..
        make install
        cd - > /dev/null
        rm -rf "$TEMP_DIR"
    fi

    echo -e "${GREEN}macOS dependencies installed successfully.${NC}"
}

# Install dependencies on Ubuntu/Debian
install_debian_deps() {
    echo -e "${BLUE}Installing dependencies on Debian/Ubuntu...${NC}"

    sudo apt-get update

    PACKAGES=(
        "build-essential"
        "cmake"
        "g++-10"
        "ninja-build"
        "qtbase5-dev"
        "qtbase5-dev-tools"
        "libboost-all-dev"
        "libssl-dev"
        "libspdlog-dev"
        "libgtest-dev"
        "nlohmann-json3-dev"
        "libwebsocketpp-dev"
    )

    echo -e "${YELLOW}Installing required packages...${NC}"
    sudo apt-get install -y "${PACKAGES[@]}"

    # Set GCC 10 as default if it's not the default
    if command_exists g++-10 && ! g++ --version | grep -q "10\."; then
        sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100
        sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100
    fi

    echo -e "${GREEN}Debian/Ubuntu dependencies installed successfully.${NC}"
}

# Install dependencies on RHEL/Fedora
install_rhel_deps() {
    echo -e "${BLUE}Installing dependencies on RHEL/Fedora...${NC}"

    # Enable EPEL for some dependencies
    if [ -f /etc/redhat-release ]; then
        sudo dnf install -y epel-release
    fi

    PACKAGES=(
        "gcc-c++"
        "cmake"
        "ninja-build"
        "qt5-qtbase-devel"
        "boost-devel"
        "openssl-devel"
        "spdlog-devel"
        "gtest-devel"
        "json-devel"
    )

    echo -e "${YELLOW}Installing required packages...${NC}"
    sudo dnf install -y "${PACKAGES[@]}"

    # websocketpp might not be available in the repositories
    if ! dnf list installed | grep -q websocketpp; then
        echo -e "${YELLOW}Installing websocketpp from source...${NC}"
        TEMP_DIR=$(mktemp -d)
        git clone https://github.com/zaphoyd/websocketpp.git "$TEMP_DIR"
        cd "$TEMP_DIR"
        mkdir -p build && cd build
        cmake ..
        sudo make install
        cd - > /dev/null
        rm -rf "$TEMP_DIR"
    fi

    echo -e "${GREEN}RHEL/Fedora dependencies installed successfully.${NC}"
}

# Install dependencies on Arch Linux
install_arch_deps() {
    echo -e "${BLUE}Installing dependencies on Arch Linux...${NC}"

    PACKAGES=(
        "gcc"
        "cmake"
        "ninja"
        "qt6-base"
        "boost"
        "openssl"
        "spdlog"
        "gtest"
        "nlohmann-json"
    )

    echo -e "${YELLOW}Installing required packages...${NC}"
    sudo pacman -Syu --needed --noconfirm "${PACKAGES[@]}"

    # Install websocketpp from AUR if not installed
    if ! pacman -Qi websocketpp &>/dev/null; then
        echo -e "${YELLOW}Installing websocketpp from AUR...${NC}"
        if ! command_exists yay; then
            echo -e "${YELLOW}Installing yay AUR helper...${NC}"
            TEMP_DIR=$(mktemp -d)
            git clone https://aur.archlinux.org/yay.git "$TEMP_DIR"
            cd "$TEMP_DIR"
            makepkg -si --noconfirm
            cd - > /dev/null
            rm -rf "$TEMP_DIR"
        fi
        yay -S --noconfirm websocketpp
    fi

    echo -e "${GREEN}Arch Linux dependencies installed successfully.${NC}"
}

# Install dependencies based on the detected OS
install_dependencies() {
    OS=$(detect_os)

    case $OS in
        macos)
            install_macos_deps
            ;;
        linux)
            DISTRO=$(detect_linux_distro)
            case $DISTRO in
                debian|ubuntu)
                    install_debian_deps
                    ;;
                rhel|fedora|centos)
                    install_rhel_deps
                    ;;
                arch|manjaro)
                    install_arch_deps
                    ;;
                *)
                    echo -e "${RED}Unsupported Linux distribution: $DISTRO${NC}"
                    echo -e "${YELLOW}Please install dependencies manually:${NC}"
                    echo "- C++20 compatible compiler (GCC 10+, Clang 10+)"
                    echo "- CMake 3.16+"
                    echo "- Qt6 development libraries"
                    echo "- Boost libraries"
                    echo "- nlohmann_json"
                    echo "- spdlog"
                    echo "- websocketpp"
                    echo "- OpenSSL"
                    echo "- Google Test"
                    exit 1
                    ;;
            esac
            ;;
        *)
            echo -e "${RED}Unsupported operating system.${NC}"
            exit 1
            ;;
    esac
}

# Configure and build the project
build_project() {
    echo -e "${BLUE}Building the project...${NC}"

    # Create or clean build directory
    if [ -d "build" ] && [ $CLEAN_BUILD -eq 1 ]; then
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf build
    fi
    
    mkdir -p build
    cd build

    # Configure the project
    echo -e "${YELLOW}Configuring project...${NC}"
    
    CMAKE_OPTIONS=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
    
    if [ $BUILD_TESTS -eq 1 ]; then
        CMAKE_OPTIONS+=("-DBUILD_TESTS=ON")
    else
        CMAKE_OPTIONS+=("-DBUILD_TESTS=OFF")
    fi

    # On macOS with Homebrew, help CMake find packages
    if [ "$(detect_os)" == "macos" ]; then
        CMAKE_OPTIONS+=("-DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)")
        export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
    fi

    cmake "${CMAKE_OPTIONS[@]}" ..

    # Build the project
    echo -e "${YELLOW}Building project with $JOBS parallel jobs...${NC}"
    cmake --build . --config $BUILD_TYPE --parallel $JOBS

    # Run tests if requested
    if [ $BUILD_TESTS -eq 1 ]; then
        echo -e "${YELLOW}Running tests...${NC}"
        ctest --output-on-failure
    fi

    echo -e "${GREEN}Build completed successfully.${NC}"
    
    # Return to the original directory
    cd ..
}

# Check for required tools
check_prerequisites() {
    echo -e "${BLUE}Checking prerequisites...${NC}"
    
    # Git is required for the script
    if ! command_exists git; then
        echo -e "${RED}Git is required but not installed.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Prerequisites check passed.${NC}"
}

# Main function
main() {
    echo -e "${BLUE}=== Collaborative Text Editor Build Script ===${NC}"
    
    check_prerequisites
    install_dependencies
    build_project
    
    echo -e "${GREEN}=== Build process completed successfully! ===${NC}"
    
    # Print usage instructions
    echo -e "${BLUE}Usage:${NC}"
    if [ $BUILD_TESTS -eq 1 ]; then
        echo "  Tests have been built and run."
    fi
    
    echo "  The built binaries can be found in the build directory:"
    echo "    - Client: $(pwd)/build/src/client/collabEditor"
    echo "    - Server: $(pwd)/build/src/server/collabServer"
    
    echo -e "${YELLOW}To run the client:${NC} ./build/src/client/collabEditor"
    echo -e "${YELLOW}To run the server:${NC} ./build/src/server/collabServer"
}

# Execute main function
main