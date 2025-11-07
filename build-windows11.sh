#!/bin/bash
# Build script for Dogecoin Core on Windows 11 using WSL2
# This script builds Dogecoin Core that will run natively on Windows 11

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running in WSL
if [ ! -d /proc/sys/fs/binfmt_misc ]; then
    print_error "This script must be run in WSL (Windows Subsystem for Linux)"
    exit 1
fi

# Detect WSL version
WSL_VERSION=$(cat /proc/version | grep -i microsoft)
if [ -z "$WSL_VERSION" ]; then
    print_warn "Could not detect WSL version. Continuing anyway..."
fi

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR" || exit 1

print_info "Building Dogecoin Core for Windows 11"
print_info "Build directory: $SCRIPT_DIR"
echo ""

# Check if we're on Windows filesystem or WSL native filesystem
if [[ "$SCRIPT_DIR" == /mnt/* ]]; then
    print_warn "Building on Windows filesystem. This may be slower."
    print_warn "Consider copying to WSL native filesystem (~/dogecoin) for better performance."
    echo ""
    
    # Clean PATH of Windows mount paths
    export PATH=$(echo "$PATH" | tr ':' '\n' | grep -v '^/mnt' | grep -v '(' | grep -v ')' | grep -v '|' | tr '\n' ':' | sed 's/:$//' | sed 's/^://')
    if [ -z "$PATH" ] || [ "$PATH" = "" ]; then
        export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
    fi
    export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH"
    export PATH=$(echo "$PATH" | tr ':' '\n' | awk '!seen[$0]++' | tr '\n' ':' | sed 's/:$//')
else
    print_info "Building on WSL native filesystem. Good choice!"
fi

# Verify we have the required files
if [ ! -f depends/Makefile ]; then
    print_error "depends/Makefile not found!"
    exit 1
fi

# Check for required tools
print_info "Checking for required tools..."
MISSING_TOOLS=()

if ! command -v make &> /dev/null; then
    MISSING_TOOLS+=("make")
fi

if ! command -v g++ &> /dev/null; then
    MISSING_TOOLS+=("g++")
fi

if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    MISSING_TOOLS+=("g++-mingw-w64-x86-64")
fi

if [ ${#MISSING_TOOLS[@]} -ne 0 ]; then
    print_error "Missing required tools: ${MISSING_TOOLS[*]}"
    print_info "Install them with:"
    echo "  sudo apt-get update"
    echo "  sudo apt-get install -y build-essential g++-mingw-w64-x86-64"
    exit 1
fi

print_info "All required tools are installed"
echo ""

# Step 1: Fix line endings in depends directory
print_info "Step 1: Fixing line endings in depends directory..."
cd depends || exit 1

if command -v dos2unix &> /dev/null; then
    dos2unix config.guess config.sub 2>/dev/null || true
else
    sed -i 's/\r$//' config.guess config.sub 2>/dev/null || true
fi

chmod +x config.guess config.sub 2>/dev/null || true

# Test that config.guess works
if ! ./config.guess >/dev/null 2>&1; then
    print_error "config.guess is not executable properly"
    exit 1
fi

cd .. || exit 1
print_info "Line endings fixed"
echo ""

# Step 2: Build dependencies
print_info "Step 2: Building dependencies (this may take 30-60 minutes)..."
print_warn "This is the longest step. Please be patient."
cd depends || exit 1

# Fix permissions on work directory (WSL permission issues with Windows filesystem)
if [ -d work ]; then
    print_info "Fixing permissions for work directory..."
    chmod -R u+w work 2>/dev/null || true
fi

# Build dependencies
print_info "Building dependencies for x86_64-w64-mingw32..."
if ! HOST=x86_64-w64-mingw32 make; then
    print_error "Dependency build failed!"
    print_info "Trying to clean and retry..."
    rm -rf work/build/x86_64-w64-mingw32/boost work/staging/x86_64-w64-mingw32/boost 2>/dev/null || true
    chmod -R u+w work 2>/dev/null || true
    find work/build -name "*.stamp_configured" -delete 2>/dev/null || true
    
    print_info "Retrying build..."
    if ! HOST=x86_64-w64-mingw32 make; then
        print_error "Dependency build failed after retry"
        exit 1
    fi
fi

cd .. || exit 1
print_info "Dependencies built successfully"
echo ""

# Step 3: Run autogen.sh
print_info "Step 3: Running autogen.sh..."
if [ ! -f configure ]; then
    ./autogen.sh || {
        print_error "autogen.sh failed!"
        exit 1
    }
else
    print_info "configure script already exists, skipping autogen.sh"
fi
echo ""

# Step 4: Configure
print_info "Step 4: Configuring build..."
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/ || {
    print_error "Configure failed!"
    exit 1
}
print_info "Configuration complete"
echo ""

# Step 5: Build Dogecoin Core
print_info "Step 5: Building Dogecoin Core (this may take 30-60 minutes)..."
print_warn "This is another long step. Please be patient."
if ! make; then
    print_error "Build failed!"
    exit 1
fi

print_info "Build complete!"
echo ""

# Step 6: Verify executables
print_info "Step 6: Verifying executables..."
EXECUTABLES=(
    "src/qt/dogecoin-qt.exe"
    "src/dogecoind.exe"
    "src/dogecoin-cli.exe"
    "src/dogecoin-tx.exe"
)

ALL_EXIST=true
for EXE in "${EXECUTABLES[@]}"; do
    if [ -f "$EXE" ]; then
        SIZE=$(du -h "$EXE" | cut -f1)
        print_info "✓ $EXE ($SIZE)"
    else
        print_warn "✗ $EXE (not found)"
        ALL_EXIST=false
    fi
done

echo ""

if [ "$ALL_EXIST" = true ]; then
    print_info "=========================================="
    print_info "Build Successful!"
    print_info "=========================================="
    echo ""
    print_info "Executables are ready:"
    echo "  • GUI Application: src/qt/dogecoin-qt.exe"
    echo "  • Daemon: src/dogecoind.exe"
    echo "  • CLI Tool: src/dogecoin-cli.exe"
    echo "  • Transaction Tool: src/dogecoin-tx.exe"
    echo ""
    print_info "To run on Windows 11:"
    echo "  1. Navigate to src/qt/ in File Explorer"
    echo "  2. Double-click dogecoin-qt.exe"
    echo ""
    
    # If on Windows filesystem, suggest copying to accessible location
    if [[ "$SCRIPT_DIR" == /mnt/* ]]; then
        print_info "The executables are already accessible from Windows"
    else
        print_info "To copy executables to Windows:"
        echo "  cp src/qt/dogecoin-qt.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/qt/"
        echo "  cp src/dogecoind.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/"
        echo "  cp src/dogecoin-cli.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/"
        echo "  cp src/dogecoin-tx.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/"
    fi
else
    print_warn "Some executables were not found. Build may have issues."
fi



