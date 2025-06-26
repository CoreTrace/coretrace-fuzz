#include "sarif_reporter.h"
#include "utils.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

SarifReporter::SarifReporter() 
    : tool_name_("LLVM Fuzzing Module")
    , tool_version_("1.0.0") {
}

void SarifReporter::setToolName(const std::string& name) {
    tool_name_ = name;
}

void SarifReporter::setToolVersion(const std::string& version) {
    tool_version_ = version;
}

void SarifReporter::setTargetFile(const std::string& file_path) {
    target_file_ = file_path;
}

void SarifReporter::addResults(const std::vector<TestResult>& results) {
    all_results_ = results;
}

void SarifReporter::addCrashes(const std::vector<TestResult>& crashes) {
    crashes_ = crashes;
}

json SarifReporter::generateReport() {
    json sarif;
    
    // SARIF version and schema
    sarif["version"] = "2.1.0";
    sarif["$schema"] = "https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json";
    
    // Runs array
    json run;
    run["tool"] = createToolInfo();
    run["artifacts"] = createArtifacts();
    run["results"] = createResults();
    
    // Add rules if we have crashes
    if (!crashes_.empty()) {
        run["tool"]["driver"]["rules"] = createRules();
    }
    
    // Statistics
    json stats;
    stats["totalTests"] = all_results_.size();
    stats["crashes"] = crashes_.size();
    stats["successRate"] = all_results_.empty() ? 0.0 : 
        (double)(all_results_.size() - crashes_.size()) / all_results_.size();
    
    run["properties"] = json::object();
    run["properties"]["statistics"] = stats;
    
    sarif["runs"] = json::array({run});
    
    return sarif;
}

bool SarifReporter::exportToFile(const std::string& output_path) {
    try {
        json report = generateReport();
        
        std::ofstream file(output_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open output file: " << output_path << std::endl;
            return false;
        }
        
        file << std::setw(2) << report << std::endl;
        file.close();
        
        std::cout << "SARIF report exported to: " << output_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error generating SARIF report: " << e.what() << std::endl;
        return false;
    }
}

json SarifReporter::createToolInfo() {
    json tool;
    json driver;
    
    driver["name"] = tool_name_;
    driver["version"] = tool_version_;
    driver["informationUri"] = "https://github.com/your-repo/fuzzing-module";
    driver["fullName"] = tool_name_ + " - LLVM-based Fuzzing Tool";
    
    tool["driver"] = driver;
    return tool;
}

json SarifReporter::createRules() {
    json rules = json::array();
    
    // Crash detection rule
    json crash_rule;
    crash_rule["id"] = "FUZZ_CRASH";
    crash_rule["name"] = "CrashDetection";
    crash_rule["shortDescription"]["text"] = "Crash detected during fuzzing";
    crash_rule["fullDescription"]["text"] = "A crash was detected while executing the target function with fuzzed input";
    crash_rule["defaultConfiguration"]["level"] = "error";
    crash_rule["helpUri"] = "https://github.com/your-repo/fuzzing-module/docs/crashes";
    
    rules.push_back(crash_rule);
    
    return rules;
}

json SarifReporter::createResults() {
    json results = json::array();
    
    // Add crash results
    for (size_t i = 0; i < crashes_.size(); ++i) {
        results.push_back(createResultFromCrash(crashes_[i], i));
    }
    
    return results;
}

json SarifReporter::createResultFromCrash(const TestResult& crash, int index) {
    json result;
    
    result["ruleId"] = "FUZZ_CRASH";
    result["ruleIndex"] = 0;
    result["level"] = "error";
    
    // Message
    std::stringstream msg;
    msg << "Crash detected";
    if (crash.signal_received != 0) {
        msg << " (" << signalToString(crash.signal_received) << ")";
    }
    if (!crash.error_message.empty()) {
        msg << ": " << crash.error_message;
    }
    
    result["message"]["text"] = msg.str();
    
    // Location (if we have target file)
    if (!target_file_.empty()) {
        json location;
        location["physicalLocation"]["artifactLocation"]["uri"] = target_file_;
        result["locations"] = json::array({location});
    }
    
    // Properties with crash details
    json properties;
    properties["crashIndex"] = index;
    properties["executionTime"] = crash.execution_time.count();
    properties["inputSize"] = crash.input.data.size();
    properties["signalReceived"] = crash.signal_received;
    if (crash.signal_received != 0) {
        properties["signalName"] = signalToString(crash.signal_received);
    }
    
    // Add input data in readable format
    properties["inputData"] = formatInputData(crash.input.data, 256);
    
    // Also add as hex string for programmatic access
    std::stringstream hex_input;
    size_t max_bytes = std::min(crash.input.data.size(), size_t(256));
    for (size_t i = 0; i < max_bytes; ++i) {
        hex_input << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(crash.input.data[i]);
    }
    if (crash.input.data.size() > max_bytes) {
        hex_input << "... (truncated)";
    }
    properties["inputDataHex"] = hex_input.str();
    
    result["properties"] = properties;
    
    return result;
}

json SarifReporter::createArtifacts() {
    json artifacts = json::array();
    
    if (!target_file_.empty()) {
        json artifact;
        artifact["location"]["uri"] = target_file_;
        artifact["mimeType"] = "application/llvm-ir";
        artifacts.push_back(artifact);
    }
    
    return artifacts;
}
