#ifndef ERROR_REPORTER_HPP
#define ERROR_REPORTER_HPP

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>

/**
 * @brief Structured error reporting system for better diagnostics and smarter fixes
 * 
 * This class provides a comprehensive error tracking system that captures:
 * - Error category and severity
 * - System context (OS, Qt version, DLL info)
 * - Stack trace and error location
 * - User environment details
 * - Frequency and patterns
 * 
 * Errors are logged in a structured format to both:
 * 1. Regular log files (human-readable)
 * 2. JSON error database (machine-parseable for analysis)
 */
class ErrorReporter {
public:
    enum class Category {
        DLL_LOADING,           // DLL path, version mismatch, loading failures
        QT_INITIALIZATION,     // Qt application startup, widget creation
        STARTUP,               // General startup failures
        RUNTIME,               // Runtime errors after successful startup
        FILESYSTEM,            // File/directory access issues
        NETWORK,               // API calls, download failures
        DATABASE,              // SQLite errors
        MEMORY,                // Allocation failures
        CONFIGURATION,         // Config file, settings issues
        USER_ACTION,           // User-triggered errors (UI interactions)
        UNKNOWN                // Uncategorized errors
    };

    enum class Severity {
        CRITICAL,      // Application cannot continue
        ERROR_HIGH,    // Major functionality broken
        ERROR_MEDIUM,  // Minor functionality broken
        WARNING,       // Potential issue, application continues
        INFO           // Informational, not an error
    };

    struct ErrorContext {
        // Core error info
        Category category;
        Severity severity;
        std::string error_code;      // e.g., "DLL_DROPEVENT_NOT_FOUND"
        std::string message;         // Human-readable message
        std::string source_file;     // __FILE__
        int source_line;             // __LINE__
        std::string function_name;   // __FUNCTION__
        
        // NEW: Code context for non-developers
        std::string stack_trace;     // Stack trace if available
        std::string code_snippet;    // Code around error location
        std::string copilot_prompt;  // Formatted message for GitHub Copilot
        
        // System context
        std::string os_version;      // Windows 10, Ubuntu 22.04, etc.
        std::string qt_compile_version;
        std::string qt_runtime_version;
        std::string app_version;
        
        // DLL-specific context (for DLL_LOADING category)
        std::string dll_name;
        std::string dll_path;
        std::string dll_version;
        std::string missing_symbol;
        std::string system_path_dirs; // First 5 PATH directories
        
        // Environment context
        std::map<std::string, std::string> env_vars; // Selected environment variables
        std::string working_directory;
        std::vector<std::string> command_line_args;
        
        // Additional context (flexible key-value pairs)
        std::map<std::string, std::string> extra_data;
        
        // Timestamp
        std::chrono::system_clock::time_point timestamp;
    };

    /**
     * @brief Initialize the error reporter with application context
     */
    static void initialize(const std::string& app_version,
                          const std::string& log_directory);

    /**
     * @brief Report a structured error with full context
     * 
     * @param context Complete error context with all available information
     * @return Unique error ID for tracking
     */
    static std::string report_error(const ErrorContext& context);

    /**
     * @brief Quick error reporting with minimal context (auto-filled)
     * 
     * @param category Error category
     * @param severity Error severity  
     * @param error_code Short error code
     * @param message Human-readable message
     * @param source_file Source file (__FILE__)
     * @param source_line Source line (__LINE__)
     * @param function_name Function name (__FUNCTION__)
     * @return Unique error ID for tracking
     */
    static std::string report_quick(Category category,
                                   Severity severity,
                                   const std::string& error_code,
                                   const std::string& message,
                                   const char* source_file,
                                   int source_line,
                                   const char* function_name);

    /**
     * @brief Add extra context to the last reported error
     */
    static void add_context(const std::string& key, const std::string& value);

    /**
     * @brief Get error statistics for analysis
     */
    static std::map<std::string, int> get_error_frequencies();

    /**
     * @brief Export errors to JSON for analysis
     */
    static bool export_to_json(const std::string& output_path);

    /**
     * @brief Get the error database file path
     */
    static std::string get_error_db_path();
    
    /**
     * @brief Generate a Copilot-friendly error message for the last error
     * 
     * Creates a formatted markdown message that can be copied and pasted
     * directly into GitHub Copilot chat for assistance.
     * 
     * @return Markdown-formatted string ready for Copilot
     */
    static std::string generate_copilot_message(const ErrorContext& context, const std::string& error_id);
    
    /**
     * @brief Get code snippet around the error location
     * 
     * Reads the source file and extracts lines around the error location
     * for context. Returns empty string if file cannot be read.
     * 
     * @param file_path Path to source file
     * @param line_number Line number where error occurred
     * @param context_lines Number of lines before/after to include (default: 5)
     * @return Code snippet with line numbers
     */
    static std::string get_code_snippet(const std::string& file_path, int line_number, int context_lines = 5);
    
    /**
     * @brief Get the path to the last generated error report file
     * 
     * Returns the path to the most recently generated COPILOT_ERROR_*.md file.
     * This can be used to direct users to the error report for troubleshooting.
     * 
     * @return Path to the last error report file, or empty string if none exists
     */
    static std::string get_last_error_report_path();

private:
    static std::string app_version_;
    static std::string log_directory_;
    static std::shared_ptr<spdlog::logger> error_logger_;
    static std::string last_error_id_;
    static std::string last_error_report_path_;  // Track the last error report file path

    static std::string generate_error_id();
    static std::string get_os_version();
    static std::string get_system_path_preview();
    static std::map<std::string, std::string> get_relevant_env_vars();
    static std::string category_to_string(Category category);
    static std::string severity_to_string(Severity severity);
    static void log_to_structured_db(const ErrorContext& context, const std::string& error_id);
    static void log_to_human_readable(const ErrorContext& context, const std::string& error_id);
    static std::string generate_troubleshooting_steps(const ErrorContext& context);

    ErrorReporter() = delete;
};

// Convenience macros for easy error reporting
#define REPORT_ERROR(category, severity, code, message) \
    ErrorReporter::report_quick(category, severity, code, message, __FILE__, __LINE__, __FUNCTION__)

#define REPORT_DLL_ERROR(code, message, dll_name, missing_symbol) \
    do { \
        auto error_id = REPORT_ERROR(ErrorReporter::Category::DLL_LOADING, \
                                     ErrorReporter::Severity::CRITICAL, \
                                     code, message); \
        ErrorReporter::add_context("dll_name", dll_name); \
        ErrorReporter::add_context("missing_symbol", missing_symbol); \
    } while(0)

#define REPORT_QT_ERROR(code, message) \
    REPORT_ERROR(ErrorReporter::Category::QT_INITIALIZATION, \
                ErrorReporter::Severity::ERROR_HIGH, code, message)

#define REPORT_STARTUP_ERROR(code, message) \
    REPORT_ERROR(ErrorReporter::Category::STARTUP, \
                ErrorReporter::Severity::CRITICAL, code, message)

#endif // ERROR_REPORTER_HPP
