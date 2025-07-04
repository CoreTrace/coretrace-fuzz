#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

// Dynamic wrapper that can call any function at runtime
// This wrapper will be compiled separately and linked with user code

// Function pointer type for user functions
typedef int (*user_function_t)(char* input, int size);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <function_name> <input_size> [input_data]\n", argv[0]);
        return 1;
    }
    
    const char* func_name = argv[1];
    int input_size = atoi(argv[2]);
    
    // Allocate buffer for input
    char* input_buffer = (char*)malloc(input_size + 1);
    if (!input_buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Initialize buffer
    memset(input_buffer, 0, input_size + 1);
    
    // Read input from stdin if provided
    if (argc > 3) {
        // Input provided as argument (hex encoded)
        const char* hex_input = argv[3];
        int hex_len = strlen(hex_input);
        int actual_size = (hex_len / 2 < input_size) ? hex_len / 2 : input_size;
        
        for (int i = 0; i < actual_size; i++) {
            char hex_byte[3] = {hex_input[i*2], hex_input[i*2+1], '\0'};
            input_buffer[i] = (char)strtol(hex_byte, NULL, 16);
        }
    } else {
        // Read binary input from stdin
        size_t bytes_read = fread(input_buffer, 1, input_size, stdin);
        if (bytes_read == 0) {
            // No input, generate some test data
            for (int i = 0; i < input_size; i++) {
                input_buffer[i] = (char)(i % 256);
            }
        }
    }
    
    // Null terminate for safety
    input_buffer[input_size] = '\0';
    
    // Try to find the function dynamically
    // First, try to get function from current executable
    void* handle = dlopen(NULL, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to get handle to current executable\n");
        free(input_buffer);
        return 2;
    }
    
    // Clear any existing error
    dlerror();
    
    // Try to find the function
    user_function_t func = (user_function_t)dlsym(handle, func_name);
    
    char* error = dlerror();
    if (error != NULL || func == NULL) {
        fprintf(stderr, "Function '%s' not found: %s\n", func_name, error ? error : "Unknown error");
        dlclose(handle);
        free(input_buffer);
        return 3;
    }
    
    // Call the function
    int result = func(input_buffer, input_size);
    
    // Print result
    printf("%d\n", result);
    
    // Cleanup
    dlclose(handle);
    free(input_buffer);
    return 0;
}
