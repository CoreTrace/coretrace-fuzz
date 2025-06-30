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
    
    // Simple function discovery by parsing the wrapper's available functions
    // For now, we'll use the known functions from the wrapper
    std::vector<std::string> known_functions = {
        "safe_add_function",
        "vulnerable_function", 
        "safe_count_bytes",
        "divide_function",
        "pointer_function",
        "loop_function",
        "buffer_copy_function",
        "math_function"
    };
    
    // In a real implementation, we could parse the source file to find extern "C" functions
    // For now, return the known functions
    return known_functions;
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
    
    // If no target functions are set, discover them
    std::vector<std::string> functions_to_fuzz = target_functions_;
    if (functions_to_fuzz.empty()) {
        if (!source_paths_.empty()) {
            functions_to_fuzz = discoverFunctionsFromMultipleSources(source_paths_);
        } else {
            std::cerr << "No functions specified and no source paths available for discovery" << std::endl;
            return FuzzingStatus::EXECUTION_ERROR;
        }
    }
    
    if (functions_to_fuzz.empty()) {
        std::cerr << "No functions found to fuzz" << std::endl;
        return FuzzingStatus::EXECUTION_ERROR;
    }
    
    std::cout << "Starting fuzzing with " << max_iterations_ << " iterations per function..." << std::endl;
    std::cout << "Functions to fuzz: ";
    for (const auto& func : functions_to_fuzz) {
        std::cout << func << " ";
    }
    std::cout << std::endl;
    
    results_.clear();
    crashes_.clear();
    
    size_t total_crashes = 0;
    
    for (const auto& function_name : functions_to_fuzz) {
        std::cout << "\n=== Fuzzing function: " << function_name << " ===" << std::endl;
        
        // Set current target function
        std::string original_target = target_function_;
        target_function_ = function_name;
        
        if (!executor_->setTargetFunction(function_name)) {
            std::cout << "Skipping function " << function_name << " (not available)" << std::endl;
            continue;
        }
        
        // Run fuzzing iterations for this function
        for (size_t i = 0; i < max_iterations_; ++i) {
            TestInput input = generateRandomInput();
            TestResult result = executeSingleTest(input, function_name);
            
            results_.push_back(result);
            
            if (result.crashed) {
                crashes_.push_back(result);
                total_crashes++;
                
                std::cout << "CRASH DETECTED at iteration " << i 
                          << " with " << signalToString(result.signal_received) << std::endl;
                std::cout << "Input data: " << formatInputData(input.data) << std::endl;
            }
        }
        
        // Restore original target
        target_function_ = original_target;
    }
    
    std::cout << "\n=== FUZZING ALL FUNCTIONS COMPLETED ===" << std::endl;
    std::cout << "Total crashes found: " << total_crashes << std::endl;
    std::cout << "Functions tested: " << functions_to_fuzz.size() << std::endl;
    
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
    return executor_->executeFunction(input, timeout_);
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
        return result;
    }
    
    TestResult result = executor_->executeFunction(input, timeout_);
    
    // Restore original function
    target_function_ = original_function;
    
    return result;
}
