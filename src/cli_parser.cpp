#include "cli_parser.h"
#include <iostream>
#include <sstream>

std::vector<std::string> CliParser::splitByComma(const std::string& str) {
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

void CliParser::printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\nTarget Selection:\n"
              << "  -f, --function NAME    Target function name (required for single function)\n"
              << "  --functions FUNC1,FUNC2 Specific functions to test (comma-separated)\n"
              << "  --all-functions        Test all functions found in source files\n"
              << "\nInput Sources:\n"
              << "  -s, --source PATH      Source file path (C++)\n"
              << "  --sources PATH1,PATH2  Multiple source file paths (comma-separated)\n"
              << "  -i, --ir PATH          LLVM IR file path\n"
              << "\nOutput Configuration:\n"
              << "  -o, --output PATH      Output SARIF file path (default: results.sarif)\n"
              << "\nFuzzing Parameters:\n"
              << "  -n, --iterations NUM   Number of fuzzing iterations (default: 10000)\n"
              << "  --min-size NUM         Minimum input size (default: 1)\n"
              << "  --max-size NUM         Maximum input size (default: 1024)\n"
              << "  --timeout MS           Execution timeout in milliseconds (default: 1000)\n"
              << "\nHelp:\n"
              << "  -h, --help             Show this help message\n"
              << "\nExamples:\n"
              << "  # Single function fuzzing\n"
              << "  " << program_name << " -f vulnerable_function -s test.cpp -o results.sarif\n"
              << "\n  # All functions in a file\n"
              << "  " << program_name << " --all-functions -s test.cpp\n"
              << "\n  # Multiple source files\n"
              << "  " << program_name << " --sources test1.cpp,test2.cpp --all-functions\n"
              << "\n  # Specific functions\n"
              << "  " << program_name << " --functions func1,func2 -s test.cpp\n"
              << "\n  # From LLVM IR\n"
              << "  " << program_name << " -f main -i test.ll -n 50000 --timeout 500\n";
}

CliOptions CliParser::parse(int argc, char* argv[]) {
    CliOptions options;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            options.show_help = true;
            return options; // Early return for help
        } else if (arg == "-f" || arg == "--function") {
            if (++i < argc) {
                options.function_name = argv[i];
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "-s" || arg == "--source") {
            if (++i < argc) {
                options.source_path = argv[i];
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "--sources") {
            if (++i < argc) {
                options.source_paths = splitByComma(argv[i]);
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "--all-functions") {
            options.test_all_functions = true;
        } else if (arg == "--functions") {
            if (++i < argc) {
                options.function_names = splitByComma(argv[i]);
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "-i" || arg == "--ir") {
            if (++i < argc) {
                options.ir_path = argv[i];
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "-o" || arg == "--output") {
            if (++i < argc) {
                options.output_path = argv[i];
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "-n" || arg == "--iterations") {
            if (++i < argc) {
                try {
                    options.iterations = std::stoul(argv[i]);
                } catch (const std::exception&) {
                    throw std::invalid_argument("Error: Invalid number for iterations: " + std::string(argv[i]));
                }
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "--min-size") {
            if (++i < argc) {
                try {
                    options.min_size = std::stoul(argv[i]);
                } catch (const std::exception&) {
                    throw std::invalid_argument("Error: Invalid number for min-size: " + std::string(argv[i]));
                }
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "--max-size") {
            if (++i < argc) {
                try {
                    options.max_size = std::stoul(argv[i]);
                } catch (const std::exception&) {
                    throw std::invalid_argument("Error: Invalid number for max-size: " + std::string(argv[i]));
                }
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else if (arg == "--timeout") {
            if (++i < argc) {
                try {
                    options.timeout_ms = std::stoi(argv[i]);
                    if (options.timeout_ms <= 0) {
                        throw std::invalid_argument("Error: Timeout must be positive");
                    }
                } catch (const std::exception&) {
                    throw std::invalid_argument("Error: Invalid number for timeout: " + std::string(argv[i]));
                }
            } else {
                throw std::invalid_argument("Error: " + arg + " requires an argument");
            }
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }
    
    return options;
}

bool CliParser::validateOptions(const CliOptions& options) {
    // Check function mode consistency
    if (!options.isSingleFunctionMode() && !options.isMultiFunctionMode()) {
        std::cerr << "Error: Must specify either:\n";
        std::cerr << "  - Single function mode: -f/--function\n";
        std::cerr << "  - Multi function mode: --all-functions or --functions\n";
        return false;
    }
    
    if (options.isSingleFunctionMode() && options.isMultiFunctionMode()) {
        std::cerr << "Error: Cannot use single function mode with multi function mode\n";
        return false;
    }
    
    // Check input source requirements
    auto sources_to_use = options.getSourcesToUse();
    if (sources_to_use.empty() && options.ir_path.empty()) {
        std::cerr << "Error: Either source file(s) or IR file is required\n";
        return false;
    }
    
    if (!sources_to_use.empty() && !options.ir_path.empty()) {
        std::cerr << "Error: Cannot specify both source file(s) and IR file\n";
        return false;
    }
    
    // Validate size ranges
    if (options.min_size > options.max_size) {
        std::cerr << "Error: min-size (" << options.min_size 
                  << ") cannot be greater than max-size (" << options.max_size << ")\n";
        return false;
    }
    
    if (options.min_size == 0) {
        std::cerr << "Error: min-size must be at least 1\n";
        return false;
    }
    
    return true;
}
