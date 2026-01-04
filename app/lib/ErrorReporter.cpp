#include "ErrorReporter.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <QSysInfo>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <spdlog/sinks/basic_file_sink.h>
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/utsname.h>
#include <unistd.h>
#endif

// Static member initialization
std::string ErrorReporter::app_version_;
std::string ErrorReporter::log_directory_;
std::shared_ptr<spdlog::logger> ErrorReporter::error_logger_;
std::string ErrorReporter::last_error_id_;

void ErrorReporter::initialize(const std::string& app_version,
                               const std::string& log_directory) {
    app_version_ = app_version;
    log_directory_ = log_directory;
    
    // Create dedicated error log file
    std::string error_log_path = log_directory + "/errors.log";
    
    try {
        // Ensure directory exists
        Utils::ensure_directory_exists(log_directory);
        
        // Create rotating error logger (10MB per file, keep 5 files)
        error_logger_ = spdlog::rotating_logger_mt("error_reporter",
                                                    error_log_path,
                                                    1024 * 1024 * 10,
                                                    5);
        error_logger_->set_level(spdlog::level::trace);
        error_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        error_logger_->flush_on(spdlog::level::warn);
        
        // Log initialization
        error_logger_->info("=== ErrorReporter initialized ===");
        error_logger_->info("App version: {}", app_version);
        error_logger_->info("OS: {}", get_os_version());
        error_logger_->info("Qt compile: {}", QT_VERSION_STR);
        error_logger_->info("Qt runtime: {}", qVersion());
        
    } catch (const std::exception& e) {
        // Fallback to stderr if logger creation fails
        std::fprintf(stderr, "ERROR: Failed to initialize ErrorReporter: %s\n", e.what());
    }
}

std::string ErrorReporter::report_error(const ErrorContext& context) {
    std::string error_id = generate_error_id();
    last_error_id_ = error_id;
    
    // Log to both structured DB and human-readable log
    log_to_human_readable(context, error_id);
    log_to_structured_db(context, error_id);
    
    return error_id;
}

std::string ErrorReporter::report_quick(Category category,
                                       Severity severity,
                                       const std::string& error_code,
                                       const std::string& message,
                                       const char* source_file,
                                       int source_line,
                                       const char* function_name) {
    ErrorContext context;
    context.category = category;
    context.severity = severity;
    context.error_code = error_code;
    context.message = message;
    context.source_file = source_file ? source_file : "";
    context.source_line = source_line;
    context.function_name = function_name ? function_name : "";
    context.timestamp = std::chrono::system_clock::now();
    
    // Auto-fill system context
    context.os_version = get_os_version();
    context.qt_compile_version = QT_VERSION_STR;
    context.qt_runtime_version = qVersion();
    context.app_version = app_version_;
    context.system_path_dirs = get_system_path_preview();
    context.env_vars = get_relevant_env_vars();
    
    if (QCoreApplication::instance()) {
        context.working_directory = QDir::currentPath().toStdString();
        QStringList args = QCoreApplication::arguments();
        context.command_line_args.clear();
        for (const QString& arg : args) {
            context.command_line_args.push_back(arg.toStdString());
        }
    }
    
    // NEW: Generate code snippet for context
    if (!context.source_file.empty() && context.source_line > 0) {
        context.code_snippet = get_code_snippet(context.source_file, context.source_line, 5);
    }
    
    std::string error_id = report_error(context);
    
    // NEW: Generate and log Copilot-friendly message
    std::string copilot_msg = generate_copilot_message(context, error_id);
    context.copilot_prompt = copilot_msg;
    
    // Save Copilot message to a separate file for easy access
    std::string copilot_file = log_directory_ + "/COPILOT_ERROR_" + error_id + ".md";
    std::ofstream copilot_out(copilot_file);
    if (copilot_out.is_open()) {
        copilot_out << copilot_msg;
        copilot_out.close();
        
        if (error_logger_) {
            error_logger_->info("Copilot-friendly error message saved to: {}", copilot_file);
            error_logger_->info("Copy this file's contents and paste into GitHub Copilot Chat for assistance");
        }
    }
    
    return error_id;
}

void ErrorReporter::add_context(const std::string& key, const std::string& value) {
    if (error_logger_ && !last_error_id_.empty()) {
        error_logger_->info("  [{}] Additional context: {} = {}", last_error_id_, key, value);
    }
}

std::string ErrorReporter::generate_error_id() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::ostringstream oss;
    oss << "ERR-" << ms.count() << "-" << dis(gen);
    return oss.str();
}

std::string ErrorReporter::get_os_version() {
#ifdef _WIN32
    QString osVersion = QSysInfo::productType() + " " + QSysInfo::productVersion();
    return osVersion.toStdString();
#else
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        return std::string(buffer.sysname) + " " + buffer.release;
    }
    return "Unknown Unix";
#endif
}

std::string ErrorReporter::get_system_path_preview() {
#ifdef _WIN32
    wchar_t pathBuffer[32768];
    DWORD pathSize = GetEnvironmentVariableW(L"PATH", pathBuffer, 32768);
    if (pathSize > 0 && pathSize < 32768) {
        QString pathStr = QString::fromWCharArray(pathBuffer);
        QStringList paths = pathStr.split(';', Qt::SkipEmptyParts);
        // Return first 5 directories
        int count = std::min(5, static_cast<int>(paths.size()));
        QStringList preview = paths.mid(0, count);
        return preview.join("; ").toStdString();
    }
#else
    const char* path = std::getenv("PATH");
    if (path) {
        QString pathStr = QString::fromUtf8(path);
        QStringList paths = pathStr.split(':', Qt::SkipEmptyParts);
        int count = std::min(5, static_cast<int>(paths.size()));
        QStringList preview = paths.mid(0, count);
        return preview.join(":").toStdString();
    }
#endif
    return "PATH not available";
}

std::map<std::string, std::string> ErrorReporter::get_relevant_env_vars() {
    std::map<std::string, std::string> vars;
    
    const char* important_vars[] = {
        "QT_PLUGIN_PATH",
        "QT_QPA_PLATFORM_PLUGIN_PATH",
        "HOME",
        "APPDATA",
        "TEMP",
        "TMP",
        "CUDA_PATH",
        "VULKAN_SDK"
    };
    
    for (const char* var_name : important_vars) {
        const char* value = std::getenv(var_name);
        if (value) {
            vars[var_name] = value;
        }
    }
    
    return vars;
}

std::string ErrorReporter::category_to_string(Category category) {
    switch (category) {
        case Category::DLL_LOADING: return "DLL_LOADING";
        case Category::QT_INITIALIZATION: return "QT_INITIALIZATION";
        case Category::STARTUP: return "STARTUP";
        case Category::RUNTIME: return "RUNTIME";
        case Category::FILESYSTEM: return "FILESYSTEM";
        case Category::NETWORK: return "NETWORK";
        case Category::DATABASE: return "DATABASE";
        case Category::MEMORY: return "MEMORY";
        case Category::CONFIGURATION: return "CONFIGURATION";
        case Category::USER_ACTION: return "USER_ACTION";
        case Category::UNKNOWN: default: return "UNKNOWN";
    }
}

std::string ErrorReporter::severity_to_string(Severity severity) {
    switch (severity) {
        case Severity::CRITICAL: return "CRITICAL";
        case Severity::ERROR_HIGH: return "ERROR_HIGH";
        case Severity::ERROR_MEDIUM: return "ERROR_MEDIUM";
        case Severity::WARNING: return "WARNING";
        case Severity::INFO: default: return "INFO";
    }
}

void ErrorReporter::log_to_human_readable(const ErrorContext& context, const std::string& error_id) {
    if (!error_logger_) return;
    
    auto level = (context.severity == Severity::CRITICAL || context.severity == Severity::ERROR_HIGH) 
                 ? spdlog::level::critical : spdlog::level::err;
    
    error_logger_->log(level, "");
    error_logger_->log(level, "========================================");
    error_logger_->log(level, "Error ID: {}", error_id);
    error_logger_->log(level, "Category: {}", category_to_string(context.category));
    error_logger_->log(level, "Severity: {}", severity_to_string(context.severity));
    error_logger_->log(level, "Code: {}", context.error_code);
    error_logger_->log(level, "Message: {}", context.message);
    error_logger_->log(level, "Location: {}:{} in {}", context.source_file, context.source_line, context.function_name);
    error_logger_->log(level, "");
    error_logger_->log(level, "System Context:");
    error_logger_->log(level, "  OS: {}", context.os_version);
    error_logger_->log(level, "  App Version: {}", context.app_version);
    error_logger_->log(level, "  Qt Compile: {}", context.qt_compile_version);
    error_logger_->log(level, "  Qt Runtime: {}", context.qt_runtime_version);
    
    if (!context.dll_name.empty()) {
        error_logger_->log(level, "");
        error_logger_->log(level, "DLL Context:");
        error_logger_->log(level, "  DLL Name: {}", context.dll_name);
        error_logger_->log(level, "  DLL Path: {}", context.dll_path);
        error_logger_->log(level, "  DLL Version: {}", context.dll_version);
        error_logger_->log(level, "  Missing Symbol: {}", context.missing_symbol);
    }
    
    if (!context.system_path_dirs.empty()) {
        error_logger_->log(level, "  System PATH (first 5): {}", context.system_path_dirs);
    }
    
    if (!context.working_directory.empty()) {
        error_logger_->log(level, "  Working Dir: {}", context.working_directory);
    }
    
    if (!context.env_vars.empty()) {
        error_logger_->log(level, "");
        error_logger_->log(level, "Environment Variables:");
        for (const auto& [key, value] : context.env_vars) {
            error_logger_->log(level, "  {}: {}", key, value);
        }
    }
    
    if (!context.extra_data.empty()) {
        error_logger_->log(level, "");
        error_logger_->log(level, "Extra Context:");
        for (const auto& [key, value] : context.extra_data) {
            error_logger_->log(level, "  {}: {}", key, value);
        }
    }
    
    error_logger_->log(level, "========================================");
    error_logger_->log(level, "");
}

void ErrorReporter::log_to_structured_db(const ErrorContext& context, const std::string& error_id) {
    // Create errors.jsonl file (JSON Lines format - one JSON object per line)
    // This format is easy to parse and analyze with scripts/tools
    std::string db_path = log_directory_ + "/errors.jsonl";
    
    try {
        QFile file(QString::fromStdString(db_path));
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            return;
        }
        
        QJsonObject json;
        json["error_id"] = QString::fromStdString(error_id);
        json["category"] = QString::fromStdString(category_to_string(context.category));
        json["severity"] = QString::fromStdString(severity_to_string(context.severity));
        json["error_code"] = QString::fromStdString(context.error_code);
        json["message"] = QString::fromStdString(context.message);
        json["source_file"] = QString::fromStdString(context.source_file);
        json["source_line"] = context.source_line;
        json["function_name"] = QString::fromStdString(context.function_name);
        
        // System context
        QJsonObject system_ctx;
        system_ctx["os_version"] = QString::fromStdString(context.os_version);
        system_ctx["app_version"] = QString::fromStdString(context.app_version);
        system_ctx["qt_compile_version"] = QString::fromStdString(context.qt_compile_version);
        system_ctx["qt_runtime_version"] = QString::fromStdString(context.qt_runtime_version);
        system_ctx["working_directory"] = QString::fromStdString(context.working_directory);
        
        // Join command line args into a single string
        std::string joined_args;
        for (size_t i = 0; i < context.command_line_args.size(); ++i) {
            if (i > 0) joined_args += " ";
            joined_args += context.command_line_args[i];
        }
        system_ctx["command_line_args"] = QString::fromStdString(joined_args);
        
        system_ctx["system_path_preview"] = QString::fromStdString(context.system_path_dirs);
        json["system_context"] = system_ctx;
        
        // DLL context (if applicable)
        if (!context.dll_name.empty()) {
            QJsonObject dll_ctx;
            dll_ctx["dll_name"] = QString::fromStdString(context.dll_name);
            dll_ctx["dll_path"] = QString::fromStdString(context.dll_path);
            dll_ctx["dll_version"] = QString::fromStdString(context.dll_version);
            dll_ctx["missing_symbol"] = QString::fromStdString(context.missing_symbol);
            json["dll_context"] = dll_ctx;
        }
        
        // Environment variables
        if (!context.env_vars.empty()) {
            QJsonObject env_obj;
            for (const auto& [key, value] : context.env_vars) {
                env_obj[QString::fromStdString(key)] = QString::fromStdString(value);
            }
            json["env_vars"] = env_obj;
        }
        
        // Extra data
        if (!context.extra_data.empty()) {
            QJsonObject extra_obj;
            for (const auto& [key, value] : context.extra_data) {
                extra_obj[QString::fromStdString(key)] = QString::fromStdString(value);
            }
            json["extra_data"] = extra_obj;
        }
        
        // Timestamp
        auto time_point = context.timestamp;
        auto time_t_val = std::chrono::system_clock::to_time_t(time_point);
        json["timestamp"] = QDateTime::fromSecsSinceEpoch(time_t_val).toString(Qt::ISODate);
        
        // Write as single-line JSON
        QJsonDocument doc(json);
        QTextStream stream(&file);
        stream << doc.toJson(QJsonDocument::Compact) << "\n";
        file.close();
        
    } catch (const std::exception& e) {
        if (error_logger_) {
            error_logger_->error("Failed to write to structured error DB: {}", e.what());
        }
    }
}

std::map<std::string, int> ErrorReporter::get_error_frequencies() {
    std::map<std::string, int> frequencies;
    std::string db_path = log_directory_ + "/errors.jsonl";
    
    QFile file(QString::fromStdString(db_path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return frequencies;
    }
    
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString error_code = obj["error_code"].toString();
            if (!error_code.isEmpty()) {
                frequencies[error_code.toStdString()]++;
            }
        }
    }
    
    file.close();
    return frequencies;
}

bool ErrorReporter::export_to_json(const std::string& output_path) {
    std::string db_path = log_directory_ + "/errors.jsonl";
    
    QFile input_file(QString::fromStdString(db_path));
    if (!input_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QJsonArray errors;
    QTextStream stream(&input_file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isObject()) {
            errors.append(doc.object());
        }
    }
    input_file.close();
    
    // Create summary object
    QJsonObject summary;
    summary["total_errors"] = errors.size();
    summary["export_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    summary["app_version"] = QString::fromStdString(app_version_);
    
    QJsonObject root;
    root["summary"] = summary;
    root["errors"] = errors;
    
    // Write formatted JSON
    QFile output_file(QString::fromStdString(output_path));
    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QJsonDocument doc(root);
    output_file.write(doc.toJson(QJsonDocument::Indented));
    output_file.close();
    
    return true;
}

std::string ErrorReporter::get_error_db_path() {
    return log_directory_ + "/errors.jsonl";
}

std::string ErrorReporter::get_code_snippet(const std::string& file_path, int line_number, int context_lines) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return ""; // File cannot be read
    }
    
    std::ostringstream snippet;
    std::string line;
    int current_line = 1;
    int start_line = std::max(1, line_number - context_lines);
    int end_line = line_number + context_lines;
    
    while (std::getline(file, line) && current_line <= end_line) {
        if (current_line >= start_line) {
            // Highlight the error line
            if (current_line == line_number) {
                snippet << " >>> " << current_line << " | " << line << "\n";
            } else {
                snippet << "     " << current_line << " | " << line << "\n";
            }
        }
        current_line++;
    }
    
    file.close();
    return snippet.str();
}

std::string ErrorReporter::generate_troubleshooting_steps(const ErrorContext& context) {
    std::ostringstream steps;
    
    steps << "## Troubleshooting Steps\n\n";
    
    // Category-specific steps
    switch (context.category) {
        case Category::DLL_LOADING:
            steps << "1. **Check Qt Installation**\n";
            steps << "   - Verify you have Qt 6.5.3 or compatible version installed\n";
            steps << "   - Remove other Qt versions from system PATH\n\n";
            steps << "2. **Run as Administrator**\n";
            steps << "   - Right-click StartAiFileSorter.exe → Run as administrator\n";
            steps << "   - This allows DLL path manipulation to work properly\n\n";
            steps << "3. **Check System PATH**\n";
            steps << "   - Open System Properties → Environment Variables\n";
            steps << "   - Look for conflicting Qt installations in PATH\n";
            steps << "   - Remove or reorder so application directory comes first\n\n";
            break;
            
        case Category::QT_INITIALIZATION:
            steps << "1. **Verify Qt DLLs**\n";
            steps << "   - Check that Qt6Core.dll, Qt6Widgets.dll are in app directory\n";
            steps << "   - Reinstall application if DLLs are missing\n\n";
            steps << "2. **Check Plugins**\n";
            steps << "   - Verify plugins/ folder exists in application directory\n";
            steps << "   - Contains qwindows.dll platform plugin\n\n";
            break;
            
        case Category::STARTUP:
            steps << "1. **Check Logs**\n";
            steps << "   - Open logs directory: " << log_directory_ << "\n";
            steps << "   - Review core.log, errors.log for details\n\n";
            steps << "2. **Fresh Start**\n";
            steps << "   - Delete config.ini and restart application\n";
            steps << "   - Reinstall if problem persists\n\n";
            break;
            
        default:
            steps << "1. **Review Error Details Above**\n";
            steps << "   - Copy the error message and context\n";
            steps << "   - Share with GitHub Copilot for specific guidance\n\n";
            break;
    }
    
    steps << "## What to Do Next\n\n";
    steps << "1. **Try the steps above** in order\n";
    steps << "2. **If issue persists**, copy this entire error message\n";
    steps << "3. **Paste into GitHub Copilot Chat** and ask:\n";
    steps << "   \"How do I fix this error? I followed the troubleshooting steps but still have the issue.\"\n\n";
    
    return steps.str();
}

std::string ErrorReporter::generate_copilot_message(const ErrorContext& context, const std::string& error_id) {
    std::ostringstream msg;
    
    msg << "# Error Report for GitHub Copilot\n\n";
    msg << "*Copy this entire message and paste into GitHub Copilot Chat for assistance*\n\n";
    msg << "---\n\n";
    
    msg << "## Error Summary\n\n";
    msg << "**Error ID:** `" << error_id << "`\n";
    msg << "**Category:** " << category_to_string(context.category) << "\n";
    msg << "**Severity:** " << severity_to_string(context.severity) << "\n";
    msg << "**Error Code:** `" << context.error_code << "`\n";
    msg << "**Message:** " << context.message << "\n\n";
    
    msg << "## Where the Error Occurred\n\n";
    msg << "**File:** `" << context.source_file << "`\n";
    msg << "**Line:** " << context.source_line << "\n";
    msg << "**Function:** `" << context.function_name << "()`\n\n";
    
    // Add code snippet if available
    if (!context.code_snippet.empty()) {
        msg << "**Code Context:**\n```cpp\n";
        msg << context.code_snippet;
        msg << "```\n\n";
    }
    
    msg << "## System Information\n\n";
    msg << "- **OS:** " << context.os_version << "\n";
    msg << "- **App Version:** " << context.app_version << "\n";
    msg << "- **Qt Compile Version:** " << context.qt_compile_version << "\n";
    msg << "- **Qt Runtime Version:** " << context.qt_runtime_version << "\n";
    msg << "- **Working Directory:** `" << context.working_directory << "`\n\n";
    
    // DLL-specific info
    if (!context.dll_name.empty()) {
        msg << "## DLL Information\n\n";
        msg << "- **DLL Name:** `" << context.dll_name << "`\n";
        if (!context.dll_path.empty()) {
            msg << "- **DLL Path:** `" << context.dll_path << "`\n";
        }
        if (!context.missing_symbol.empty()) {
            msg << "- **Missing Symbol:** `" << context.missing_symbol << "`\n";
        }
        msg << "\n";
    }
    
    // PATH information
    if (!context.system_path_dirs.empty()) {
        msg << "## System PATH\n\n";
        msg << "```\n" << context.system_path_dirs << "\n```\n\n";
    }
    
    // Extra context
    if (!context.extra_data.empty()) {
        msg << "## Additional Context\n\n";
        for (const auto& [key, value] : context.extra_data) {
            msg << "- **" << key << ":** " << value << "\n";
        }
        msg << "\n";
    }
    
    // Add troubleshooting steps
    msg << generate_troubleshooting_steps(context);
    
    msg << "---\n\n";
    msg << "## Question for Copilot\n\n";
    msg << "Based on this error, what is the root cause and how can I fix it?\n";
    msg << "I'm using AI File Sorter on Windows and I'm comfortable following step-by-step instructions.\n";
    
    return msg.str();
}
