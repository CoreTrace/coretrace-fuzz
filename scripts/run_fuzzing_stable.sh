#!/bin/bash

# Wrapper script to run the fuzzing module with LLVM 19 environment
# This script ensures compatibility and falls back to clang when the dynamic library fails

# Set up LLVM 19 environment
echo "Setting up LLVM 19 environment..."
export PATH="/usr/lib/llvm19/bin:$PATH"
export LD_LIBRARY_PATH="/usr/lib/llvm19/lib:$LD_LIBRARY_PATH"
export CC=clang
export CXX=clang++

echo "LLVM/Clang version:"
clang --version | head -n1
echo ""

# Temporarily move the problematic dynamic library so it falls back to clang
LIBRARY_PATH="/home/yoolooops/eip/coretrace-fuzz/include/compilerlib/libcompilerlib.so"
BACKUP_PATH="${LIBRARY_PATH}.disabled"

if [ -f "$LIBRARY_PATH" ]; then
    echo "Temporarily disabling dynamic library to use stable clang fallback..."
    mv "$LIBRARY_PATH" "$BACKUP_PATH"
    LIBRARY_MOVED=true
else
    LIBRARY_MOVED=false
fi

echo "Running fuzzing module with arguments: $@"
echo "=================================="

# Run the fuzzing module with provided arguments
/home/yoolooops/eip/coretrace-fuzz/build/fuzzing_module "$@"
RESULT=$?

# Restore the library
if [ "$LIBRARY_MOVED" = true ]; then
    echo ""
    echo "Restoring dynamic library..."
    mv "$BACKUP_PATH" "$LIBRARY_PATH"
fi

echo ""
echo "Fuzzing completed with exit code: $RESULT"
exit $RESULT
