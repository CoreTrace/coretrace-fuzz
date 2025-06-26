#!/bin/bash

echo "=== Building LLVM Fuzzing Module ==="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "=== Running example fuzzing test ==="
    
    # Test with vulnerable function
    echo "Testing vulnerable_function..."
    ./fuzzing_module -f vulnerable_function -s ../tests/vulnerable_test.cpp -o vulnerable_results.sarif -n 5000
    
    echo ""
    echo "=== Testing divide function ==="
    ./fuzzing_module -f divide_function -s ../tests/vulnerable_test.cpp -o divide_results.sarif -n 3000
    
    echo ""
    echo "Results saved to:"
    echo "  - vulnerable_results.sarif"
    echo "  - divide_results.sarif"
    
else
    echo "Build failed!"
    exit 1
fi
