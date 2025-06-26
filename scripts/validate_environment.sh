#!/bin/bash

# Script de validation de l'environnement pour le build cross-platform
# Usage: ./scripts/validate_environment.sh

set -e

echo "=== Validation de l'environnement LLVM Fuzzing Module ==="

# Détecter l'OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
    echo "🐧 Système détecté: Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
    echo "🍎 Système détecté: macOS"
else
    echo "❌ Système non supporté: $OSTYPE"
    exit 1
fi

# Vérifier les outils de base
echo ""
echo "=== Vérification des outils de base ==="

check_command() {
    if command -v "$1" &> /dev/null; then
        echo "✅ $1: $(command -v "$1")"
        return 0
    else
        echo "❌ $1: Non trouvé"
        return 1
    fi
}

MISSING_TOOLS=0

check_command "cmake" || MISSING_TOOLS=$((MISSING_TOOLS + 1))
check_command "make" || check_command "ninja" || MISSING_TOOLS=$((MISSING_TOOLS + 1))
check_command "git" || MISSING_TOOLS=$((MISSING_TOOLS + 1))

# Vérifier LLVM
echo ""
echo "=== Vérification de LLVM 19 ==="

LLVM_FOUND=false
LLVM_CONFIG=""
CLANG_CMD=""
CLANGXX_CMD=""

if [[ "$OS" == "Linux" ]]; then
    # Chemins Linux
    for prefix in "/usr/lib/llvm-19" "/usr/lib/llvm19" "/usr/local/llvm19"; do
        if [[ -d "$prefix" ]]; then
            echo "✅ LLVM trouvé dans: $prefix"
            LLVM_FOUND=true
            if [[ -f "$prefix/bin/llvm-config" ]]; then
                LLVM_CONFIG="$prefix/bin/llvm-config"
            fi
            if [[ -f "$prefix/bin/clang" ]]; then
                CLANG_CMD="$prefix/bin/clang"
                CLANGXX_CMD="$prefix/bin/clang++"
            fi
            break
        fi
    done
    
    # Vérifier les alternatives système
    if [[ "$LLVM_FOUND" == false ]]; then
        if command -v llvm-config-19 &> /dev/null; then
            LLVM_CONFIG="llvm-config-19"
            LLVM_FOUND=true
            echo "✅ LLVM trouvé via llvm-config-19"
        fi
        if command -v clang-19 &> /dev/null; then
            CLANG_CMD="clang-19"
            CLANGXX_CMD="clang++-19"
        fi
    fi

elif [[ "$OS" == "macOS" ]]; then
    # Chemins macOS Homebrew
    for prefix in "/opt/homebrew/opt/llvm@19" "/usr/local/opt/llvm@19"; do
        if [[ -d "$prefix" ]]; then
            echo "✅ LLVM trouvé dans: $prefix"
            LLVM_FOUND=true
            LLVM_CONFIG="$prefix/bin/llvm-config"
            CLANG_CMD="$prefix/bin/clang"
            CLANGXX_CMD="$prefix/bin/clang++"
            break
        fi
    done
fi

if [[ "$LLVM_FOUND" == false ]]; then
    echo "❌ LLVM 19 non trouvé"
    MISSING_TOOLS=$((MISSING_TOOLS + 1))
    
    echo ""
    echo "Instructions d'installation LLVM 19:"
    if [[ "$OS" == "Linux" ]]; then
        echo "Ubuntu/Debian:"
        echo "  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -"
        echo "  sudo add-apt-repository \"deb http://apt.llvm.org/\$(lsb_release -cs)/ llvm-toolchain-\$(lsb_release -cs)-19 main\""
        echo "  sudo apt-get update"
        echo "  sudo apt-get install llvm-19 llvm-19-dev clang-19"
    elif [[ "$OS" == "macOS" ]]; then
        echo "macOS (Homebrew):"
        echo "  brew install llvm@19"
    fi
else
    # Tester LLVM
    if [[ -n "$LLVM_CONFIG" ]] && [[ -x "$LLVM_CONFIG" ]]; then
        VERSION=$($LLVM_CONFIG --version)
        echo "✅ LLVM Version: $VERSION"
        
        if [[ "$VERSION" == 19.* ]]; then
            echo "✅ Version LLVM compatible"
        else
            echo "⚠️  Version LLVM: $VERSION (attendu: 19.x)"
        fi
    fi
    
    if [[ -n "$CLANG_CMD" ]] && [[ -x "$CLANG_CMD" ]]; then
        CLANG_VERSION=$($CLANG_CMD --version | head -1)
        echo "✅ Clang: $CLANG_VERSION"
    fi
fi

# Vérifier les bibliothèques de développement
echo ""
echo "=== Vérification des bibliothèques ==="

if [[ "$OS" == "Linux" ]]; then
    # Vérifier les headers C++
    if [[ -f "/usr/include/c++/13/iostream" ]] || [[ -f "/usr/include/c++/14/iostream" ]] || [[ -f "/usr/include/c++/15/iostream" ]]; then
        echo "✅ Headers C++ trouvés"
    else
        echo "⚠️  Headers C++ non trouvés - installer build-essential"
    fi
fi

# Test de compilation simple
echo ""
echo "=== Test de compilation simple ==="

if [[ "$LLVM_FOUND" == true ]] && [[ -n "$CLANGXX_CMD" ]]; then
    # Créer un fichier de test simple
    TEST_FILE="/tmp/llvm_test_$$.cpp"
    cat > "$TEST_FILE" << 'EOF'
#include <iostream>
int main() {
    std::cout << "Test LLVM compilation" << std::endl;
    return 0;
}
EOF

    if $CLANGXX_CMD -std=c++17 "$TEST_FILE" -o "/tmp/llvm_test_$$" 2>/dev/null; then
        echo "✅ Compilation C++ réussie"
        if "/tmp/llvm_test_$$" &>/dev/null; then
            echo "✅ Exécution réussie"
        fi
        rm -f "/tmp/llvm_test_$$"
    else
        echo "❌ Échec de compilation"
        MISSING_TOOLS=$((MISSING_TOOLS + 1))
    fi
    
    rm -f "$TEST_FILE"
fi

# Vérifier les capacités CMake
echo ""
echo "=== Vérification CMake ==="

if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -1 | grep -o '[0-9]\+\.[0-9]\+')
    echo "✅ CMake Version: $CMAKE_VERSION"
    
    # Vérifier version minimale
    if [[ $(echo "$CMAKE_VERSION >= 3.16" | bc -l 2>/dev/null || echo 0) == 1 ]]; then
        echo "✅ Version CMake compatible (>= 3.16)"
    else
        echo "⚠️  Version CMake: $CMAKE_VERSION (recommandé: >= 3.16)"
    fi
fi

# Résumé final
echo ""
echo "=== Résumé ==="

if [[ $MISSING_TOOLS -eq 0 ]]; then
    echo "🎉 Environnement prêt pour le build!"
    echo ""
    echo "Commandes suggérées:"
    echo "  mkdir -p build && cd build"
    if [[ "$OS" == "Linux" ]]; then
        echo "  cmake .. -DLLVM_DIR=/usr/lib/llvm19/lib/cmake/llvm"
    elif [[ "$OS" == "macOS" ]]; then
        echo "  cmake .. -DLLVM_DIR=/opt/homebrew/opt/llvm@19/lib/cmake/llvm"
    fi
    echo "  make -j\$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    exit 0
else
    echo "❌ $MISSING_TOOLS problème(s) détecté(s)"
    echo "Installez les outils manquants et relancez ce script."
    exit 1
fi
