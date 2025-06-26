#include <cstring>

extern "C" {

// Safe function that bounds all operations
int safe_add_function(char* input, int size) {
    // Bound the size to prevent large allocations
    if (size < 0 || size > 1000) {
        return -1;  // Invalid size
    }
    
    // Handle null pointer
    if (input == nullptr) {
        return 0;  // Safe default
    }
    
    int sum = 0;
    // Safely iterate through the input
    for (int i = 0; i < size; ++i) {
        // Simple addition with overflow protection
        if (sum > (2147483647 - static_cast<unsigned char>(input[i]))) {
            break;  // Prevent overflow
        }
        sum += static_cast<unsigned char>(input[i]);
    }
    
    return sum;
}

// Another safe function for testing
int safe_count_bytes(char* input, int size) {
    if (input == nullptr || size < 0 || size > 1000) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < size; ++i) {
        if (input[i] != 0) {
            count++;
        }
    }
    
    return count;
}

}
