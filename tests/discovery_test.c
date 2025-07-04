// discovery_test.c - Test file for function discovery
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A simple function that counts non-zero bytes
int count_nonzero(char* input, int size) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (input[i] != 0) {
            count++;
        }
    }
    return count;
}

// A function that can crash with a segfault
int dangerous_access(char* input, int size) {
    if (size > 0 && input[0] == 'X') {
        // This will cause a segfault
        int* null_ptr = NULL;
        return *null_ptr;
    }
    return 42;
}

// A function that can cause a divide by zero
int divide_by_value(char* input, int size) {
    if (size > 0) {
        // Potential divide by zero if input[0] is 0
        return 100 / (int)input[0];
    }
    return 0;
}

// A memory handling function that copies data
int memory_copy_test(char* input, int size) {
    if (size <= 0) return 0;
    
    char* buffer = (char*)malloc(size);
    if (!buffer) return -1;
    
    // Copy input to buffer
    memcpy(buffer, input, size);
    
    // Process buffer
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += buffer[i];
    }
    
    free(buffer);
    return sum;
}

// A function with integer overflow potential
int integer_overflow(char* input, int size) {
    if (size >= 4) {
        // Extract an integer from the first 4 bytes of input
        int value = 0;
        memcpy(&value, input, 4);
        
        // Potential integer overflow
        return value * value;
    }
    return 0;
}
