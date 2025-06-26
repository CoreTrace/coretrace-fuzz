#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <memory>

struct TestInput {
    std::vector<uint8_t> data;
    std::chrono::high_resolution_clock::time_point timestamp;
};

struct TestResult {
    TestInput input;
    std::vector<uint8_t> return_value;
    std::chrono::nanoseconds execution_time;
    bool crashed;
    int signal_received;
    std::string error_message;
    
    TestResult() : crashed(false), signal_received(0) {}
};

enum class FuzzingStatus {
    SUCCESS,
    CRASH_DETECTED,
    TIMEOUT,
    EXECUTION_ERROR,
    MEMORY_ERROR
};
