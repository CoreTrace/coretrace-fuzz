#include <cstring>
#include <cstdlib>

// Another vulnerable function for testing
extern "C" int buffer_copy_function(char* input, int size) {
    char buffer[32];  // Small buffer
    
    // Vulnerability: no size check
    strcpy(buffer, input);  // Buffer overflow if input > 31 chars
    
    return strlen(buffer);
}

// Simple math function
extern "C" int math_function(char* input, int size) {
    if (size < 4) return 0;
    
    int a = *reinterpret_cast<int*>(input);
    int b = size;
    
    return a + b * 2;
}
