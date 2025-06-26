#include <cstring>
#include <cstdlib>

// Fonction vulnérable avec buffer overflow
extern "C" int vulnerable_function(char* input, int size) {
    char buffer[64];  // Buffer fixe de 64 bytes
    
    // Vulnérabilité: pas de vérification de taille
    memcpy(buffer, input, size);  // Potential buffer overflow!
    
    int sum = 0;
    for (int i = 0; i < size && i < 64; ++i) {
        sum += buffer[i];
    }
    
    return sum;
}

// Fonction avec division par zéro
extern "C" int divide_function(char* input, int size) {
    if (size < 4) return 0;
    
    int divisor = *reinterpret_cast<int*>(input);
    int dividend = 1000;
    
    // Vulnérabilité: division par zéro si input[0-3] == 0
    return dividend / divisor;
}

// Fonction avec accès mémoire non valide
extern "C" int pointer_function(char* input, int size) {
    if (size < sizeof(void*)) return 0;
    
    // Interpréter input comme un pointeur
    void** ptr = reinterpret_cast<void**>(input);
    
    // Vulnérabilité: déréférencement de pointeur arbitraire
    return *reinterpret_cast<int*>(*ptr);
}

// Fonction avec boucle infinie potentielle
extern "C" int loop_function(char* input, int size) {
    if (size < 4) return 0;
    
    int count = *reinterpret_cast<int*>(input);
    int result = 0;
    
    // Vulnérabilité: boucle potentiellement infinie
    for (int i = 0; i < count; ++i) {
        result += i;
        if (result < 0) break;  // Protection contre overflow
    }
    
    return result;
}
