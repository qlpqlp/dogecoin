#!/bin/bash
# Quick fix script for permission errors during build
# This fixes Qt build permission issues on Windows filesystem

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR" || exit 1

print_info "Fixing permission errors for Dogecoin build"
echo ""

# Check if we're on Windows filesystem
if [[ "$SCRIPT_DIR" != /mnt/* ]]; then
    print_warn "You're already on WSL native filesystem."
    print_info "Permission errors shouldn't occur here."
    print_info "If you still see errors, try:"
    echo "  sudo chmod -R u+w depends/work"
    exit 0
fi

print_info "Detected Windows filesystem. Fixing permissions..."
echo ""

cd depends || exit 1

# Check if work directory exists
if [ ! -d work ]; then
    print_error "depends/work directory not found!"
    exit 1
fi

print_info "Step 1: Removing problematic Qt files with sudo..."
echo ""

# Remove specific problematic files
QT_BUILD_DIR="work/build/x86_64-w64-mingw32/qt"
QT_STAGING_DIR="work/staging/x86_64-w64-mingw32/qt"

if [ -d "$QT_BUILD_DIR" ]; then
    print_info "Removing Qt build directory..."
    sudo rm -rf "$QT_BUILD_DIR" 2>/dev/null || {
        print_warn "Could not remove Qt build directory. Trying alternative method..."
        # Try removing specific problematic files
        sudo find "$QT_BUILD_DIR" -type f -name "*.h" -delete 2>/dev/null || true
        sudo find "$QT_BUILD_DIR" -type d -name "Qt*" -exec rm -rf {} + 2>/dev/null || true
    }
fi

if [ -d "$QT_STAGING_DIR" ]; then
    print_info "Removing Qt staging directory..."
    sudo rm -rf "$QT_STAGING_DIR" 2>/dev/null || true
fi

echo ""

print_info "Step 2: Fixing permissions on work directory..."
echo ""

# Fix permissions
sudo chmod -R u+w work 2>/dev/null || {
    print_warn "Could not fix all permissions with chmod"
    print_info "Trying to fix specific directories..."
    
    # Fix permissions on common problematic directories
    for DIR in work/build work/staging work/downloads; do
        if [ -d "$DIR" ]; then
            sudo chmod -R u+w "$DIR" 2>/dev/null || true
        fi
    done
}

echo ""

print_info "Step 3: Cleaning stamp files..."
echo ""

# Remove stamp files that might cause issues
find work/build -name "*.stamp_configured" -delete 2>/dev/null || true
find work/build -name ".stamp_extracted" -delete 2>/dev/null || true
find work/build -name ".stamp_staged" -delete 2>/dev/null || true

echo ""

print_info "Step 4: Fixing permissions on Qt source (if extracted)..."
echo ""

# Fix permissions on Qt source if it exists
if [ -d "work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c" ]; then
    sudo chmod -R u+w "work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c" 2>/dev/null || true
fi

echo ""

print_info "Permissions fixed!"
echo ""
print_info "You can now continue the build with:"
echo "  cd depends"
echo "  export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'"
echo "  HOST=x86_64-w64-mingw32 make"
echo ""
print_warn "Note: For best results, consider building on WSL native filesystem"
print_info "See BUILD_WINDOWS11.md for instructions"



