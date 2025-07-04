#include "fuzzer.h"
#include "memory_executor.h"
#include "signal_handler.h"
#include "sarif_reporter.h"
#include "bytecode_transformer.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <thread>
#include <regex>
#include <fstream>
#include <llvm/IR/Module.h>
#include <fstream>
#include <regex>
#include <cstdio>

Fuzzer::Fuzzer() 
    : executor_(std::make_unique<MemoryExecutor>())
    , reporter_(std::make_unique<SarifReporter>())
    , signal_handler_(&SignalHandler::getInstance())
    , min_input_size_(1)
    , max_input_size_(1024)
    , max_iterations_(10000)
    , timeout_(std::chrono::milliseconds(1000))
    , rng_(std::random_device{}())
{
    // Setup signal handling
    signal_handler_->setupHandlers();
    signal_handler_->setCrashCallback([this](int signal) {
        std::cout << "Crash detected with " << signalToString(signal) << std::endl;
    });
    
    // Configure SARIF reporter
    reporter_->setToolName("LLVM Fuzzing Module");
    reporter_->setToolVersion("1.0.0");
}

Fuzzer::~Fuzzer() = default;

void Fuzzer::setTargetFunction(const std::string& function_name) {
    target_function_ = function_name;
}

void Fuzzer::setTargetFunctions(const std::vector<std::string>& function_names) {
    target_functions_ = function_names;
}

void Fuzzer::setInputSize(size_t min_size, size_t max_size) {
    min_input_size_ = min_size;
    max_input_size_ = max_size;
}

void Fuzzer::setMaxIterations(size_t iterations) {
    max_iterations_ = iterations;
}

void Fuzzer::setTimeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
}

bool Fuzzer::loadFunction(const std::string& bytecode_path) {
    if (!executor_->loadModule(bytecode_path)) {
        std::cerr << "Failed to load LLVM module from: " << bytecode_path << std::endl;
        return false;
    }
    
    if (!target_function_.empty()) {
        if (!executor_->setTargetFunction(target_function_)) {
            std::cerr << "Failed to set target function: " << target_function_ << std::endl;
            return false;
        }
    }
    
    reporter_->setTargetFile(bytecode_path);
    return true;
}

bool Fuzzer::loadFunctionFromSource(const std::string& source_path) {
    // Load source directly into executor
    if (!executor_->loadModule(source_path)) {
        std::cerr << "Failed to load source file: " << source_path << std::endl;
        return false;
    }
    
    if (!target_function_.empty()) {
        if (!executor_->setTargetFunction(target_function_)) {
            std::cerr << "Failed to set target function: " << target_function_ << std::endl;
            return false;
        }
    }
    
    reporter_->setTargetFile(source_path);
    source_paths_.clear();
    source_paths_.push_back(source_path);
    return true;
}

bool Fuzzer::loadMultipleSources(const std::vector<std::string>& source_paths) {
    source_paths_ = source_paths;
    
    // For multiple sources, we'll use the first one as primary
    if (!source_paths.empty()) {
        if (!loadFunctionFromSource(source_paths[0])) {
            return false;
        }
        // Store all paths for later use
        source_paths_ = source_paths;
    }
    return true;
}

std::vector<std::string> Fuzzer::discoverFunctions(const std::string& source_path) {
    std::vector<std::string> functions;
    
    std::cout << "Starting function discovery for file: " << source_path << std::endl;
    
    // First, try to compile to LLVM IR and extract function names (most reliable method)
    BytecodeTransformer transformer;
    std::string temp_ir = source_path + ".temp.ll";
    
    if (transformer.transformSourceToIR(source_path, temp_ir)) {
        auto discovered = transformer.getFunctionNames(temp_ir);
        
        // Filter out standard library functions and only keep user-defined functions
        for (const auto& func : discovered) {
            // Skip standard library functions, main, and internal functions
            if (func != "main" && 
                    func.find("llvm.") == std::string::npos &&
                    func.find("__") != 0 &&
                    func.find("_GLOBAL_") == std::string::npos) {
                    functions.push_back(func);
            }
        }
        
        // Clean up temp file
        std::remove(temp_ir.c_str());
        
        if (!functions.empty()) {
            std::cout << "Discovered functions from " << source_path << " (LLVM IR):" << std::endl;
            for (const auto& func : functions) {
                std::cout << "  - " << func << std::endl;
            }
            return functions;
        }
    }
    
    // Fallback: Use source file parsing method
    auto discovered_from_source = transformer.getFunctionNamesFromSource(source_path);
    for (const auto& func : discovered_from_source) {
        // Apply same filtering
        if (func != "main" && 
                func.find("__") != 0 &&
                func.find("_GLOBAL_") == std::string::npos) {
                functions.push_back(func);
        }
    }
    
    if (!functions.empty()) {
        std::cout << "Discovered functions from " << source_path << " (source parsing):" << std::endl;
        for (const auto& func : functions) {
            std::cout << "  - " << func << std::endl;
        }
        return functions;
    }
    
    // Final fallback: Advanced text parsing for function signatures
    std::ifstream file(source_path);
    if (!file.is_open()) {
        std::cerr << "Cannot open source file for function discovery: " << source_path << std::endl;
        return functions;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // More comprehensive regex for C and C++ function declarations
    // This captures functions with various return types and parameter lists
    std::regex cpp_func_pattern(R"((?:^|\n|\s)(?:static\s+)?(?:inline\s+)?(?:__attribute__\s*\(\([^)]*\)\)\s*)?(?:int|void|char\*|char\s*\*|double|float|bool|size_t|unsigned|unsigned\s+\w+|long|long\s+\w+|short|short\s+\w+|struct\s+\w+|\w+_t|\w+::\w+|\w+<[^>]*>|\w+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^{;]*)\)\s*(?:const)?\s*(?:noexcept)?\s*(?:override)?\s*(?:final)?\s*\{)");
    
    // C-specific pattern to find extern declarations
    std::regex c_extern_pattern(R"(extern\s+(?:"C"\s+)?(?:int|void|char\*|double|float|size_t|unsigned|long|short|\w+_t|\w+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^;]*)\)\s*;)");
    
    std::smatch match;
    std::string::const_iterator search_start(content.cbegin());
    
    while (std::regex_search(search_start, content.cend(), match, cpp_func_pattern)) {
        std::string func_name = match[1].str();
        
        // Skip main and other special functions
        if (func_name != "main" && func_name != "operator" && 
            func_name.find("__") != 0) {
            functions.push_back(func_name);
        }
        
        search_start = match.suffix().first;
    }
    
    // Also search for extern C declarations that might be in header files
    search_start = content.cbegin();
    while (std::regex_search(search_start, content.cend(), match, c_extern_pattern)) {
        std::string func_name = match[1].str();
        
        // Add if not already in the list
        if (func_name != "main" && func_name.find("__") != 0 &&
            std::find(functions.begin(), functions.end(), func_name) == functions.end()) {
            functions.push_back(func_name);
        }
        
        search_start = match.suffix().first;
    }
    
    if (!functions.empty()) {
        std::cout << "Discovered functions from " << source_path << " (text parsing):" << std::endl;
        for (const auto& func : functions) {
            std::cout << "  - " << func << std::endl;
        }
    }
    
    return functions;
}

std::vector<std::string> Fuzzer::discoverFunctionsFromMultipleSources(const std::vector<std::string>& source_paths) {
    std::vector<std::string> all_functions;
    
    for (const auto& path : source_paths) {
        auto functions = discoverFunctions(path);
        all_functions.insert(all_functions.end(), functions.begin(), functions.end());
    }
    
    // Remove duplicates
    std::sort(all_functions.begin(), all_functions.end());
    all_functions.erase(std::unique(all_functions.begin(), all_functions.end()), all_functions.end());
    
    return all_functions;
}

FuzzingStatus Fuzzer::startFuzzing() {
    if (!executor_->isReady()) {
        std::cerr << "Executor not ready for fuzzing" << std::endl;
        return FuzzingStatus::EXECUTION_ERROR;
    }
    
    std::cout << "Starting fuzzing with " << max_iterations_ << " iterations..." << std::endl;
    
    results_.clear();
    crashes_.clear();
    
    for (size_t i = 0; i < max_iterations_; ++i) {
        // Generate random input
        TestInput input = generateRandomInput();
        
        // Execute test
        TestResult result = executeSingleTest(input);
        
        // Store result
        results_.push_back(result);
        
        // Check for crash
        if (result.crashed) {
            crashes_.push_back(result);
            std::cout << "CRASH DETECTED at iteration " << i << " with ";
            if (result.signal_received > 0) {
                std::cout << signalToString(result.signal_received);
            } else {
                std::cout << "error: " << result.error_message;
            }
            std::cout << std::endl;
            std::cout << "Input data: " << formatInputData(input.data) << std::endl;
            
            // Add small delay after crash to ensure system stability
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Always reset signal handler state after each test
        signal_handler_->reset();
    }
    
    std::cout << "Fuzzing completed. Total crashes: " << crashes_.size() << std::endl;
    
    // Add results to reporter
    reporter_->addResults(results_);
    reporter_->addCrashes(crashes_);
    
    return crashes_.empty() ? FuzzingStatus::SUCCESS : FuzzingStatus::CRASH_DETECTED;
}

FuzzingStatus Fuzzer::startFuzzingAllFunctions() {
    if (!executor_->isReady()) {
        std::cerr << "Executor not ready for fuzzing" << std::endl;
        return FuzzingStatus::EXECUTION_ERROR;
    }
    
    // If no target functions are set, discover them from source files
    std::vector<std::string> functions_to_fuzz = target_functions_;
    if (functions_to_fuzz.empty()) {
        if (!source_paths_.empty()) {
            std::cout << "Discovering functions from source files..." << std::endl;
            functions_to_fuzz = discoverFunctionsFromMultipleSources(source_paths_);
        } else {
            std::cerr << "No functions specified and no source paths available for function discovery" << std::endl;
            return FuzzingStatus::EXECUTION_ERROR;
        }
    }
    
    if (functions_to_fuzz.empty()) {
        std::cerr << "No functions found to fuzz. Please check if the source file contains properly defined functions." << std::endl;
        return FuzzingStatus::EXECUTION_ERROR;
    }
    
    std::cout << "\n=== Starting fuzzing with " << max_iterations_ << " iterations per function ===" << std::endl;
    std::cout << "Total functions to fuzz: " << functions_to_fuzz.size() << std::endl;
    
    // Print function names with proper formatting
    std::cout << "Functions to fuzz:" << std::endl;
    for (const auto& func : functions_to_fuzz) {
        std::cout << "  - " << func << std::endl;
    }
    std::cout << std::endl;
    
    results_.clear();
    crashes_.clear();
    
    size_t total_crashes = 0;
    size_t functions_with_crashes = 0;
    size_t functions_skipped = 0;
    size_t functions_succeeded = 0;
    
    for (const auto& function_name : functions_to_fuzz) {
        std::cout << "\n=== Fuzzing function: " << function_name << " ===" << std::endl;
        
        // Set current target function
        std::string original_target = target_function_;
        target_function_ = function_name;
        
        // Try to set the target function
        if (!executor_->setTargetFunction(function_name)) {
            std::cout << "Warning: Function '" << function_name << "' could not be loaded. Skipping..." << std::endl;
            functions_skipped++;
            continue;
        }
        
        bool function_had_crash = false;
        
        // Run fuzzing iterations for this function
        for (size_t i = 0; i < max_iterations_; ++i) {
            TestInput input = generateRandomInput();
            TestResult result = executeSingleTest(input, function_name);
            
            results_.push_back(result);
            
            if (result.crashed) {
                crashes_.push_back(result);
                total_crashes++;
                if (!function_had_crash) {
                    function_had_crash = true;
                    functions_with_crashes++;
                }
                
                std::cout << "CRASH DETECTED in function '" << function_name 
                          << "' at iteration " << i << " with ";
                          
                if (result.signal_received > 0) {
                    std::cout << signalToString(result.signal_received);
                } else {
                    std::cout << "error: " << result.error_message;
                }
                std::cout << std::endl;
                
                std::cout << "Input data: " << formatInputData(input.data) << std::endl;
                
                // Add small delay after crash to ensure system stability
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            // Reset signal handler state after each test
            signal_handler_->reset();
        }
        
        if (!function_had_crash) {
            functions_succeeded++;
        }
        
        // Restore original target
        target_function_ = original_target;
    }
    
    std::cout << "\n=== FUZZING ALL FUNCTIONS COMPLETED ===" << std::endl;
    std::cout << "Total crashes found: " << total_crashes << std::endl;
    std::cout << "Functions with crashes: " << functions_with_crashes << std::endl;
    std::cout << "Functions successfully tested: " << functions_succeeded << std::endl;
    std::cout << "Functions skipped: " << functions_skipped << std::endl;
    std::cout << "Total functions tested: " << (functions_succeeded + functions_with_crashes) << std::endl;
    
    // Add results to reporter
    reporter_->addResults(results_);
    reporter_->addCrashes(crashes_);
    
    return crashes_.empty() ? FuzzingStatus::SUCCESS : FuzzingStatus::CRASH_DETECTED;
}

const std::vector<TestResult>& Fuzzer::getResults() const {
    return results_;
}

const std::vector<TestResult>& Fuzzer::getCrashes() const {
    return crashes_;
}

bool Fuzzer::exportResults(const std::string& output_path) const {
    return reporter_->exportToFile(output_path);
}

TestInput Fuzzer::generateRandomInput() {
    TestInput input;
    input.timestamp = std::chrono::high_resolution_clock::now();
    
    // Generate random size between min and max
    std::uniform_int_distribution<size_t> size_dist(min_input_size_, max_input_size_);
    size_t input_size = size_dist(rng_);
    
    // Generate random bytes
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    input.data.reserve(input_size);
    
    for (size_t i = 0; i < input_size; ++i) {
        input.data.push_back(byte_dist(rng_));
    }
    
    return input;
}

TestResult Fuzzer::executeSingleTest(const TestInput& input) {
    TestResult result = executor_->executeFunction(input, timeout_);
    result.function_name = target_function_; // Store the current target function name
    return result;
}

TestResult Fuzzer::executeSingleTest(const TestInput& input, const std::string& function_name) {
    // Temporarily set the function and execute
    std::string original_function = target_function_;
    target_function_ = function_name;
    
    if (!executor_->setTargetFunction(function_name)) {
        // Create a failed result if function is not available
        TestResult result;
        result.input = input;
        result.crashed = true;
        result.signal_received = 0;
        result.error_message = "Function not available: " + function_name;
        result.execution_time = std::chrono::nanoseconds(0);
        result.function_name = function_name; // Store the function name
        return result;
    }
    
    TestResult result = executor_->executeFunction(input, timeout_);
    result.function_name = function_name; // Store the function name in the result
    
    // Restore original function
    target_function_ = original_function;
    
    return result;
}
