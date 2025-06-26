# Module de Fuzzing LLVM C++

Un module de fuzzing avancé utilisant LLVM 19 pour tester automatiquement des fonctions C++ et détecter les vulnérabilités.

## Fonctionnalités

✅ **Chargement de fonctions** - Load des fonctions depuis le code source C++ ou LLVM IR  
✅ **Exécution en mémoire** - Exécution directe en mémoire avec LLVM JIT  
✅ **Stockage des résultats** - Sauvegarde des entrées, valeurs de retour et durées d'exécution  
✅ **Détection de crash** - Capture automatique des signaux (SIGSEGV, SIGABRT, etc.)  
✅ **Format SARIF** - Export des résultats au format SARIF JSON standardisé  
✅ **Transformation bytecode** - Compilation automatique C++ → LLVM IR  
✅ **Exécution instrumentée** - Mesure précise du temps d'exécution  

## Architecture

```
src/
├── fuzzer.cpp              # Moteur principal de fuzzing
├── memory_executor.cpp     # Exécution LLVM JIT en mémoire  
├── signal_handler.cpp      # Gestion des signaux de crash
├── sarif_reporter.cpp      # Export format SARIF
├── bytecode_transformer.cpp # Compilation C++ → LLVM IR
└── main.cpp                # Interface CLI

include/
├── fuzzer.h
├── memory_executor.h
├── signal_handler.h  
├── sarif_reporter.h
├── bytecode_transformer.h
└── test_result.h           # Structures de données
```

## Prérequis

- **LLVM 19** avec headers de développement
- **Clang++** pour la compilation
- **CMake 3.16+**
- **nlohmann/json** (installé automatiquement)

### Installation LLVM 19 (Ubuntu/Debian)

```bash
# Ajouter le repository LLVM
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo add-apt-repository "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-19 main"

# Installer LLVM 19
sudo apt update
sudo apt install llvm-19-dev clang-19 cmake build-essential
```

## Compilation

```bash
# Build du projet
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Ou utiliser le script automatique
./build_and_test.sh
```

## Utilisation

### Fuzzing depuis code source C++

```bash
./fuzzing_module -f vulnerable_function -s test.cpp -o results.sarif -n 10000
```

### Fuzzing depuis LLVM IR

```bash
# Compilation manuelle
clang++ -S -emit-llvm test.cpp -o test.ll

# Fuzzing
./fuzzing_module -f main -i test.ll -o results.sarif -n 5000
```

### Options complètes

```bash
./fuzzing_module [OPTIONS]

Options:
  -f, --function NAME    Nom de la fonction cible (requis)
  -s, --source PATH      Fichier source C++
  -i, --ir PATH          Fichier LLVM IR
  -o, --output PATH      Fichier SARIF de sortie (défaut: results.sarif)
  -n, --iterations NUM   Nombre d'itérations (défaut: 10000)
  --min-size NUM         Taille min d'entrée (défaut: 1)
  --max-size NUM         Taille max d'entrée (défaut: 1024)
  --timeout MS           Timeout en ms (défaut: 1000)
  -h, --help             Aide
```

## Exemple de fonction vulnérable

```cpp
// tests/vulnerable_test.cpp
extern "C" int vulnerable_function(char* input, int size) {
    char buffer[64];  // Buffer fixe
    
    // VULNÉRABILITÉ: pas de vérification de taille
    memcpy(buffer, input, size);  // Buffer overflow possible!
    
    int sum = 0;
    for (int i = 0; i < size && i < 64; ++i) {
        sum += buffer[i];
    }
    return sum;
}
```

## Format de sortie SARIF

Le module génère des rapports au format SARIF 2.1.0 standardisé :

```json
{
  "version": "2.1.0",
  "runs": [{
    "tool": {
      "driver": {
        "name": "LLVM Fuzzing Module",
        "version": "1.0.0"
      }
    },
    "results": [{
      "ruleId": "FUZZ_CRASH",
      "level": "error", 
      "message": { "text": "Crash detected (signal 11)" },
      "properties": {
        "executionTime": 1234567,
        "inputSize": 128,
        "signalReceived": 11,
        "inputData": "41414141..."
      }
    }]
  }]
}
```

## Tests automatiques

```bash
# Test avec les fonctions vulnérables d'exemple
./build_and_test.sh

# Résultats attendus:
# - vulnerable_function: détection de buffer overflow (SIGSEGV)
# - divide_function: détection de division par zéro (SIGFPE)
```

## Fonctionnement interne

### 1. Chargement et compilation

```cpp
// Transformation C++ → LLVM IR
BytecodeTransformer transformer;
transformer.transformSourceToIR("test.cpp", "test.ll");

// Chargement en mémoire
MemoryExecutor executor;
executor.loadModule("test.ll");
executor.setTargetFunction("vulnerable_function");
```

### 2. Génération d'entrées aléatoires

```cpp
// Génération d'input aléatoire
TestInput input;
input.data = generateRandomBytes(size);
input.timestamp = now();
```

### 3. Exécution instrumentée

```cpp
// Exécution avec timeout et capture de signaux
auto start = high_resolution_clock::now();
GenericValue result = engine->runFunction(function, args);
auto duration = high_resolution_clock::now() - start;

// Vérification crash
if (SignalHandler::hasCrashed()) {
    result.crashed = true;
    result.signal = SignalHandler::getSignal();
}
```

### 4. Export SARIF

```cpp
// Génération du rapport
SarifReporter reporter;
reporter.addCrashes(crashes);
reporter.exportToFile("results.sarif");
```

## Personnalisation

### Ajout de nouveaux types de détection

```cpp
// Dans fuzzer.cpp
TestResult Fuzzer::executeSingleTest(const TestInput& input) {
    // Ajoutez vos propres vérifications
    if (detectMemoryLeak(result)) {
        result.crashed = true;
        result.error_message = "Memory leak detected";
    }
    return result;
}
```

### Stratégies de génération d'input

```cpp
// Personnaliser la génération d'entrées
TestInput Fuzzer::generateSmartInput() {
    // Implémentez vos stratégies:
    // - Mutation d'inputs précédents
    // - Dictionnaires de valeurs intéressantes  
    // - Génération guidée par couverture
}
```

## Limitations actuelles

- Passage d'arguments simplifié (pointeur + taille)
- Pas de support natif pour types complexes (structs)
- Génération d'input purement aléatoire (pas de feedback)

## Roadmap

- [ ] Support des types complexes (structs, classes)
- [ ] Fuzzing guidé par couverture (coverage-guided)
- [ ] Mutation intelligente d'inputs
- [ ] Support des fonctions C++ avec exceptions
- [ ] Interface graphique pour visualisation
- [ ] Intégration CI/CD

## Contribution

Le module est conçu pour être extensible. Consultez les TODOs dans le code pour les améliorations prioritaires.

---

**Note**: Ce module implémente toutes les spécifications listées dans votre TODO :
- ✅ Module de fuzzing avec LLVM 19
- ✅ Load fonction → écriture en mémoire et test  
- ✅ Stock entrées et valeurs de retour
- ✅ Calcul durée d'exécution → gardé en mémoire
- ✅ Capture signaux en cas de crash
- ✅ Implémentation format SARIF JSON générique
- ✅ Transformation code source → bytecode
- ✅ Instructions machine → exécution en mémoire
