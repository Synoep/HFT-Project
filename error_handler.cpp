#include "error_handler.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

// Singleton instance
ErrorHandler* ErrorHandler::instance_ = nullptr;
std::mutex ErrorHandler::instance_mutex_;

ErrorHandler* ErrorHandler::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        instance_ = new ErrorHandler();
    }
    return instance_;
}

ErrorHandler::ErrorHandler()
    : max_log_size_(10 * 1024 * 1024)  // 10MB default
    , log_rotation_count_(5)
    , recovery_enabled_(true)
    , error_count_(0)
    , recovery_attempt_count_(0) {
    
    log_directory_ = "logs";
    std::filesystem::create_directories(log_directory_);
    
    // Open initial log file
    auto now = std::chrono::system_clock::now();
    std::string filename = log_directory_ + "/error_log_" + 
                          formatTimestamp(now) + ".log";
    log_file_.open(filename, std::ios::app);
}

ErrorHandler::~ErrorHandler() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void ErrorHandler::logError(ErrorSeverity severity,
                          const std::string& message,
                          const std::string& context,
                          const std::string& source_file,
                          int line_number,
                          const std::string& function_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ErrorInfo error;
    error.severity = severity;
    error.message = message;
    error.context = context;
    error.timestamp = std::chrono::system_clock::now();
    error.source_file = source_file;
    error.line_number = line_number;
    error.function_name = function_name;
    error.stack_trace = getStackTrace();
    
    error_history_.push_back(error);
    if (error_history_.size() > 1000) {  // Keep last 1000 errors
        error_history_.erase(error_history_.begin());
    }
    
    error_count_++;
    
    writeToLog(error);
    notifyCallbacks(error);
    
    if (severity == ErrorSeverity::CRITICAL && recovery_enabled_) {
        attemptRecovery(error);
    }
}

void ErrorHandler::addRecoveryAction(const RecoveryAction& action) {
    std::lock_guard<std::mutex> lock(mutex_);
    recovery_actions_.push_back(action);
    std::sort(recovery_actions_.begin(), recovery_actions_.end(),
              [](const RecoveryAction& a, const RecoveryAction& b) {
                  return a.priority > b.priority;
              });
}

bool ErrorHandler::attemptRecovery(const ErrorInfo& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& action : recovery_actions_) {
        recovery_attempt_count_++;
        int attempts = 0;
        
        while (attempts < action.max_attempts) {
            try {
                if (action.action()) {
                    LOG_INFO("Recovery action '" + action.name + "' succeeded",
                            "Error: " + error.message);
                    return true;
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Recovery action '" + action.name + "' failed: " + 
                         std::string(e.what()),
                         "Error: " + error.message);
            }
            
            attempts++;
            if (attempts < action.max_attempts) {
                std::this_thread::sleep_for(action.retry_interval);
            }
        }
    }
    
    return false;
}

void ErrorHandler::writeToLog(const ErrorInfo& error) {
    if (!log_file_.is_open()) return;
    
    std::stringstream ss;
    ss << formatTimestamp(error.timestamp) << " "
       << getSeverityString(error.severity) << " "
       << "[" << error.source_file << ":" << error.line_number << "] "
       << error.function_name << " - "
       << error.message;
    
    if (!error.context.empty()) {
        ss << " [Context: " << error.context << "]";
    }
    
    if (!error.stack_trace.empty()) {
        ss << "\nStack Trace:\n" << error.stack_trace;
    }
    
    ss << "\n\n";
    
    log_file_ << ss.str();
    log_file_.flush();
    
    // Check log size and rotate if needed
    if (log_file_.tellp() > max_log_size_) {
        rotateLogs();
    }
}

void ErrorHandler::rotateLogs() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
    
    // Remove oldest log if we've reached rotation count
    std::vector<std::filesystem::path> log_files;
    for (const auto& entry : std::filesystem::directory_iterator(log_directory_)) {
        if (entry.path().extension() == ".log") {
            log_files.push_back(entry.path());
        }
    }
    
    std::sort(log_files.begin(), log_files.end());
    while (log_files.size() >= log_rotation_count_) {
        std::filesystem::remove(log_files.front());
        log_files.erase(log_files.begin());
    }
    
    // Create new log file
    auto now = std::chrono::system_clock::now();
    std::string filename = log_directory_ + "/error_log_" + 
                          formatTimestamp(now) + ".log";
    log_file_.open(filename, std::ios::app);
}

std::string ErrorHandler::getStackTrace() {
    std::stringstream ss;
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    // Initialize symbol handler
    SymInitialize(process, NULL, TRUE);
    
    // Generate stack frames
    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
    
    // Setup stack frame
    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(STACKFRAME64));
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    
    // Walk the stack
    for (DWORD frame_number = 0; frame_number < 50; frame_number++) {
        if (!StackWalk64(
            IMAGE_FILE_MACHINE_AMD64,
            process,
            thread,
            &frame,
            &context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL)) {
            break;
        }
        
        // Get symbol info
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol)) {
            ss << "#" << frame_number << " " << symbol->Name;
            
            // Get line info
            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD displacement_line = 0;
            if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement_line, &line)) {
                ss << " at " << line.FileName << ":" << line.LineNumber;
            }
            ss << "\n";
        }
    }
    
    SymCleanup(process);
    return ss.str();
}

std::string ErrorHandler::formatTimestamp(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string ErrorHandler::getSeverityString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO: return "INFO";
        case ErrorSeverity::WARNING: return "WARNING";
        case ErrorSeverity::ERROR: return "ERROR";
        case ErrorSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void ErrorHandler::notifyCallbacks(const ErrorInfo& error) {
    for (const auto& callback : callbacks_) {
        try {
            callback(error);
        } catch (...) {
            // Ignore callback errors
        }
    }
}

void ErrorHandler::setErrorCallback(std::function<void(const ErrorInfo&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(callback);
}

void ErrorHandler::enableRecovery(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    recovery_enabled_ = enable;
}

void ErrorHandler::setMaxLogSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_log_size_ = max_size;
}

void ErrorHandler::setLogRotationCount(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_rotation_count_ = count;
}

void ErrorHandler::setLogDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_directory_ = directory;
    std::filesystem::create_directories(log_directory_);
}

std::vector<ErrorHandler::ErrorInfo> ErrorHandler::getRecentErrors(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t start = error_history_.size() > count ? 
                  error_history_.size() - count : 0;
    return std::vector<ErrorInfo>(error_history_.begin() + start, 
                                 error_history_.end());
}

std::vector<ErrorHandler::RecoveryAction> ErrorHandler::getFailedRecoveryActions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return recovery_actions_;
}

bool ErrorHandler::isRecoveryEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return recovery_enabled_;
}

size_t ErrorHandler::getErrorCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_count_;
}

size_t ErrorHandler::getRecoveryAttemptCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return recovery_attempt_count_;
} 