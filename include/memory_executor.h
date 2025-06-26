#pragma once

#include "test_result.h"
#include <memory>
#include <string>

class MemoryExecutor {
public:
    MemoryExecutor();
    ~MemoryExecutor();
    
    // Load and compile source file
    bool loadModule(const std::string& source_path);
    bool loadModuleFromString(const std::string& source_code);
    
    // Set target function
    bool setTargetFunction(const std::string& function_name);
    
    // Execute function with input data
    TestResult executeFunction(const TestInput& input, 
                              std::chrono::milliseconds timeout);
    
    // Check if ready for execution
    bool isReady() const;

private:
    std::string source_path_;
    std::string target_function_;
    std::string executable_path_;
    bool ready_;
    
    // Compile source to executable
    bool compileToExecutable(const std::string& source_path);
    
    // Execute with timeout using external process
    TestResult executeWithTimeout(const TestInput& input,
                                 std::chrono::milliseconds timeout);
};
