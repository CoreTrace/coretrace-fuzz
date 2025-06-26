#pragma once

#include <signal.h>
#include <functional>
#include <atomic>

class SignalHandler {
public:
    static SignalHandler& getInstance();
    
    // Setup signal handlers
    void setupHandlers();
    
    // Register crash callback
    void setCrashCallback(std::function<void(int)> callback);
    
    // Check if crash occurred
    bool hasCrashed() const;
    int getSignal() const;
    void reset();

private:
    SignalHandler() = default;
    ~SignalHandler() = default;
    
    static void signalHandler(int signal);
    
    std::function<void(int)> crash_callback_;
    static std::atomic<bool> crashed_;
    static std::atomic<int> signal_received_;
    
    // Non-copyable
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
};
