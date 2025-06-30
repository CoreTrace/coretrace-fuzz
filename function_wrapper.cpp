#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Forward declaration - will be linked with the target function
// Use weak symbols so they don't need to exist
extern "C" int safe_add_function(char* input, int size) __attribute__((weak));
extern "C" int vulnerable_function(char* input, int size) __attribute__((weak));
extern "C" int safe_count_bytes(char* input, int size) __attribute__((weak));
extern "C" int divide_function(char* input, int size) __attribute__((weak));
extern "C" int pointer_function(char* input, int size) __attribute__((weak));
extern "C" int loop_function(char* input, int size) __attribute__((weak));
extern "C" int buffer_copy_function(char* input, int size) __attribute__((weak));
extern "C" int math_function(char* input, int size) __attribute__((weak));

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
        return 2;
    }
    
    // Read input data from stdin or command line
    if (argc > 3) {
        // Input provided as argument
        strncpy(input_buffer, argv[3], input_size);
    } else {
        // Read from stdin
        size_t bytes_read = fread(input_buffer, 1, input_size, stdin);
        if (bytes_read < input_size) {
            memset(input_buffer + bytes_read, 0, input_size - bytes_read);
        }
    }
    input_buffer[input_size] = '\0';
    
    int result = 0;
    
    // Call the appropriate function
    if (strcmp(func_name, "safe_add_function") == 0) {
        if (safe_add_function) {
            result = safe_add_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function safe_add_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "vulnerable_function") == 0) {
        if (vulnerable_function) {
            result = vulnerable_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function vulnerable_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "safe_count_bytes") == 0) {
        if (safe_count_bytes) {
            result = safe_count_bytes(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function safe_count_bytes not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "divide_function") == 0) {
        if (divide_function) {
            result = divide_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function divide_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "pointer_function") == 0) {
        if (pointer_function) {
            result = pointer_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function pointer_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "loop_function") == 0) {
        if (loop_function) {
            result = loop_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function loop_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "buffer_copy_function") == 0) {
        if (buffer_copy_function) {
            result = buffer_copy_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function buffer_copy_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else if (strcmp(func_name, "math_function") == 0) {
        if (math_function) {
            result = math_function(input_buffer, input_size);
        } else {
            fprintf(stderr, "Function math_function not available\n");
            free(input_buffer);
            return 3;
        }
    } else {
        fprintf(stderr, "Unknown function: %s\n", func_name);
        free(input_buffer);
        return 3;
    }
    
    // Print result
    printf("%d\n", result);
    
    free(input_buffer);
    return 0;
}
