# Module de Fuzzing CoreTrace - Résumé

Ce document présente un résumé des fonctionnalités, fonctions et bibliothèques utilisées dans le module de fuzzing CoreTrace.

## Fonctionnalités Principales

### 1. Support Multi-Langage
- **Analyse C/C++** : Support complet pour les fichiers source en C et C++
- **Détection automatique** : Reconnaissance automatique du type de fichier (.c ou .cpp) et utilisation des compilateurs appropriés

### 2. Découverte de Fonctions
- **Mode `--all-functions`** : Analyse automatique du code source pour découvrir toutes les fonctions testables
- **Méthodes multiples** : Utilisation de l'analyse LLVM IR et expressions régulières pour une découverte robuste
- **Support des fonctions C** : Détection sans nécessiter de déclaration `extern "C"`

### 3. Détection et Rapport de Crash
- **Identification des crashes** : Capture des signaux et erreurs d'exécution
- **Rapport détaillé** : Inclut le nom de la fonction qui a crashé, le type de signal, et la valeur d'entrée
- **Format SARIF** : Génération de rapports standardisés pour l'intégration CI/CD

### 4. Exécution Sécurisée
- **Isolation des processus** : Exécution dans des processus séparés pour éviter les crashs du programme principal
- **Gestion des timeouts** : Détection des fonctions qui entrent en boucle infinie
- **Manipulation mémoire sécurisée** : Utilisation de wrappers pour l'exécution des fonctions

## Principales Classes et Fonctions

### BytecodeTransformer
- `compileWithClang()` : Compilation de code source en bytecode LLVM
- `compileToIR()` : Conversion du code en représentation intermédiaire LLVM

### Fuzzer
- `discoverFunctions()` : Analyse le code source pour trouver toutes les fonctions
- `startFuzzing()` : Lance le processus de fuzzing sur une fonction
- `startFuzzingAllFunctions()` : Exécute le fuzzing sur toutes les fonctions découvertes
- `executeSingleTest()` : Exécute un test avec une entrée donnée

### MemoryExecutor
- `loadModule()` : Charge le code source à tester
- `compileToExecutable()` : Compile le code source avec le wrapper approprié
- `executeFunction()` : Exécute une fonction avec une entrée spécifique
- `executeWithTimeout()` : Exécute avec surveillance du délai d'attente

### SarifReporter
- `addResults()` : Ajoute les résultats de test au rapport
- `addCrashes()` : Ajoute les informations de crash au rapport
- `generateReport()` : Génère un rapport SARIF complet
- `exportToFile()` : Sauvegarde le rapport au format JSON

### SignalHandler
- `setupHandlers()` : Configuration des gestionnaires de signaux
- `reset()` : Réinitialisation de l'état entre les tests
- `setCrashCallback()` : Définition des actions à exécuter lors d'un crash

## Structures de Données Clés

### TestResult
- `input` : Données d'entrée du test
- `crashed` : Indicateur de crash
- `signal_received` : Signal reçu en cas de crash
- `error_message` : Message d'erreur détaillé
- `function_name` : Nom de la fonction testée
- `execution_time` : Temps d'exécution

### TestInput
- `data` : Vecteur d'octets utilisé comme entrée
- `timestamp` : Horodatage de la génération

## Bibliothèques Externes

### LLVM
- **Analyse du bytecode** : Extraction des informations de fonction
- **Compilation** : Transformation du code source en représentation intermédiaire
- **Outils** : `llvm-config`, `clang`, `clang++`

### nlohmann::json
- **Génération de rapport SARIF** : Création des structures JSON
- **Sérialisation** : Conversion des données en format JSON

### Bibliothèques système
- **<signal.h>** : Gestion des signaux et des crashs
- **<dlfcn.h>** : Chargement dynamique des fonctions (utilisé dans les wrappers)
- **<sys/wait.h>** et **<unistd.h>** : Création et surveillance des processus

## Wrappers

### Dynamic Wrapper
- `dynamic_wrapper.c` : Wrapper basique pour l'appel dynamique de fonctions
- `dynamic_wrapper_improved.c` : Version améliorée avec support C/C++ robuste

### Static Wrapper
- `function_wrapper.cpp` : Wrapper pour l'appel statique de fonctions

## Scripts de Test et Validation

### Scripts principaux
- `test_all_functionality.sh` : Test complet de toutes les fonctionnalités
- `test_discovery.sh` : Validation de la découverte de fonctions C/C++
- `test_function_name_in_report.sh` : Vérification des noms de fonction dans les rapports de crash

### Fichiers de test
- `crash_c_test.c` : Fonctions C conçues pour crasher
- `vulnerable_test.cpp` : Fonctions C++ avec vulnérabilités
- `discovery_test.c` : Test de découverte de fonctions C
- `pure_c_test.c` : Validation du support C pur
