#!/bin/bash
set -e

# Create lib directory if it doesn't exist
mkdir -p lib

# Check if libcompilerlib.so is in standard locations or current directory
if [ -f "/usr/lib/libcompilerlib.so" ]; then
    echo "Found libcompilerlib.so in /usr/lib"
    # Create symlink to the lib directory for easier access
    ln -sf /usr/lib/libcompilerlib.so lib/libcompilerlib.so
elif [ -f "/usr/local/lib/libcompilerlib.so" ]; then
    echo "Found libcompilerlib.so in /usr/local/lib"
    ln -sf /usr/local/lib/libcompilerlib.so lib/libcompilerlib.so
elif [ -f "libcompilerlib.so" ]; then
    echo "Found libcompilerlib.so in current directory"
    mv libcompilerlib.so lib/
else
    echo "Warning: libcompilerlib.so not found in standard locations"
    echo "Will attempt dynamic loading at runtime"
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure and build project
cmake ..
make -j$(nproc)

echo "Build completed"
