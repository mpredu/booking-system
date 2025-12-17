#!/usr/bin/env bash
# ============================================================================
# check_compiler.sh - Check for C++20 compiler with barrier support
# ============================================================================

set -e

TEST_FILE=$(mktemp /tmp/test_barrier_XXXXX.cpp)
echo '#include <barrier>' > "$TEST_FILE"
echo 'int main() { return 0; }' >> "$TEST_FILE"

CXX_COMPILER=""
CXX_FLAGS=""
CXX_FOUND=false

# Search for clang++ versions (18 down to 12)
for VER in 18 17 16 15 14 13 12 ""; do
    if [ -z "$VER" ]; then
        COMPILER="clang++"
    else
        COMPILER="clang++-$VER"
    fi
    
    if command -v "$COMPILER" &> /dev/null; then
        # Try with libc++
        if $COMPILER -std=c++20 -stdlib=libc++ -c "$TEST_FILE" -o /tmp/test.o &> /dev/null; then
            CXX_COMPILER="$COMPILER"
            CXX_FLAGS="-stdlib=libc++"
            CXX_FOUND=true
            echo "✓ Found $COMPILER with C++20 support (libc++)"
            break
        fi
        # Try with default stdlib
        if $COMPILER -std=c++20 -c "$TEST_FILE" -o /tmp/test.o &> /dev/null; then
            CXX_COMPILER="$COMPILER"
            CXX_FLAGS=""
            CXX_FOUND=true
            echo "✓ Found $COMPILER with C++20 support"
            break
        fi
    fi
done

# Search for g++ versions (14 down to 10) if clang didn't work
if [ "$CXX_FOUND" = false ]; then
    for VER in 14 13 12 11 10 ""; do
        if [ -z "$VER" ]; then
            COMPILER="g++"
        else
            COMPILER="g++-$VER"
        fi
        
        if command -v "$COMPILER" &> /dev/null; then
            if $COMPILER -std=c++20 -c "$TEST_FILE" -o /tmp/test.o &> /dev/null; then
                CXX_COMPILER="$COMPILER"
                CXX_FLAGS=""
                CXX_FOUND=true
                echo "✓ Found $COMPILER with C++20 support"
                break
            fi
        fi
    done
fi

rm -f "$TEST_FILE" /tmp/test.o

if [ "$CXX_FOUND" = false ]; then
    echo "✗ No C++20 compiler with <barrier> support found"
    echo ""
    echo "Please install a compatible compiler:"
    echo ""
    echo "For Ubuntu/Debian:"
    echo "  sudo apt update"
    echo "  sudo apt install clang-15 libc++-15-dev libc++abi-15-dev"
    echo "  or"
    echo "  sudo apt install g++-11"
    echo ""
    echo "For macOS:"
    echo "  brew install llvm"
    echo "  or update Xcode Command Line Tools"
    echo ""
    exit 1
fi

# Export for use by calling script
export CXX="$CXX_COMPILER"
if [ -n "$CXX_FLAGS" ]; then
    export CXXFLAGS="$CXX_FLAGS"
fi

echo "Using compiler: $CXX $CXX_FLAGS"
