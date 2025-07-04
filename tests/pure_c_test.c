#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Pure C function - buffer overflow vulnerability
int c_buffer_overflow(char* input, int size) {
    char buffer[32];  // Small buffer
    
    // Vulnerability: no bounds checking
    if (size > 0) {
        memcpy(buffer, input, size);  // Potential overflow
    }
    
    int sum = 0;
    for (int i = 0; i < size && i < 32; i++) {
        sum += (unsigned char)buffer[i];
    }
    return sum;
}

// Pure C function - array bounds violation
int c_array_access(char* input, int size) {
    int array[10] = {0};
    
    if (size > 0) {
        int index = (unsigned char)input[0];  // Use first byte as index
        
        // Vulnerability: no bounds checking
        if (index >= 0) {
            array[index] = 42;  // Potential out-of-bounds
        }
    }
    
    return array[0];
}

// Pure C function - division by zero
int c_divide(char* input, int size) {
    if (size < 4) return 0;
    
    // Extract int from input (first 4 bytes)
    int divisor = 0;
    memcpy(&divisor, input, sizeof(int));
    
    // Vulnerability: division by zero
    return 1000 / divisor;
}

// Pure C function - safe (should not crash)
int c_safe_function(char* input, int size) {
    if (!input || size <= 0) return 0;
    
    int sum = 0;
    // Safe bounds checking
    for (int i = 0; i < size && i < 100; i++) {
        sum += (unsigned char)input[i];
    }
    
    return sum % 1000;  // Prevent overflow
}

// Pure C function - memory access pattern
int c_memory_pattern(char* input, int size) {
    static char static_buffer[64];
    
    if (size > 0 && size < 64) {
        // Safe copy
        memcpy(static_buffer, input, size);
        static_buffer[size] = '\0';
        
        return strlen(static_buffer);
    }
    
    return 0;
}
