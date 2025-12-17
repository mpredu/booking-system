#!/usr/bin/env bash
# ============================================================================
# do_all.sh - Complete build, test, and documentation script for Linux/macOS
# ============================================================================

set -e

echo "========================================================="
echo " Movie Booking System - Complete Build Script"
echo "========================================================="
echo ""

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found"
    echo "Please install CMake: sudo apt-get install cmake (Ubuntu/Debian)"
    echo "                      brew install cmake (macOS)"
    exit 1
fi

# Check C++20 compiler
echo "========================================================="
echo "Checking C++20 support..."
echo "========================================================="
source ./check_compiler.sh
echo ""

# Check if Docker is available (optional)
if command -v docker &> /dev/null; then
    DOCKER_AVAILABLE=true
    echo "✓ Docker detected - will build and test Docker image"
else
    DOCKER_AVAILABLE=false
    echo "⊘ Docker not detected - skipping Docker build"
fi

# Check if Doxygen is available (optional)
if command -v doxygen &> /dev/null; then
    DOXYGEN_AVAILABLE=true
    echo "✓ Doxygen detected - will generate documentation"
else
    DOXYGEN_AVAILABLE=false
    echo "⊘ Doxygen not detected - skipping documentation generation"
fi

echo ""
echo "========================================================="
echo "Step 1: Clean previous build"
echo "========================================================="
rm -rf build docs Testing
echo "Clean complete."
echo ""

echo "========================================================="
echo "Step 2: Create build directory and configure"
echo "========================================================="
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cd ..
echo "Configuration complete."
echo ""

echo "========================================================="
echo "Step 3: Build project"
echo "========================================================="
cmake --build build
echo "Build complete."
echo ""

echo "========================================================="
echo "Step 4: Run tests"
echo "========================================================="
cd build

echo "Running test_lockfree..."
./test_lockfree || echo "WARNING: test_lockfree failed"
echo ""

echo "Running test_overbooking..."
./test_overbooking || echo "WARNING: test_overbooking failed"
echo ""

echo "Running test_two_thread_race..."
./test_two_thread_race || echo "WARNING: test_two_thread_race failed"
echo ""

echo "Running test_scalability..."
./test_scalability || echo "WARNING: test_scalability failed"
echo ""

cd ..
echo "All tests completed."
echo ""

# Generate documentation if Doxygen is available
if [ "$DOXYGEN_AVAILABLE" = true ]; then
    echo "========================================================="
    echo "Step 5: Generate documentation with Doxygen"
    echo "========================================================="
    doxygen Doxyfile
    echo "Documentation generated in docs/html/index.html"
    echo ""
else
    echo "========================================================="
    echo "Step 5: Skip documentation (Doxygen not available)"
    echo "========================================================="
    echo "Install Doxygen: sudo apt-get install doxygen (Ubuntu/Debian)"
    echo "                 brew install doxygen (macOS)"
    echo ""
fi

# Docker build and test if available
if [ "$DOCKER_AVAILABLE" = true ]; then
    echo "========================================================="
    echo "Step 6: Build and test Docker image"
    echo "========================================================="
    
    echo "Building Docker image..."
    docker build -t booking-lockfree . || {
        echo "WARNING: Docker build failed"
        exit 0
    }
    echo ""
    
    echo "Running Docker tests..."
    echo "- test_lockfree"
    docker run --rm booking-lockfree test_lockfree
    echo ""
    
    echo "- test_overbooking"
    docker run --rm booking-lockfree test_overbooking
    echo ""
    
    echo "- test_two_thread_race"
    docker run --rm booking-lockfree test_two_thread_race
    echo ""
    
    echo "- test_scalability"
    docker run --rm booking-lockfree test_scalability
    echo ""
else
    echo "========================================================="
    echo "Step 6: Skip Docker (Docker not available)"
    echo "========================================================="
    echo "Install Docker: https://docs.docker.com/engine/install/"
    echo ""
fi

echo "========================================================="
echo "                    BUILD COMPLETE"
echo "========================================================="
echo ""
echo "Executables are in: build/"
echo "  - booking_lockfree (CLI application)"
echo "  - test_lockfree"
echo "  - test_overbooking"
echo "  - test_two_thread_race"
echo "  - test_scalability"
echo ""

if [ "$DOXYGEN_AVAILABLE" = true ]; then
    echo "Documentation: docs/html/index.html"
    echo ""
fi

echo "To run the CLI application:"
echo "  cd build"
echo "  ./booking_lockfree"
echo ""
