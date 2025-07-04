#include "bytecode_transformer.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <regex>
#include <sstream>
#include <cstring>

BytecodeTransformer::BytecodeTransformer() {
}

BytecodeTransformer::~BytecodeTransformer() = default;

bool BytecodeTransformer::transformSourceToIR(const std::string& source_path, 
                                              const std::string& output_path) {
    // Use compilerlib to transform source to LLVM IR
    return compileWithCompilerLib(source_path, output_path);
}

std::string BytecodeTransformer::transformSourceStringToIR(const std::string& source_code) {
    // Write source to temporary file (C only)
    std::string temp_source = "/tmp/fuzz_source.c";
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

std::vector<std::string> BytecodeTransformer::getFunctionNames(const std::string& ir_path) {
    std::vector<std::string> function_names;
    
    // Try to read the IR file and extract function names
    std::ifstream ir_file(ir_path);
    if (!ir_file.is_open()) {
        std::cerr << "Failed to open IR file: " << ir_path << std::endl;
        return function_names;
    }
    
    std::string ir_content((std::istreambuf_iterator<char>(ir_file)),
                           std::istreambuf_iterator<char>());
    ir_file.close();
    
    return extractFunctionNamesFromIR(ir_content);
}

std::vector<std::string> BytecodeTransformer::getFunctionNamesFromSource(const std::string& source_path) {
    std::vector<std::string> function_names;
    
    std::ifstream source_file(source_path);
    if (!source_file.is_open()) {
        std::cerr << "Failed to open source file: " << source_path << std::endl;
        return function_names;
    }
    
    std::string source_content((std::istreambuf_iterator<char>(source_file)),
                               std::istreambuf_iterator<char>());
    source_file.close();
    
    return extractFunctionNamesFromSourceRegex(source_content);
}

// Fonction auxiliaire pour vérifier si un répertoire existe
bool BytecodeTransformer::directory_exists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

// Fonction pour détecter automatiquement les chemins d'inclusion GCC et Clang
std::vector<std::string> BytecodeTransformer::detect_include_paths() {
    std::vector<std::string> include_paths;
    
    // Chemins de base à vérifier
    include_paths.push_back("/usr/include");
    include_paths.push_back("/usr/local/include");
    
    // Détecter les chemins d'inclusion GCC
    const std::string gcc_base = "/usr/lib/gcc/x86_64-pc-linux-gnu/";
    std::filesystem::path gcc_path(gcc_base);
    
    if (std::filesystem::exists(gcc_path)) {
        // Parcourir toutes les versions GCC installées
        for (const auto& entry : std::filesystem::directory_iterator(gcc_path)) {
            if (entry.is_directory()) {
                std::string version_path = entry.path().string() + "/include";
                if (directory_exists(version_path)) {
                    include_paths.push_back(version_path);
                }
            }
        }
    }
    
    // Détecter les chemins d'inclusion Clang
    const std::string clang_base = "/usr/lib/clang/";
    std::filesystem::path clang_path(clang_base);
    
    if (std::filesystem::exists(clang_path)) {
        // Parcourir toutes les versions Clang installées
        for (const auto& entry : std::filesystem::directory_iterator(clang_path)) {
            if (entry.is_directory()) {
                std::string version_path = entry.path().string() + "/include";
                if (directory_exists(version_path)) {
                    include_paths.push_back(version_path);
                }
            }
        }
    }
    
    return include_paths;
}

bool BytecodeTransformer::compileWithCompilerLib(const std::string& source_path, 
                                                 const std::string& output_path) {
    try {
        // Only support C files - check extension
        if (source_path.length() < 2 || source_path.substr(source_path.length() - 2) != ".c") {
            std::cerr << "Error: Only C files (.c extension) are supported" << std::endl;
            return false;
        }
        
        // Detect include paths automatically
        std::vector<std::string> include_paths = detect_include_paths();
        
        // Configure compilation arguments for C files only
        std::vector<std::string> compiler_args;
        
        // Add compiler name
        compiler_args.push_back("clang");
        
        // Add compilation options
        compiler_args.push_back("-S");
        compiler_args.push_back("-emit-llvm");
        compiler_args.push_back("-O0");
        compiler_args.push_back("-g");
        
        // Add C-specific flags
        compiler_args.push_back("-fPIC");
        compiler_args.push_back("-rdynamic");
        
        // Add detected include paths
        for (const auto& path : include_paths) {
            compiler_args.push_back("-isystem");
            compiler_args.push_back(path);
        }
        
        // Add output and input files
        compiler_args.push_back("-o");
        compiler_args.push_back(output_path);
        compiler_args.push_back(source_path);
        
        std::cout << "Compiling C file using clang directly" << std::endl;
        
        // Display arguments for debugging
        std::cout << "Command: ";
        for (const auto& arg : compiler_args) {
            std::cout << arg << " ";
        }
        std::cout << std::endl;
        
        // Skip dynamic library loading and use clang directly for C files
        std::cout << "Using direct clang compilation for C files (C++ support disabled)" << std::endl;
        return compileWithClangFallback(source_path, output_path);
    } catch (const std::exception& e) {
        std::cerr << "Exception in compileWithCompilerLib: " << e.what() << std::endl;
        return compileWithClangFallback(source_path, output_path);
    }
}

bool BytecodeTransformer::compileWithClangFallback(const std::string& source_path, 
                                                  const std::string& output_path) {
    try {
        // Only support C files
        if (source_path.length() < 2 || source_path.substr(source_path.length() - 2) != ".c") {
            std::cerr << "Error: Only C files (.c extension) are supported" << std::endl;
            return false;
        }
        
        // Build clang command for C files only
        std::string command = "clang -S -emit-llvm -O0 -g -fPIC -rdynamic";
        command += " \"" + source_path + "\" -o \"" + output_path + "\"";
        
        std::cout << "Using C compilation: " << command << std::endl;
        
        int result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "C compilation failed with exit code: " << result << std::endl;
            
            // Try a simpler command in case of failure
            std::string simple_command = "clang -S -emit-llvm -o \"" + output_path + "\" \"" + source_path + "\"";
            std::cout << "Trying simpler C command: " << simple_command << std::endl;
            
            result = std::system(simple_command.c_str());
            if (result != 0) {
                std::cerr << "Simple C command also failed with exit code: " << result << std::endl;
                return false;
            }
        }
        
        // Check if output file was created and has content
        std::ifstream test_file(output_path);
        if (!test_file.good()) {
            std::cerr << "Output IR file was not created: " << output_path << std::endl;
            return false;
        }
        
        test_file.seekg(0, std::ios::end);
        size_t size = test_file.tellg();
        if (size == 0) {
            std::cerr << "Output IR file is empty: " << output_path << std::endl;
            return false;
        }
        
        std::cout << "Successfully compiled C file to IR: " << output_path << " (size: " << size << " bytes)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in compileWithClangFallback: " << e.what() << std::endl;
        return false;
    }
}

// Helper method to extract function names from LLVM IR content using text parsing
std::vector<std::string> BytecodeTransformer::extractFunctionNamesFromIR(const std::string& ir_content) {
    std::vector<std::string> function_names;
    
    // Regex to match LLVM IR function definitions
    // Pattern: define [attributes] return_type @function_name(parameters)
    std::regex function_regex(R"(define\s+(?:[^@]*\s+)?@(\w+)\s*\()");
    
    std::sregex_iterator iter(ir_content.begin(), ir_content.end(), function_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        if (match.size() > 1) {
            std::string func_name = match[1].str();
            // Filter out internal/compiler-generated functions
            if (func_name != "main" && 
                func_name.find("llvm.") == std::string::npos &&
                func_name.find("__") == std::string::npos) {
                function_names.push_back(func_name);
            }
        }
    }
    
    return function_names;
}

// Helper method to extract function names from source code using regex
std::vector<std::string> BytecodeTransformer::extractFunctionNamesFromSourceRegex(const std::string& source_content) {
    std::vector<std::string> function_names;
    
    // Remove comments to avoid false positives
    std::string cleaned_content = source_content;
    
    // Remove single-line comments
    std::regex single_comment(R"(//.*$)", std::regex_constants::multiline);
    cleaned_content = std::regex_replace(cleaned_content, single_comment, "");
    
    // Remove multi-line comments
    std::regex multi_comment(R"(/\*.*?\*/)", std::regex_constants::ECMAScript);
    cleaned_content = std::regex_replace(cleaned_content, multi_comment, "");
    
    // Regex patterns for function definitions
    std::vector<std::regex> patterns = {
        // C function pattern: return_type function_name(parameters) {
        std::regex(R"((?:^|\n)\s*(?:static\s+|extern\s+|inline\s+)*\s*(?:const\s+)?(?:unsigned\s+|signed\s+)?(?:void|int|char|float|double|long|short|struct\s+\w+|\w+(?:\s*\*)*)\s+(\w+)\s*\([^)]*\)\s*\{)", std::regex_constants::multiline),
        
        // Alternative pattern for functions with pointer returns
        std::regex(R"((?:^|\n)\s*(?:static\s+|extern\s+|inline\s+)*\s*(?:\w+\s*\*+\s*|\w+\s+)(\w+)\s*\([^)]*\)\s*\{)", std::regex_constants::multiline)
    };
    
    for (const auto& pattern : patterns) {
        std::sregex_iterator iter(cleaned_content.begin(), cleaned_content.end(), pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::smatch match = *iter;
            if (match.size() > 1) {
                std::string func_name = match[1].str();
                
                // Filter out common keywords and invalid function names
                if (func_name != "if" && func_name != "while" && func_name != "for" && 
                    func_name != "switch" && func_name != "return" && func_name != "main" &&
                    func_name != "struct" && func_name != "typedef" && func_name != "enum" &&
                    std::find(function_names.begin(), function_names.end(), func_name) == function_names.end()) {
                    function_names.push_back(func_name);
                }
            }
        }
    }
    
    return function_names;
}
