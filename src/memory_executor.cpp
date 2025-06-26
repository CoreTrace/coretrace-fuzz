#include "memory_executor.h"
#include "signal_handler.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

MemoryExecutor::MemoryExecutor() 
    : ready_(false) {
}

MemoryExecutor::~MemoryExecutor() {
    // Cleanup temporary executable
    if (!executable_path_.empty()) {
        struct stat buffer;
        if (stat(executable_path_.c_str(), &buffer) == 0) {
            unlink(executable_path_.c_str());
        }
    }
}

bool MemoryExecutor::loadModule(const std::string& source_path) {
    source_path_ = source_path;
    return compileToExecutable(source_path);
}

bool MemoryExecutor::loadModuleFromString(const std::string& source_code) {
    // Write source code to temporary file
    std::string temp_path = "/tmp/fuzz_temp_" + std::to_string(getpid()) + ".cpp";
    std::ofstream temp_file(temp_path);
    if (!temp_file) {
        std::cerr << "Failed to create temporary file" << std::endl;
        return false;
    }
    temp_file << source_code;
    temp_file.close();
    
    source_path_ = temp_path;
    return compileToExecutable(temp_path);
}

bool MemoryExecutor::setTargetFunction(const std::string& function_name) {
    target_function_ = function_name;
    return true;
}

bool MemoryExecutor::compileToExecutable(const std::string& source_path) {
    // Create temporary executable path
    executable_path_ = "/tmp/fuzz_exec_" + std::to_string(getpid());
    
    // Construct compilation command
    // Find the parent directory of source_path
    size_t last_slash = source_path.find_last_of('/');
    std::string parent_dir = (last_slash != std::string::npos) ? 
                            source_path.substr(0, last_slash) : ".";
    std::string wrapper_path = parent_dir + "/function_wrapper.cpp";
    
    std::string compile_cmd = "clang++ -O0 -g -w " + wrapper_path + " " + source_path + " -o " + executable_path_;
    
    std::cout << "Compiling with command: " << compile_cmd << std::endl;
    
    int result = system(compile_cmd.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed with code: " << result << std::endl;
        return false;
    }
    
    // Check if executable was created
    struct stat buffer;
    if (stat(executable_path_.c_str(), &buffer) != 0) {
        std::cerr << "Executable not created: " << executable_path_ << std::endl;
        return false;
    }
    
    ready_ = true;
    std::cout << "Successfully compiled to executable: " << executable_path_ << std::endl;
    return true;
}

TestResult MemoryExecutor::executeFunction(const TestInput& input, 
                                          std::chrono::milliseconds timeout) {
    if (!isReady()) {
        TestResult result;
        result.input = input;
        result.crashed = true;
        result.error_message = "Executor not ready";
        return result;
    }
    
    return executeWithTimeout(input, timeout);
}

bool MemoryExecutor::isReady() const {
    struct stat buffer;
    return ready_ && !executable_path_.empty() && (stat(executable_path_.c_str(), &buffer) == 0);
}

TestResult MemoryExecutor::executeWithTimeout(const TestInput& input,
                                             std::chrono::milliseconds timeout) {
    TestResult result;
    result.input = input;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create pipes for communication
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        result.crashed = true;
        result.error_message = "Failed to create pipes";
        return result;
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        close(stdin_pipe[1]);   // Close write end of stdin pipe
        close(stdout_pipe[0]);  // Close read end of stdout pipe
        
        // Redirect stdin and stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        // Prepare arguments
        std::string size_arg = std::to_string(input.data.size());
        
        // Execute the program
        execl(executable_path_.c_str(), executable_path_.c_str(), 
              target_function_.c_str(), size_arg.c_str(), nullptr);
        
        // If we reach here, exec failed
        perror("execl failed");
        _exit(127);
        
    } else if (pid > 0) {
        // Parent process
        close(stdin_pipe[0]);   // Close read end of stdin pipe
        close(stdout_pipe[1]);  // Close write end of stdout pipe
        
        // Write input data to child's stdin
        if (!input.data.empty()) {
            write(stdin_pipe[1], input.data.data(), input.data.size());
        }
        close(stdin_pipe[1]);
        
        // Wait for child with timeout
        int status;
        bool timeout_occurred = false;
        
        // Use alarm for timeout
        alarm(timeout.count() / 1000 + 1); // Convert to seconds + buffer
        
        pid_t wait_result = waitpid(pid, &status, 0);
        alarm(0); // Cancel alarm
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time);
        
        if (wait_result == -1) {
            // Wait failed or timeout
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            result.crashed = true;
            result.error_message = "Execution timeout or wait failed";
        } else {
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code == 0) {
                    // Success - read output
                    char output_buffer[256];
                    ssize_t bytes_read = read(stdout_pipe[0], output_buffer, sizeof(output_buffer) - 1);
                    if (bytes_read > 0) {
                        output_buffer[bytes_read] = '\0';
                        int return_value = atoi(output_buffer);
                        result.return_value.resize(sizeof(int));
                        *reinterpret_cast<int*>(result.return_value.data()) = return_value;
                    }
                    result.crashed = false;
                } else {
                    result.crashed = true;
                    result.error_message = "Process exited with code " + std::to_string(exit_code);
                }
            } else if (WIFSIGNALED(status)) {
                result.crashed = true;
                result.signal_received = WTERMSIG(status);
                result.error_message = "Process crashed with signal " + std::to_string(WTERMSIG(status));
            }
        }
        
        close(stdout_pipe[0]);
        
    } else {
        // Fork failed
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        result.crashed = true;
        result.error_message = "Failed to fork process";
    }
    
    return result;
}
