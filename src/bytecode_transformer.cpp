#include "bytecode_transformer.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <iostream>
#include <cstdlib>
#include <fstream>

BytecodeTransformer::BytecodeTransformer() 
    : context_(std::make_unique<llvm::LLVMContext>()) {
}

BytecodeTransformer::~BytecodeTransformer() = default;

bool BytecodeTransformer::transformSourceToIR(const std::string& source_path, 
                                              const std::string& output_path) {
    // Use clang to compile C++ to LLVM IR
    return compileWithClang(source_path, output_path);
}

std::string BytecodeTransformer::transformSourceStringToIR(const std::string& source_code) {
    // Write source to temporary file
    std::string temp_source = "/tmp/fuzz_source.cpp";
    std::string temp_ir = "/tmp/fuzz_source.ll";
    
    std::ofstream source_file(temp_source);
    if (!source_file.is_open()) {
        std::cerr << "Failed to create temporary source file" << std::endl;
        return "";
    }
    
    source_file << source_code;
    source_file.close();
    
    // Transform to IR
    if (!transformSourceToIR(temp_source, temp_ir)) {
        return "";
    }
    
    // Read IR back
    std::ifstream ir_file(temp_ir);
    if (!ir_file.is_open()) {
        std::cerr << "Failed to read temporary IR file" << std::endl;
        return "";
    }
    
    std::string ir_content((std::istreambuf_iterator<char>(ir_file)),
                           std::istreambuf_iterator<char>());
    
    // Clean up temporary files
    std::remove(temp_source.c_str());
    std::remove(temp_ir.c_str());
    
    return ir_content;
}

std::unique_ptr<llvm::Module> BytecodeTransformer::loadIRModule(const std::string& ir_path) {
    llvm::SMDiagnostic error;
    auto module = llvm::parseIRFile(ir_path, error, *context_);
    
    if (!module) {
        std::cerr << "Error loading IR module: " << error.getMessage().str() << std::endl;
        return nullptr;
    }
    
    return module;
}

bool BytecodeTransformer::optimizeModule(llvm::Module* module) {
    if (!module) {
        return false;
    }

    try {
        // Use the new PassBuilder for LLVM 19
        llvm::LoopAnalysisManager LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager CGAM;
        llvm::ModuleAnalysisManager MAM;
        
        llvm::PassBuilder PB;
        
        // Register the analysis managers
        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
        
        // Create a basic optimization pipeline
        llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
        
        // Run the optimization pipeline
        MPM.run(*module, MAM);
        
        std::cout << "Module optimized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error optimizing module: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> BytecodeTransformer::getFunctionNames(const llvm::Module* module) {
    std::vector<std::string> function_names;
    
    if (!module) {
        return function_names;
    }
    
    for (const auto& function : *module) {
        if (!function.isDeclaration() && function.hasName()) {
            function_names.push_back(function.getName().str());
        }
    }
    
    return function_names;
}

bool BytecodeTransformer::compileWithClang(const std::string& source_path, 
                                          const std::string& output_path) {
    // Build clang command
    std::string command = "clang++ -S -emit-llvm -O0 -g ";
    command += "\"" + source_path + "\" -o \"" + output_path + "\"";
    
    std::cout << "Compiling with command: " << command << std::endl;
    
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Clang compilation failed with exit code: " << result << std::endl;
        return false;
    }
    
    // Check if output file was created
    std::ifstream test_file(output_path);
    if (!test_file.good()) {
        std::cerr << "Output IR file was not created: " << output_path << std::endl;
        return false;
    }
    
    std::cout << "Successfully compiled to IR: " << output_path << std::endl;
    return true;
}
