#include "fuzzing_application.h"
#include <iostream>
#include <chrono>
#include <filesystem>

FuzzingApplication::FuzzingApplication(const CliOptions& options) 
    : options_(options) {
    // Configure fuzzer based on options
    if (options_.isSingleFunctionMode()) {
        fuzzer_.setTargetFunction(options_.function_name);
    } else if (!options_.function_names.empty()) {
        fuzzer_.setTargetFunctions(options_.function_names);
    }
    
    fuzzer_.setInputSize(options_.min_size, options_.max_size);
    fuzzer_.setMaxIterations(options_.iterations);
    fuzzer_.setTimeout(std::chrono::milliseconds(options_.timeout_ms));
}

int FuzzingApplication::run() {
    try {
        printHeader();
        
        if (!loadTargets()) {
            std::cerr << "Failed to load target function(s)" << std::endl;
            return 1;
        }
        
        listAvailableFunctions();
        
        std::cout << "\nStarting fuzzing process..." << std::endl;
        FuzzingStatus status = executeFuzzing();
        
        printResults(status);
        
        if (!exportResults()) {
            std::cerr << "Failed to export results" << std::endl;
            return 1;
        }
        
        return getExitCode(status);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

void FuzzingApplication::printHeader() const {
    std::cout << "=== LLVM Fuzzing Module ===" << std::endl;
    std::cout << "Mode: " << getModeDescription() << std::endl;
    std::cout << "Iterations: " << options_.iterations << std::endl;
    std::cout << "Input size range: " << options_.min_size << " - " << options_.max_size << " bytes" << std::endl;
    std::cout << "Timeout: " << options_.timeout_ms << " ms" << std::endl;
    std::cout << "Output file: " << options_.output_path << std::endl;
    std::cout << "=========================" << std::endl;
}

std::string FuzzingApplication::getModeDescription() const {
    if (options_.isSingleFunctionMode()) {
        return "Single function (" + options_.function_name + ")";
    } else if (options_.test_all_functions) {
        return "All functions";
    } else {
        std::string desc = "Specific functions (";
        for (size_t i = 0; i < options_.function_names.size(); ++i) {
            desc += options_.function_names[i];
            if (i < options_.function_names.size() - 1) desc += ", ";
        }
        desc += ")";
        return desc;
    }
}

bool FuzzingApplication::loadTargets() {
    auto sources_to_use = options_.getSourcesToUse();
    
    if (!sources_to_use.empty()) {
        if (sources_to_use.size() == 1) {
            std::cout << "Loading from source: " << sources_to_use[0] << std::endl;
            return fuzzer_.loadFunctionFromSource(sources_to_use[0]);
        } else {
            std::cout << "Loading from multiple sources: ";
            for (const auto& src : sources_to_use) {
                std::cout << src << " ";
            }
            std::cout << std::endl;
            return fuzzer_.loadMultipleSources(sources_to_use);
        }
    } else {
        std::cout << "Loading from IR: " << options_.ir_path << std::endl;
        return fuzzer_.loadFunction(options_.ir_path);
    }
}

void FuzzingApplication::listAvailableFunctions() const {
    auto sources_to_use = options_.getSourcesToUse();
    if (!sources_to_use.empty()) {
        BytecodeTransformer transformer;
        std::string temp_ir = sources_to_use[0] + ".ll";
        
        // Try to generate IR first if it doesn't exist
        if (!std::filesystem::exists(temp_ir)) {
            transformer.transformSourceToIR(sources_to_use[0], temp_ir);
        }
        
        // Extract function names from IR file or source file
        auto functions = transformer.getFunctionNames(temp_ir);
        if (functions.empty()) {
            // Fallback to source parsing if IR parsing fails
            functions = transformer.getFunctionNamesFromSource(sources_to_use[0]);
        }
        
        if (!functions.empty()) {
            std::cout << "Available functions in module:" << std::endl;
            for (const auto& func : functions) {
                std::cout << "  - " << func << std::endl;
            }
        }
    }
}

FuzzingStatus FuzzingApplication::executeFuzzing() {
    if (options_.isMultiFunctionMode()) {
        return fuzzer_.startFuzzingAllFunctions();
    } else {
        return fuzzer_.startFuzzing();
    }
}

void FuzzingApplication::printResults(const FuzzingStatus& status) const {
    (void)status; // Suppress unused parameter warning
    const auto& results = fuzzer_.getResults();
    const auto& crashes = fuzzer_.getCrashes();
    
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
            
            // Print input data (first 32 bytes)
            std::cout << "  Input data: ";
            const auto& data = crash.input.data;
            for (size_t j = 0; j < std::min(data.size(), size_t(32)); ++j) {
                printf("%02x", static_cast<unsigned char>(data[j]));
            }
            if (data.size() > 32) {
                std::cout << "...";
            }
            std::cout << std::endl << std::endl;
        }
    }
}

bool FuzzingApplication::exportResults() const {
    std::cout << "Exporting results to: " << options_.output_path << std::endl;
    if (fuzzer_.exportResults(options_.output_path)) {
        std::cout << "Results exported successfully!" << std::endl;
        return true;
    }
    return false;
}

int FuzzingApplication::getExitCode(const FuzzingStatus& status) const {
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
}
