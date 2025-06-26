#pragma once

#include "test_result.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

class SarifReporter {
public:
    SarifReporter();
    
    // Set metadata
    void setToolName(const std::string& name);
    void setToolVersion(const std::string& version);
    void setTargetFile(const std::string& file_path);
    
    // Add test results
    void addResults(const std::vector<TestResult>& results);
    void addCrashes(const std::vector<TestResult>& crashes);
    
    // Generate SARIF report
    nlohmann::json generateReport();
    
    // Export to file
    bool exportToFile(const std::string& output_path);

private:
    std::string tool_name_;
    std::string tool_version_;
    std::string target_file_;
    
    std::vector<TestResult> all_results_;
    std::vector<TestResult> crashes_;
    
    // SARIF format helpers
    nlohmann::json createToolInfo();
    nlohmann::json createRules();
    nlohmann::json createResults();
    nlohmann::json createResultFromCrash(const TestResult& crash, int index);
    nlohmann::json createArtifacts();
};
