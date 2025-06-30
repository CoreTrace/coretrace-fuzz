#pragma once

#include "cli_parser.h"
#include "fuzzer.h"
#include "bytecode_transformer.h"
#include "test_result.h"

class FuzzingApplication {
public:
    explicit FuzzingApplication(const CliOptions& options);
    
    int run();
    
private:
    const CliOptions& options_;
    
    // Core methods
    void printHeader() const;
    bool loadTargets();
    void listAvailableFunctions() const;
    FuzzingStatus executeFuzzing();
    void printResults(const FuzzingStatus& status) const;
    bool exportResults() const;
    
    // Helper methods
    std::string getModeDescription() const;
    int getExitCode(const FuzzingStatus& status) const;
    
    // Members
    Fuzzer fuzzer_;
};
