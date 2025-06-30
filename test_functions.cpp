#include <cstring>
#include <cstdlib>

// Vulnerable function - buffer overflow
void vulnerable_function(const char* input) {
    char buffer[100];
    strcpy(buffer, input);  // Potential buffer overflow
}

// Safe function
void safe_function(const char* input) {
    char buffer[100];
    if (strlen(input) < 100) {
        strcpy(buffer, input);
    }
}

// Another vulnerable function - null pointer dereference
void null_ptr_function(int* ptr) {
    if (ptr == nullptr) {
        *ptr = 42;  // Null pointer dereference
    }
}

// Integer overflow vulnerability
void overflow_function(int a, int b) {
    int result = a + b;
    if (result < a) {  // Overflow detection
        abort();
    }
}

// Array bounds vulnerability
void array_bounds_function(int index) {
    int arr[10];
    if (index >= 0 && index < 20) {  // Wrong bounds check
        arr[index] = 42;  // Potential out-of-bounds access
    }
}
