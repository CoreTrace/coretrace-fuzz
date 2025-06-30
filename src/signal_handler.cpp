#include "signal_handler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>  // For _exit()

std::atomic<bool> SignalHandler::crashed_(false);
std::atomic<int> SignalHandler::signal_received_(0);

SignalHandler& SignalHandler::getInstance() {
    static SignalHandler instance;
    return instance;
}

void SignalHandler::setupHandlers() {
    // Setup handlers for common crash signals
    signal(SIGSEGV, signalHandler);  // Segmentation fault
    signal(SIGABRT, signalHandler);  // Abort
    signal(SIGFPE, signalHandler);   // Floating point exception
    signal(SIGILL, signalHandler);   // Illegal instruction
    signal(SIGBUS, signalHandler);   // Bus error
    signal(SIGTRAP, signalHandler);  // Trap
}

void SignalHandler::setCrashCallback(std::function<void(int)> callback) {
    crash_callback_ = std::move(callback);
}

bool SignalHandler::hasCrashed() const {
    return crashed_.load();
}

int SignalHandler::getSignal() const {
    return signal_received_.load();
}

void SignalHandler::reset() {
    crashed_.store(false);
    signal_received_.store(0);
}

void SignalHandler::signalHandler(int signal) {
    crashed_.store(true);
    signal_received_.store(signal);
    
    // Get instance and call callback if set
    auto& instance = getInstance();
    if (instance.crash_callback_) {
        instance.crash_callback_(signal);
    }
    
    // Print signal info
    const char* signal_name = "UNKNOWN";
    switch (signal) {
        case SIGSEGV: signal_name = "SIGSEGV (Segmentation fault)"; break;
        case SIGABRT: signal_name = "SIGABRT (Abort)"; break;
        case SIGFPE: signal_name = "SIGFPE (Floating point exception)"; break;
        case SIGILL: signal_name = "SIGILL (Illegal instruction)"; break;
        case SIGBUS: signal_name = "SIGBUS (Bus error)"; break;
        case SIGTRAP: signal_name = "SIGTRAP (Trap)"; break;
    }
    
    std::cerr << "Signal caught: " << signal_name << " (" << signal << ")" << std::endl;
    
    // Exit immediately to prevent infinite loops
    _exit(signal);
}
