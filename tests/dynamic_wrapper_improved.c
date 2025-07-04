#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <signal.h>

// Improved dynamic wrapper that can call any function at runtime
// This wrapper uses dlopen/dlsym to find and call functions dynamically

// Function pointer types for different signatures
typedef int (*user_func_int_char_int)(char* input, int size);
typedef void (*user_func_void_char_int)(char* input, int size);
typedef int (*user_func_int_void)(void);
typedef void (*user_func_void_void)(void);

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
    
    // Try to find the function dynamically
    // First, try to get function from current executable
    void* handle = dlopen(NULL, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to get handle to current executable: %s\n", dlerror());
        free(input_buffer);
        return 2;
    }
    
    // Clear any existing error
    dlerror();
    
    // Try to find the function
    void* func_ptr = dlsym(handle, func_name);
    char* error = dlerror();
    
    // If function not found, try different name mangling patterns for C++ support
    if (error != NULL || func_ptr == NULL) {
        // Try C++ mangling pattern for simple functions
        // This is a very simplified approach and won't work for all C++ functions
        char mangled_name[256];
        
        // Try different name patterns
        const char* patterns[] = {
            "_%s",           // Some systems
            "_Z%zu%s",       // Length-prefixed (simplified)
            "_Z%d%s",        // Another variant
            "_%s_",          // Another pattern
            NULL
        };
        
        for (int i = 0; patterns[i] != NULL && func_ptr == NULL; i++) {
            snprintf(mangled_name, sizeof(mangled_name), patterns[i], 
                    strlen(func_name), func_name);
            func_ptr = dlsym(handle, mangled_name);
            error = dlerror();
        }
    }
    
    if (error != NULL || func_ptr == NULL) {
        fprintf(stderr, "Function '%s' not found: %s\n", 
                func_name, error ? error : "Unknown error");
        dlclose(handle);
        free(input_buffer);
        return 3;
    }
    
    int result = 0;
    
    // Call the function with appropriate signature
    // We assume the most common signature (int func(char*, int))
    user_func_int_char_int func_int = (user_func_int_char_int)func_ptr;
    
    // Set up signal handling to catch crashes
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    
    // Call the function with the expected signature
    result = func_int(input_buffer, input_size);
    
    // Print result
    printf("%d\n", result);
    
    // Cleanup
    dlclose(handle);
    free(input_buffer);
    return 0;
}
