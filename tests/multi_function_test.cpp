// Test file with multiple functions for comprehensive testing
#include <cstring>
#include <cstdlib>
#include <cstdio>  // For snprintf

extern "C" {

// Function 1: Buffer overflow vulnerability
void vulnerable_strcpy(const char* input) {
    char buffer[10];
    strcpy(buffer, input);  // Buffer overflow if input > 9 chars
}

// Function 2: Out-of-bounds array access
int array_overflow(const char* input) {
    int arr[5] = {1, 2, 3, 4, 5};
    if (input && input[0]) {
        int index = (unsigned char)input[0];
        return arr[index];  // Potential out-of-bounds access
    }
    return 0;
}

// Function 3: Division by zero
int divide_by_input(const char* input) {
    if (!input || !input[0]) return 0;
    int divisor = (int)input[0];
    return 100 / divisor;  // Division by zero if input[0] == 0
}

// Function 4: Memory leak (not a crash but bad practice)
void memory_leak(const char* input) {
    if (input && input[0] > 50) {
        char* leaked = (char*)malloc(100);
        // Intentionally not freeing memory
        strcpy(leaked, "leaked");
    }
}

// Function 5: Safe function that should not crash
int safe_function(const char* input) {
    if (!input) return 0;
    int sum = 0;
    for (int i = 0; i < 10 && input[i]; i++) {
        sum += (unsigned char)input[i];
    }
    return sum % 1000;  // Safe modulo operation
}

// Function 6: Null pointer dereference
void null_deref(const char* input) {
    char* ptr = nullptr;
    if (input && input[0] == 42) {
        *ptr = 'X';  // Null pointer dereference
    }
}

// Function 7: Stack overflow (recursive)
int recursive_crash(const char* input) {
    static int depth = 0;
    if (input && input[0] > 100) {
        depth++;
        if (depth < 10000) {  // Deep recursion
            return recursive_crash(input) + 1;
        }
    }
    depth = 0;
    return 0;
}

// Function 8: Format string vulnerability simulation
void format_vuln(const char* input) {
    char buffer[50];
    if (input && strlen(input) > 5) {
        snprintf(buffer, sizeof(buffer), "%s", input);
        if (strlen(input) > 40) {
            buffer[49] = '\0';  // Force null termination but might be too late
        }
    }
}

}
