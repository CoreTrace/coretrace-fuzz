#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <signal.h>
#include <algorithm>

// Utility function to convert signal number to name
inline std::string signalToString(int signal) {
    switch (signal) {
        case SIGSEGV: return "SIGSEGV (Segmentation fault)";
        case SIGABRT: return "SIGABRT (Abort)";
        case SIGFPE: return "SIGFPE (Floating point exception)";
        case SIGILL: return "SIGILL (Illegal instruction)";
        case SIGBUS: return "SIGBUS (Bus error)";
        case SIGTRAP: return "SIGTRAP (Trap)";
        case SIGKILL: return "SIGKILL (Kill)";
        case SIGTERM: return "SIGTERM (Terminate)";
        case SIGPIPE: return "SIGPIPE (Broken pipe)";
        default: return "SIGNAL" + std::to_string(signal) + " (Unknown)";
    }
}

// Utility function to format input data for display
inline std::string formatInputData(const std::vector<uint8_t>& data, size_t max_display = 64) {
    if (data.empty()) {
        return "<empty>";
    }
    
    std::stringstream ss;
    ss << "\"";
    
    for (size_t i = 0; i < std::min(data.size(), max_display); ++i) {
        uint8_t byte = data[i];
        if (byte >= 32 && byte <= 126) {
            // Printable ASCII
            if (byte == '"' || byte == '\\') {
                ss << "\\" << static_cast<char>(byte);
            } else {
                ss << static_cast<char>(byte);
            }
        } else {
            // Non-printable, show as hex
            ss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
    }
    
    if (data.size() > max_display) {
        ss << "...(+" << (data.size() - max_display) << " more bytes)";
    }
    
    ss << "\"";
    return ss.str();
}

#endif // UTILS_H
