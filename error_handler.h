#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <queue>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

class ErrorHandler {
public:
    enum class ErrorSeverity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    struct ErrorInfo {
        ErrorSeverity severity;
        std::string message;
        std::string context;
        std::string stack_trace;
        std::chrono::system_clock::time_point timestamp;
        std::string source_file;
        int line_number;
        std::string function_name;
    };

    struct RecoveryAction {
        std::string name;
        std::function<bool()> action;
        int priority;
        int max_attempts;
        std::chrono::milliseconds retry_interval;
    };

    static ErrorHandler& getInstance() {
        static ErrorHandler instance;
        return instance;
    }

    void logError(ErrorSeverity severity, 
                 const std::string& message,
                 const std::string& context = "",
                 const std::string& source_file = "",
                 int line_number = 0,
                 const std::string& function_name = "");

    void addRecoveryAction(const RecoveryAction& action);
    void setErrorCallback(std::function<void(const ErrorInfo&)> callback);
    void enableRecovery(bool enable);
    void setMaxLogSize(size_t max_size);
    void setLogRotationCount(size_t count);
    void setLogDirectory(const std::string& directory);

    std::vector<ErrorInfo> getRecentErrors(size_t count = 10) const;
    std::vector<RecoveryAction> getFailedRecoveryActions() const;
    bool isRecoveryEnabled() const;
    size_t getErrorCount() const;
    size_t getRecoveryAttemptCount() const;

private:
    ErrorHandler();
    ~ErrorHandler();
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    void writeToLog(const ErrorInfo& error);
    void notifyCallbacks(const ErrorInfo& error);
    bool attemptRecovery(const ErrorInfo& error);
    void rotateLogs();
    std::string getStackTrace();
    std::string formatTimestamp(const std::chrono::system_clock::time_point& time) const;
    std::string getSeverityString(ErrorSeverity severity) const;

    std::vector<ErrorInfo> error_history_;
    std::vector<RecoveryAction> recovery_actions_;
    std::vector<std::function<void(const ErrorInfo&)>> callbacks_;
    std::mutex mutex_;
    std::ofstream log_file_;
    std::string log_directory_;
    size_t max_log_size_;
    size_t log_rotation_count_;
    bool recovery_enabled_;
    size_t error_count_;
    size_t recovery_attempt_count_;
};

// Helper macros for easier error logging
#define LOG_INFO(message, context) \
    ErrorHandler::getInstance().logError(ErrorHandler::ErrorSeverity::INFO, message, context, __FILE__, __LINE__, __func__)

#define LOG_WARNING(message, context) \
    ErrorHandler::getInstance().logError(ErrorHandler::ErrorSeverity::WARNING, message, context, __FILE__, __LINE__, __func__)

#define LOG_ERROR(message, context) \
    ErrorHandler::getInstance().logError(ErrorHandler::ErrorSeverity::ERROR, message, context, __FILE__, __LINE__, __func__)

#define LOG_CRITICAL(message, context) \
    ErrorHandler::getInstance().logError(ErrorHandler::ErrorSeverity::CRITICAL, message, context, __FILE__, __LINE__, __func__)

#endif // ERROR_HANDLER_H 