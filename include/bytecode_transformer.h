#pragma once

#include <string>
#include <vector>
#include <memory>

class BytecodeTransformer {
public:
    BytecodeTransformer();
    ~BytecodeTransformer();
    
    // Transform C/C++ source to LLVM IR using libcompilerlib.so
    bool transformSourceToIR(const std::string& source_path, 
                            const std::string& output_path);
    
    // Transform C/C++ source string to LLVM IR string using libcompilerlib.so
    std::string transformSourceStringToIR(const std::string& source_code);
    
    // Get function names from IR file (text parsing method)
    std::vector<std::string> getFunctionNames(const std::string& ir_path);
    
    // Parse function names directly from source file (fallback method)
    std::vector<std::string> getFunctionNamesFromSource(const std::string& source_path);

private:
    // Compilation using libcompilerlib.so
    bool compileWithCompilerLib(const std::string& source_path, 
                               const std::string& output_path);
    
    // Fallback compilation method using clang directly
    bool compileWithClangFallback(const std::string& source_path, 
                                 const std::string& output_path);
    
    // Helper methods for include path detection and file operations
    bool directory_exists(const std::string& path);
    std::vector<std::string> detect_include_paths();
    
    // Helper methods for function name extraction
    std::vector<std::string> extractFunctionNamesFromIR(const std::string& ir_content);
    std::vector<std::string> extractFunctionNamesFromSourceRegex(const std::string& source_content);
};
