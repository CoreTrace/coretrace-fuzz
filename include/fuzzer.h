#pragma once

#include "test_result.h"
#include <memory>
#include <functional>
#include <vector>
#include <random>

class MemoryExecutor;
class SarifReporter;
class SignalHandler;

class Fuzzer {
public:
    Fuzzer();
    ~Fuzzer();
    
    // Configuration
    void setTargetFunction(const std::string& function_name);
    void setTargetFunctions(const std::vector<std::string>& function_names);
    void setInputSize(size_t min_size, size_t max_size);
    void setMaxIterations(size_t iterations);
    void setTimeout(std::chrono::milliseconds timeout);
    
    // Load function from bytecode or source
    bool loadFunction(const std::string& bytecode_path);
    bool loadFunctionFromSource(const std::string& source_path);
    bool loadMultipleSources(const std::vector<std::string>& source_paths);
    
    // Auto-discover functions from source files
    std::vector<std::string> discoverFunctions(const std::string& source_path);
    std::vector<std::string> discoverFunctionsFromMultipleSources(const std::vector<std::string>& source_paths);
    
    // Start fuzzing
    FuzzingStatus startFuzzing();
    FuzzingStatus startFuzzingAllFunctions(); // Fuzz all discovered/set functions
    
    // Get results
    const std::vector<TestResult>& getResults() const;
    const std::vector<TestResult>& getCrashes() const;
    
    // Export results to SARIF format
    bool exportResults(const std::string& output_path) const;

private:
    std::unique_ptr<MemoryExecutor> executor_;
    std::unique_ptr<SarifReporter> reporter_;
    SignalHandler* signal_handler_;
    
    std::vector<TestResult> results_;
    std::vector<TestResult> crashes_;
    
    std::string target_function_;
    std::vector<std::string> target_functions_;
    std::vector<std::string> source_paths_;
    size_t min_input_size_;
    size_t max_input_size_;
    size_t max_iterations_;
    std::chrono::milliseconds timeout_;
    
    std::mt19937 rng_;
    
    // Generate random input
    TestInput generateRandomInput();
    
    // Execute single test
    TestResult executeSingleTest(const TestInput& input);
    
    // Execute single test for specific function
    TestResult executeSingleTest(const TestInput& input, const std::string& function_name);
};
