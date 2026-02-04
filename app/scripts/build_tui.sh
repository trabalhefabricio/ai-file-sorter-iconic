#!/bin/bash
# Build script for AI File Sorter TUI
# This script builds the TUI version of AI File Sorter

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$APP_DIR/.." && pwd)"
TUI_DIR="$APP_DIR/tui"
BUILD_DIR="$REPO_ROOT/build-tui"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  AI File Sorter TUI - Build Script${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Parse arguments
CLEAN=0
RUN_TESTS=0
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN=1
            shift
            ;;
        --test)
            RUN_TESTS=1
            shift
            ;;
        --jobs=*)
            PARALLEL_JOBS="${arg#*=}"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --clean     Clean build directory before building"
            echo "  --test      Run tests after building"
            echo "  --jobs=N    Number of parallel build jobs (default: auto)"
            echo "  --help      Show this help message"
            exit 0
            ;;
    esac
done

# Check for required tools
echo -e "${YELLOW}Checking dependencies...${NC}"

check_command() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}Error: $1 is required but not installed.${NC}"
        exit 1
    fi
}

check_command cmake
check_command make

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake "$TUI_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAI_FILE_SORTER_TUI_BUILD_TESTS=$( [ $RUN_TESTS -eq 1 ] && echo "ON" || echo "OFF" )

# Build
echo -e "${YELLOW}Building with $PARALLEL_JOBS parallel jobs...${NC}"
cmake --build . --parallel "$PARALLEL_JOBS"

# Check if build succeeded
if [ -f "$BUILD_DIR/aifilesorter-tui" ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Build successful!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "Binary location: ${YELLOW}$BUILD_DIR/aifilesorter-tui${NC}"
    echo ""
    
    # Run tests if requested
    if [ $RUN_TESTS -eq 1 ]; then
        echo -e "${YELLOW}Running tests...${NC}"
        ctest --output-on-failure
    fi
    
    # Show version
    echo -e "${YELLOW}Version info:${NC}"
    "$BUILD_DIR/aifilesorter-tui" --version
    
    echo ""
    echo -e "To run: ${YELLOW}$BUILD_DIR/aifilesorter-tui${NC}"
    echo ""
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
