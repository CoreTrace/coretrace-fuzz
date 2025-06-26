#include "fuzzer.h"
#include "bytecode_transformer.h"
#include <llvm/IR/Module.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// Utility function to split strings by comma
std::vector<std::string> splitByComma(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        }
    }
    
    return result;
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "Options:\n"
              << "  -f, --function NAME    Target function name (required for single function)\n"
              << "  -s, --source PATH      Source file path (C++)\n"
              << "  --sources PATH1,PATH2  Multiple source file paths (comma-separated)\n"
              << "  --all-functions        Test all functions found in source files\n"
              << "  --functions FUNC1,FUNC2 Specific functions to test (comma-separated)\n"
              << "  -i, --ir PATH          LLVM IR file path\n"
              << "  -o, --output PATH      Output SARIF file path (default: results.sarif)\n"
              << "  -n, --iterations NUM   Number of fuzzing iterations (default: 10000)\n"
              << "  --min-size NUM         Minimum input size (default: 1)\n"
              << "  --max-size NUM         Maximum input size (default: 1024)\n"
              << "  --timeout MS           Execution timeout in milliseconds (default: 1000)\n"
              << "  -h, --help             Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " -f vulnerable_function -s test.cpp -o results.sarif\n"
              << "  " << program_name << " --all-functions -s test.cpp\n"
              << "  " << program_name << " --sources test1.cpp,test2.cpp --all-functions\n"
              << "  " << program_name << " --functions func1,func2 -s test.cpp\n"
              << "  " << program_name << " -f main -i test.ll -n 50000 --timeout 500\n";
}

int main(int argc, char* argv[]) {
    std::string function_name;
    std::string source_path;
    std::string ir_path;
    std::string output_path = "results.sarif";
    std::vector<std::string> source_paths;
    std::vector<std::string> function_names;
    bool test_all_functions = false;
    size_t iterations = 10000;
    size_t min_size = 1;
    size_t max_size = 1024;
    int timeout_ms = 1000;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-f" || arg == "--function") {
            if (++i < argc) {
                function_name = argv[i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-s" || arg == "--source") {
            if (++i < argc) {
                source_path = argv[i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--sources") {
            if (++i < argc) {
                source_paths = splitByComma(argv[i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--all-functions") {
            test_all_functions = true;
        } else if (arg == "--functions") {
            if (++i < argc) {
                function_names = splitByComma(argv[i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-i" || arg == "--ir") {
            if (++i < argc) {
                ir_path = argv[i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (++i < argc) {
                output_path = argv[i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-n" || arg == "--iterations") {
            if (++i < argc) {
                iterations = std::stoul(argv[i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--min-size") {
            if (++i < argc) {
                min_size = std::stoul(argv[i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--max-size") {
            if (++i < argc) {
                max_size = std::stoul(argv[i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--timeout") {
            if (++i < argc) {
                timeout_ms = std::stoi(argv[i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate arguments
    bool single_function_mode = !function_name.empty();
    bool multi_function_mode = test_all_functions || !function_names.empty();
    bool multi_source_mode = !source_paths.empty();
    
    if (!single_function_mode && !multi_function_mode) {
        std::cerr << "Error: Must specify either:\n";
        std::cerr << "  - Single function mode: -f/--function\n";
        std::cerr << "  - Multi function mode: --all-functions or --functions\n";
        printUsage(argv[0]);
        return 1;
    }
    
    if (single_function_mode && multi_function_mode) {
        std::cerr << "Error: Cannot use single function mode with multi function mode\n";
        return 1;
    }
    
    // Determine source files to use
    std::vector<std::string> sources_to_use;
    if (multi_source_mode) {
        sources_to_use = source_paths;
    } else if (!source_path.empty()) {
        sources_to_use.push_back(source_path);
    }
    
    if (sources_to_use.empty() && ir_path.empty()) {
        std::cerr << "Error: Either source file(s) or IR file is required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    if (!sources_to_use.empty() && !ir_path.empty()) {
        std::cerr << "Error: Cannot specify both source file(s) and IR file\n";
        return 1;
    }
    
    std::cout << "=== LLVM Fuzzing Module ===" << std::endl;
    if (single_function_mode) {
        std::cout << "Target function: " << function_name << std::endl;
    } else if (test_all_functions) {
        std::cout << "Mode: Test all functions" << std::endl;
    } else {
        std::cout << "Target functions: ";
        for (size_t i = 0; i < function_names.size(); ++i) {
            std::cout << function_names[i];
            if (i < function_names.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Input size range: " << min_size << " - " << max_size << " bytes" << std::endl;
    std::cout << "Timeout: " << timeout_ms << " ms" << std::endl;
    std::cout << "Output file: " << output_path << std::endl;
    std::cout << "=========================" << std::endl;
    
    try {
        // Create fuzzer
        Fuzzer fuzzer;
        
        // Configure fuzzer
        if (single_function_mode) {
            fuzzer.setTargetFunction(function_name);
        } else if (!function_names.empty()) {
            fuzzer.setTargetFunctions(function_names);
        }
        fuzzer.setInputSize(min_size, max_size);
        fuzzer.setMaxIterations(iterations);
        fuzzer.setTimeout(std::chrono::milliseconds(timeout_ms));
        
        // Load function(s)
        bool loaded = false;
        if (!sources_to_use.empty()) {
            if (sources_to_use.size() == 1) {
                std::cout << "Loading from source: " << sources_to_use[0] << std::endl;
                loaded = fuzzer.loadFunctionFromSource(sources_to_use[0]);
            } else {
                std::cout << "Loading from multiple sources: ";
                for (const auto& src : sources_to_use) {
                    std::cout << src << " ";
                }
                std::cout << std::endl;
                loaded = fuzzer.loadMultipleSources(sources_to_use);
            }
        } else {
            std::cout << "Loading from IR: " << ir_path << std::endl;
            loaded = fuzzer.loadFunction(ir_path);
        }
        
        if (!loaded) {
            std::cerr << "Failed to load target function(s)" << std::endl;
            return 1;
        }
        
        // List available functions if transformation was used
        if (!sources_to_use.empty()) {
            BytecodeTransformer transformer;
            std::string temp_ir = sources_to_use[0] + ".ll";
            auto module = transformer.loadIRModule(temp_ir);
            if (module) {
                auto functions = transformer.getFunctionNames(module.get());
                std::cout << "Available functions in module:" << std::endl;
                for (const auto& func : functions) {
                    std::cout << "  - " << func << std::endl;
                }
            }
        }
        
        // Start fuzzing
        std::cout << "\nStarting fuzzing process..." << std::endl;
        FuzzingStatus status;
        
        if (multi_function_mode) {
            status = fuzzer.startFuzzingAllFunctions();
        } else {
            status = fuzzer.startFuzzing();
        }
        
        // Print results
        const auto& results = fuzzer.getResults();
        const auto& crashes = fuzzer.getCrashes();
        
        std::cout << "\n=== FUZZING RESULTS ===" << std::endl;
        std::cout << "Total tests: " << results.size() << std::endl;
        std::cout << "Crashes found: " << crashes.size() << std::endl;
        
        if (!crashes.empty()) {
            std::cout << "\n=== CRASH DETAILS ===" << std::endl;
            for (size_t i = 0; i < crashes.size(); ++i) {
                const auto& crash = crashes[i];
                std::cout << "Crash #" << (i + 1) << ":" << std::endl;
                std::cout << "  Signal: " << crash.signal_received << std::endl;
                std::cout << "  Input size: " << crash.input.data.size() << " bytes" << std::endl;
                std::cout << "  Execution time: " << crash.execution_time.count() << " ns" << std::endl;
                if (!crash.error_message.empty()) {
                    std::cout << "  Error: " << crash.error_message << std::endl;
                }
                std::cout << std::endl;
            }
        }
        
        // Export results
        std::cout << "Exporting results to: " << output_path << std::endl;
        if (fuzzer.exportResults(output_path)) {
            std::cout << "Results exported successfully!" << std::endl;
        } else {
            std::cerr << "Failed to export results" << std::endl;
            return 1;
        }
        
        // Return appropriate exit code
        switch (status) {
            case FuzzingStatus::SUCCESS:
                std::cout << "Fuzzing completed successfully - no crashes found" << std::endl;
                return 0;
            case FuzzingStatus::CRASH_DETECTED:
                std::cout << "Fuzzing completed - crashes detected!" << std::endl;
                return 2; // Different exit code for crashes found
            default:
                std::cerr << "Fuzzing failed with error" << std::endl;
                return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
