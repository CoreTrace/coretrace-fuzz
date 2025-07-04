#!/bin/bash

# Setup script for LLVM 19 environment
# This ensures the fuzzing module uses LLVM/Clang 19.1 for compatibility

echo "Setting up LLVM 19 environment..."

# Add LLVM 19 to PATH (highest priority)
export PATH="/usr/lib/llvm19/bin:$PATH"

# Add LLVM 19 libraries to LD_LIBRARY_PATH
export LD_LIBRARY_PATH="/usr/lib/llvm19/lib:$LD_LIBRARY_PATH"

# Set compiler environment variables
export CC=clang
export CXX=clang++
export LLVM_CONFIG=llvm-config

# For CMake
export LLVM_ROOT=/usr/lib/llvm19
export CMAKE_PREFIX_PATH="/usr/lib/llvm19:$CMAKE_PREFIX_PATH"

echo "LLVM 19 environment configured successfully!"
echo "Current clang version:"
clang --version | head -n1

echo ""
echo "To use this environment:"
echo "  source scripts/setup_llvm19_env.sh"
echo ""
echo "To make this permanent, add this line to your ~/.bashrc or ~/.zshrc:"
echo "  source /home/yoolooops/eip/coretrace-fuzz/scripts/setup_llvm19_env.sh"
