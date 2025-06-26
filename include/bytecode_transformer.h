#pragma once

#include <string>
#include <vector>
#include <memory>

namespace llvm {
    class Module;
    class LLVMContext;
}

class BytecodeTransformer {
public:
    BytecodeTransformer();
    ~BytecodeTransformer();
    
    // Transform C++ source to LLVM IR
    bool transformSourceToIR(const std::string& source_path, 
                            const std::string& output_path);
    
    // Transform C++ source string to LLVM IR string
    std::string transformSourceStringToIR(const std::string& source_code);
    
    // Load and parse LLVM IR
    std::unique_ptr<llvm::Module> loadIRModule(const std::string& ir_path);
    
    // Optimize LLVM IR
    bool optimizeModule(llvm::Module* module);
    
    // Get function names from module
    std::vector<std::string> getFunctionNames(const llvm::Module* module);

private:
    std::unique_ptr<llvm::LLVMContext> context_;
    
    // Compilation helpers
    bool compileWithClang(const std::string& source_path, 
                         const std::string& output_path);
};
