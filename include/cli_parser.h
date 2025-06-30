#pragma once

#include <string>
#include <vector>

struct CliOptions {
    // Function configuration
    std::string function_name;
    std::vector<std::string> function_names;
    bool test_all_functions = false;
    
    // Input configuration
    std::string source_path;
    std::vector<std::string> source_paths;
    std::string ir_path;
    
    // Output configuration
    std::string output_path = "results.sarif";
    
    // Fuzzing parameters
    size_t iterations = 10000;
    size_t min_size = 1;
    size_t max_size = 1024;
    int timeout_ms = 1000;
    
    // Help
    bool show_help = false;
    
    // Validation methods
    bool isSingleFunctionMode() const { return !function_name.empty(); }
    bool isMultiFunctionMode() const { return test_all_functions || !function_names.empty(); }
    bool isMultiSourceMode() const { return !source_paths.empty(); }
    
    std::vector<std::string> getSourcesToUse() const {
        if (isMultiSourceMode()) {
            return source_paths;
        } else if (!source_path.empty()) {
            return {source_path};
        }
        return {};
    }
};

class CliParser {
public:
    static CliOptions parse(int argc, char* argv[]);
    static void printUsage(const char* program_name);
    static bool validateOptions(const CliOptions& options);
    
private:
    static std::vector<std::string> splitByComma(const std::string& str);
};
