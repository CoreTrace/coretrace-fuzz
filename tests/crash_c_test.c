#include <string.h>
#include <stdlib.h>

// Simple C function that crashes easily
int simple_c_crash(char* input, int size) {
    char buffer[10];  // Very small buffer
    
    // Direct vulnerability: always copy full size without checking
    memcpy(buffer, input, size);  // This will crash with size > 10
    
    return buffer[0];
}

// C function with null pointer crash
int c_null_crash(char* input, int size) {
    char* ptr = NULL;
    
    if (size > 0 && input[0] == 'A') {
        *ptr = 42;  // Guaranteed crash
    }
    
    return 0;
}

// C function with array overflow
int c_array_crash(char* input, int size) {
    int arr[5] = {0};
    
    if (size > 0) {
        int index = input[0];  // Use first byte as index
        arr[index] = 42;       // Will crash if index >= 5
    }
    
    return arr[0];
}
