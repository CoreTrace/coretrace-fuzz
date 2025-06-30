// Simple safe test with no vulnerabilities
extern "C" {

int safe_add(const char* input) {
    if (!input) return 0;
    
    int result = 0;
    for (int i = 0; i < 5 && input[i] != '\0'; i++) {
        result += (unsigned char)input[i];
    }
    return result;
}

int safe_multiply(const char* input) {
    if (!input || !input[0]) return 1;
    
    int result = 1;
    for (int i = 0; i < 3 && input[i] != '\0'; i++) {
        if (input[i] != 0) {  // Avoid multiplication by zero
            result *= ((unsigned char)input[i] % 10 + 1);  // Ensure positive
        }
    }
    return result;
}

int safe_hash(const char* input) {
    if (!input) return 0;
    
    unsigned int hash = 5381;
    for (int i = 0; i < 10 && input[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + (unsigned char)input[i];
    }
    return (int)(hash % 1000);
}

}
