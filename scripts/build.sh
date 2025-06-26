#!/bin/bash

# Script de build cross-platform pour LLVM Fuzzing Module
# Usage: ./scripts/build.sh [Release|Debug] [clean]

set -e

# Configuration par défaut
BUILD_TYPE="${1:-Release}"
CLEAN_BUILD="${2:-}"
BUILD_DIR="build"

echo "=== Build LLVM Fuzzing Module ==="
echo "Type de build: $BUILD_TYPE"

# Détecter l'OS et configurer LLVM
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "🐧 Configuration pour Linux"
    
    # Chercher LLVM 19
    LLVM_PATHS=(
        "/usr/lib/llvm19/lib/cmake/llvm"
        "/usr/lib/llvm-19/lib/cmake/llvm"
        "/usr/local/llvm19/lib/cmake/llvm"
    )
    
    LLVM_DIR=""
    for path in "${LLVM_PATHS[@]}"; do
        if [[ -d "$path" ]]; then
            LLVM_DIR="$path"
            echo "✅ LLVM trouvé: $LLVM_DIR"
            break
        fi
    done
    
    if [[ -z "$LLVM_DIR" ]]; then
        echo "❌ LLVM 19 non trouvé dans les chemins standards"
        echo "Chemins vérifiés:"
        printf '  %s\n' "${LLVM_PATHS[@]}"
        exit 1
    fi
    
    # Compilers
    if command -v clang-19 &> /dev/null; then
        CC_COMPILER="clang-19"
        CXX_COMPILER="clang++-19"
    elif command -v clang &> /dev/null; then
        CC_COMPILER="clang"
        CXX_COMPILER="clang++"
    else
        CC_COMPILER="gcc"
        CXX_COMPILER="g++"
    fi
    
    MAKE_CMD="make"
    PARALLEL_JOBS=$(nproc)

elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "🍎 Configuration pour macOS"
    
    # Chercher LLVM via Homebrew
    LLVM_PATHS=(
        "/opt/homebrew/opt/llvm@19/lib/cmake/llvm"
        "/usr/local/opt/llvm@19/lib/cmake/llvm"
    )
    
    LLVM_DIR=""
    for path in "${LLVM_PATHS[@]}"; do
        if [[ -d "$path" ]]; then
            LLVM_DIR="$path"
            echo "✅ LLVM trouvé: $LLVM_DIR"
            break
        fi
    done
    
    if [[ -z "$LLVM_DIR" ]]; then
        echo "❌ LLVM 19 non trouvé"
        echo "Installez LLVM 19 avec: brew install llvm@19"
        exit 1
    fi
    
    # Utiliser LLVM Clang
    LLVM_PREFIX=$(dirname $(dirname "$LLVM_DIR"))
    CC_COMPILER="$LLVM_PREFIX/bin/clang"
    CXX_COMPILER="$LLVM_PREFIX/bin/clang++"
    
    MAKE_CMD="make"
    PARALLEL_JOBS=$(sysctl -n hw.ncpu)

else
    echo "❌ Système non supporté: $OSTYPE"
    exit 1
fi

echo "Compilateurs: $CC_COMPILER / $CXX_COMPILER"
echo "Jobs parallèles: $PARALLEL_JOBS"

# Nettoyer si demandé
if [[ "$CLEAN_BUILD" == "clean" ]]; then
    echo "🧹 Nettoyage du dossier build..."
    rm -rf "$BUILD_DIR"
fi

# Créer le dossier de build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Vérifier si Ninja est disponible
if command -v ninja &> /dev/null; then
    echo "⚡ Utilisation de Ninja pour un build plus rapide"
    GENERATOR="-GNinja"
    MAKE_CMD="ninja"
    PARALLEL_ARGS=""
else
    echo "🔨 Utilisation de Make"
    GENERATOR=""
    PARALLEL_ARGS="-j$PARALLEL_JOBS"
fi

# Configuration CMake
echo ""
echo "=== Configuration CMake ==="

CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DLLVM_DIR=$LLVM_DIR"
    "-DCMAKE_C_COMPILER=$CC_COMPILER"
    "-DCMAKE_CXX_COMPILER=$CXX_COMPILER"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

if [[ -n "$GENERATOR" ]]; then
    CMAKE_ARGS+=("$GENERATOR")
fi

echo "Arguments CMake: ${CMAKE_ARGS[*]}"

if ! cmake .. "${CMAKE_ARGS[@]}"; then
    echo "❌ Échec de la configuration CMake"
    exit 1
fi

echo "✅ Configuration réussie"

# Build
echo ""
echo "=== Build ==="

if ! $MAKE_CMD $PARALLEL_ARGS; then
    echo "❌ Échec du build"
    exit 1
fi

echo "✅ Build réussi!"

# Vérifier l'exécutable
if [[ -f "fuzzing_module" ]]; then
    echo ""
    echo "=== Test de l'exécutable ==="
    
    # Test basique
    if ./fuzzing_module --help &>/dev/null; then
        echo "✅ Exécutable fonctionnel"
        
        # Afficher les informations
        echo ""
        echo "Informations de l'exécutable:"
        file fuzzing_module
        ls -lh fuzzing_module
        
        echo ""
        echo "Usage:"
        ./fuzzing_module --help | head -10
        
    else
        echo "⚠️  Exécutable généré mais test basique échoué"
    fi
else
    echo "❌ Exécutable 'fuzzing_module' non trouvé"
    exit 1
fi

# Suggestions
echo ""
echo "=== Build terminé ==="
echo "Exécutable: $(pwd)/fuzzing_module"
echo ""
echo "Tests suggérés:"
echo "  # Test avec code source"
echo "  ./fuzzing_module -f safe_add_function -s ../tests/safe_test.cpp -n 1000"
echo ""
echo "  # Test avec LLVM IR"
if [[ -f "../tests/safe_test.cpp.ll" ]]; then
    echo "  ./fuzzing_module -f safe_add_function -i ../tests/safe_test.cpp.ll -n 1000"
else
    echo "  # (Générer d'abord un fichier .ll avec clang++ -S -emit-llvm)"
fi
echo ""
echo "  # Test de toutes les fonctions"
echo "  ./fuzzing_module --all-functions -s ../tests/safe_test.cpp -n 500"
