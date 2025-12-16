#!/usr/bin/env bash
# ============================================================================
# clean.sh - Complete cleanup script for Linux/macOS
# ============================================================================

set -e

echo "========================================================="
echo " Movie Booking System - Complete Cleanup (Linux/macOS)"
echo "========================================================="
echo ""

# Function to safely remove directory
safe_remove() {
    if [ -d "$1" ]; then
        echo "Removing $1..."
        rm -rf "$1"
        echo "  ✓ Removed $1"
    else
        echo "  ⊘ $1 does not exist (skipping)"
    fi
}

# Function to safely remove file
safe_remove_file() {
    if [ -f "$1" ]; then
        echo "Removing $1..."
        rm -f "$1"
        echo "  ✓ Removed $1"
    else
        echo "  ⊘ $1 does not exist (skipping)"
    fi
}

echo "Cleaning build artifacts..."
echo "----------------------------"
safe_remove "build"
safe_remove "docs"
safe_remove "Testing"
safe_remove ".cmake"
echo ""

echo "Cleaning CMake cache files..."
echo "-----------------------------"
safe_remove_file "CMakeCache.txt"
safe_remove_file "cmake_install.cmake"
safe_remove "CMakeFiles"
echo ""

echo "Cleaning compiled binaries..."
echo "-----------------------------"
safe_remove_file "booking_lockfree"
safe_remove_file "test_lockfree"
safe_remove_file "test_overbooking"
safe_remove_file "test_two_thread_race"
safe_remove_file "test_scalability"
echo ""

echo "Cleaning library files..."
echo "------------------------"
safe_remove_file "libbooking_lockfree_lib.a"
safe_remove_file "*.so"
safe_remove_file "*.dylib"
safe_remove_file "*.dll"
echo ""

echo "Cleaning object files..."
echo "-----------------------"
find . -name "*.o" -type f -delete 2>/dev/null || true
find . -name "*.obj" -type f -delete 2>/dev/null || true
echo "  ✓ Removed all .o and .obj files"
echo ""

echo "Cleaning editor/IDE files..."
echo "---------------------------"
safe_remove ".vscode"
safe_remove ".idea"
safe_remove "*.swp"
safe_remove "*.swo"
safe_remove "*~"
safe_remove_file ".DS_Store"
echo ""

echo "Cleaning Docker artifacts (optional)..."
echo "---------------------------------------"
if command -v docker &> /dev/null; then
    # Remove Docker image if it exists
    if docker image inspect booking-lockfree &> /dev/null; then
        echo "Removing Docker image: booking-lockfree"
        docker rmi booking-lockfree 2>/dev/null || echo "  ⊘ Failed to remove Docker image (may be in use)"
    else
        echo "  ⊘ Docker image 'booking-lockfree' does not exist"
    fi
    
    # Clean dangling images
    DANGLING=$(docker images -f "dangling=true" -q | wc -l)
    if [ "$DANGLING" -gt 0 ]; then
        echo "Removing $DANGLING dangling Docker images..."
        docker image prune -f &> /dev/null || true
        echo "  ✓ Cleaned dangling images"
    fi
else
    echo "  ⊘ Docker not available (skipping Docker cleanup)"
fi
echo ""

echo "========================================================="
echo "                  CLEANUP COMPLETE"
echo "========================================================="
echo ""
echo "All build artifacts, caches, and temporary files removed."
echo "The project is now in a clean state."
echo ""
echo "To rebuild the project, run:"
echo "  ./do_all.sh"
echo ""
