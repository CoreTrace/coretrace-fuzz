/**
 * safe_c_test.c
 * 
 * This file contains C functions that are designed to be safe and not crash
 * during fuzzing tests. It's used to verify that the fuzzing module correctly
 * handles functions that don't crash.
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * Safely copies input data to a buffer with proper bounds checking
 */
int safe_string_copy(char* input, int size) {
    char buffer[128];
    
    // Safe copy: ensure we don't exceed buffer size
    int copy_size = size < sizeof(buffer) - 1 ? size : sizeof(buffer) - 1;
    if (input && copy_size > 0) {
        memcpy(buffer, input, copy_size);
        buffer[copy_size] = '\0'; // Ensure null termination
    } else {
        buffer[0] = '\0';
    }
    
    return copy_size;
}

/**
 * Safe array access with bounds checking
 */
int safe_array_access(char* input, int size) {
    int array[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int result = 0;
    
    if (input && size > 0) {
        // Get index from first byte but ensure it's within bounds
        unsigned char index = input[0] % 10;
        result = array[index];
    }
    
    return result;
}

/**
 * Simple arithmetic operation with overflow protection
 */
int32_t safe_arithmetic(char* input, int size) {
    int32_t a = 0, b = 1;
    
    // Extract values from input if available
    if (input && size >= 8) {
        // Use memcpy to avoid alignment issues
        memcpy(&a, input, 4);
        memcpy(&b, input + 4, 4);
    }
    
    // Prevent division by zero
    if (b == 0) {
        b = 1;
    }
    
    // Prevent overflow by checking limits
    if (a > INT32_MAX / 2 || a < INT32_MIN / 2) {
        a = a % 1000;
    }
    
    return a + (a / b);
}

/**
 * Safe memory allocation and deallocation
 */
int safe_memory_op(char* input, int size) {
    // Limit allocation size for safety
    size_t alloc_size = 0;
    
    if (input && size >= 4) {
        // Extract allocation size from first 4 bytes
        memcpy(&alloc_size, input, 4);
        
        // Ensure reasonable allocation size (0-1024 bytes)
        alloc_size = alloc_size % 1024 + 1;
    } else {
        alloc_size = 64; // Default size
    }
    
    // Allocate memory
    char* buffer = (char*)malloc(alloc_size);
    int result = 0;
    
    if (buffer) {
        // Initialize buffer safely
        memset(buffer, 0, alloc_size);
        
        // Copy input data if available, with bounds checking
        if (input && size > 4) {
            int copy_size = (size - 4) < alloc_size ? (size - 4) : alloc_size;
            memcpy(buffer, input + 4, copy_size);
            result = buffer[0]; // Just return the first byte
        }
        
        // Always free allocated memory
        free(buffer);
    }
    
    return result;
}

/**
 * Safe parsing of input as a string
 */
int safe_string_parsing(char* input, int size) {
    int count = 0;
    
    if (input && size > 0) {
        // Create a null-terminated copy of the input
        char* safe_input = (char*)malloc(size + 1);
        if (safe_input) {
            memcpy(safe_input, input, size);
            safe_input[size] = '\0';
            
            // Count occurrences of a certain character
            for (int i = 0; i < size; i++) {
                if (safe_input[i] == 'A') {
                    count++;
                }
            }
            
            free(safe_input);
        }
    }
    
    return count;
}

/**
 * Process input bytes safely
 */
int process_bytes_safely(char* input, int size) {
    uint32_t checksum = 0;
    
    if (input && size > 0) {
        for (int i = 0; i < size; i++) {
            // Simple checksum calculation
            checksum = ((checksum << 5) + checksum) + input[i];
        }
    }
    
    return (int)(checksum & 0x7FFFFFFF);
}

/**
 * A more complex function that performs multiple operations safely
 */
int complex_safe_function(char* input, int size) {
    int result = 0;
    
    if (input && size > 0) {
        // First part: process string safely
        int str_result = safe_string_parsing(input, size);
        
        // Second part: safe arithmetic
        int32_t math_result = safe_arithmetic(input, size);
        
        // Third part: process bytes
        int bytes_result = process_bytes_safely(input, size);
        
        // Combine results safely
        result = (str_result * 1000) + (math_result % 1000) + (bytes_result % 100);
    }
    
    return result;
}
