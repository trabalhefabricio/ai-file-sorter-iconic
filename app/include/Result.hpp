#ifndef RESULT_HPP
#define RESULT_HPP

#include <optional>
#include <string>
#include <variant>
#include <utility>

namespace afs {

/**
 * @brief Error severity levels for categorizing issues.
 */
enum class ErrorSeverity {
    Info,       // Informational, operation may continue
    Warning,    // Non-critical issue, operation can continue with degraded behavior
    Error,      // Operation failed but system is stable
    Critical    // System stability may be affected
};

/**
 * @brief Error categories for grouping related error types.
 */
enum class ErrorCategory {
    None,           // No error
    Validation,     // Input validation errors
    FileSystem,     // File/directory access issues
    Network,        // Network connectivity issues
    Api,            // API-related errors (rate limits, auth, etc.)
    Database,       // Database operations errors
    Configuration,  // Configuration/settings errors
    LLM,            // LLM inference errors
    Internal        // Internal/programming errors
};

/**
 * @brief Detailed error codes for specific error conditions.
 */
enum class ErrorCode {
    // Success
    Ok = 0,

    // Validation errors (1xx)
    InvalidPath = 100,
    InvalidInput = 101,
    EmptyInput = 102,
    PathNotFound = 103,
    PathNotAccessible = 104,
    InvalidApiKey = 105,
    InvalidConfiguration = 106,

    // File system errors (2xx)
    FileNotFound = 200,
    DirectoryNotFound = 201,
    PermissionDenied = 202,
    DiskFull = 203,
    FileInUse = 204,
    InvalidFileName = 205,

    // Network errors (3xx)
    NetworkUnavailable = 300,
    ConnectionTimeout = 301,
    HostUnreachable = 302,
    SslError = 303,

    // API errors (4xx)
    ApiAuthFailed = 400,
    ApiRateLimited = 401,
    ApiServerError = 402,
    ApiInvalidRequest = 403,
    ApiResponseParseError = 404,
    ApiInvalidResponse = 405,
    ApiQuotaExceeded = 406,
    ApiModelNotAvailable = 407,

    // Database errors (5xx)
    DatabaseOpenFailed = 500,
    DatabaseQueryFailed = 501,
    DatabaseWriteFailed = 502,
    DatabaseCorrupted = 503,
    DatabaseLocked = 504,

    // LLM errors (6xx)
    LlmLoadFailed = 600,
    LlmInferenceFailed = 601,
    LlmModelNotFound = 602,
    LlmOutOfMemory = 603,
    LlmTimeout = 604,
    LlmInvalidOutput = 605,

    // Internal errors (9xx)
    InternalError = 900,
    NotImplemented = 901,
    InvalidState = 902,
    Cancelled = 903
};

/**
 * @brief Converts ErrorCode to its category.
 */
inline ErrorCategory get_error_category(ErrorCode code) {
    int value = static_cast<int>(code);
    if (value == 0) return ErrorCategory::None;
    if (value >= 100 && value < 200) return ErrorCategory::Validation;
    if (value >= 200 && value < 300) return ErrorCategory::FileSystem;
    if (value >= 300 && value < 400) return ErrorCategory::Network;
    if (value >= 400 && value < 500) return ErrorCategory::Api;
    if (value >= 500 && value < 600) return ErrorCategory::Database;
    if (value >= 600 && value < 700) return ErrorCategory::LLM;
    return ErrorCategory::Internal;
}

/**
 * @brief Human-readable name for error codes.
 */
inline const char* error_code_name(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok: return "OK";
        case ErrorCode::InvalidPath: return "InvalidPath";
        case ErrorCode::InvalidInput: return "InvalidInput";
        case ErrorCode::EmptyInput: return "EmptyInput";
        case ErrorCode::PathNotFound: return "PathNotFound";
        case ErrorCode::PathNotAccessible: return "PathNotAccessible";
        case ErrorCode::InvalidApiKey: return "InvalidApiKey";
        case ErrorCode::InvalidConfiguration: return "InvalidConfiguration";
        case ErrorCode::FileNotFound: return "FileNotFound";
        case ErrorCode::DirectoryNotFound: return "DirectoryNotFound";
        case ErrorCode::PermissionDenied: return "PermissionDenied";
        case ErrorCode::DiskFull: return "DiskFull";
        case ErrorCode::FileInUse: return "FileInUse";
        case ErrorCode::InvalidFileName: return "InvalidFileName";
        case ErrorCode::NetworkUnavailable: return "NetworkUnavailable";
        case ErrorCode::ConnectionTimeout: return "ConnectionTimeout";
        case ErrorCode::HostUnreachable: return "HostUnreachable";
        case ErrorCode::SslError: return "SslError";
        case ErrorCode::ApiAuthFailed: return "ApiAuthFailed";
        case ErrorCode::ApiRateLimited: return "ApiRateLimited";
        case ErrorCode::ApiServerError: return "ApiServerError";
        case ErrorCode::ApiInvalidRequest: return "ApiInvalidRequest";
        case ErrorCode::ApiResponseParseError: return "ApiResponseParseError";
        case ErrorCode::ApiInvalidResponse: return "ApiInvalidResponse";
        case ErrorCode::ApiQuotaExceeded: return "ApiQuotaExceeded";
        case ErrorCode::ApiModelNotAvailable: return "ApiModelNotAvailable";
        case ErrorCode::DatabaseOpenFailed: return "DatabaseOpenFailed";
        case ErrorCode::DatabaseQueryFailed: return "DatabaseQueryFailed";
        case ErrorCode::DatabaseWriteFailed: return "DatabaseWriteFailed";
        case ErrorCode::DatabaseCorrupted: return "DatabaseCorrupted";
        case ErrorCode::DatabaseLocked: return "DatabaseLocked";
        case ErrorCode::LlmLoadFailed: return "LlmLoadFailed";
        case ErrorCode::LlmInferenceFailed: return "LlmInferenceFailed";
        case ErrorCode::LlmModelNotFound: return "LlmModelNotFound";
        case ErrorCode::LlmOutOfMemory: return "LlmOutOfMemory";
        case ErrorCode::LlmTimeout: return "LlmTimeout";
        case ErrorCode::LlmInvalidOutput: return "LlmInvalidOutput";
        case ErrorCode::InternalError: return "InternalError";
        case ErrorCode::NotImplemented: return "NotImplemented";
        case ErrorCode::InvalidState: return "InvalidState";
        case ErrorCode::Cancelled: return "Cancelled";
        default: return "UnknownError";
    }
}

/**
 * @brief Structured error information with context.
 */
struct Error {
    ErrorCode code = ErrorCode::Ok;
    std::string message;
    std::string details;
    ErrorSeverity severity = ErrorSeverity::Error;

    Error() = default;
    
    explicit Error(ErrorCode c, std::string msg = "", std::string det = "", 
                   ErrorSeverity sev = ErrorSeverity::Error)
        : code(c), message(std::move(msg)), details(std::move(det)), severity(sev) {}

    [[nodiscard]] bool is_ok() const { return code == ErrorCode::Ok; }
    [[nodiscard]] bool is_error() const { return code != ErrorCode::Ok; }
    [[nodiscard]] ErrorCategory category() const { return get_error_category(code); }
    [[nodiscard]] const char* code_name() const { return error_code_name(code); }

    [[nodiscard]] std::string format() const {
        if (is_ok()) return "Success";
        std::string result = std::string(code_name()) + ": " + message;
        if (!details.empty()) {
            result += " (" + details + ")";
        }
        return result;
    }
};

/**
 * @brief Result type that holds either a value or an error.
 * 
 * Usage:
 *   Result<int> divide(int a, int b) {
 *       if (b == 0) return Error{ErrorCode::InvalidInput, "Division by zero"};
 *       return a / b;
 *   }
 *   
 *   auto result = divide(10, 2);
 *   if (result) {
 *       int value = result.value();  // 5
 *   } else {
 *       Error err = result.error();  // handle error
 *   }
 */
template<typename T>
class Result {
public:
    // Implicit conversion from value
    Result(T value) : data_(std::move(value)) {}

    // Implicit conversion from error
    Result(Error error) : data_(std::move(error)) {}

    // Check if result contains a value
    [[nodiscard]] bool has_value() const { return std::holds_alternative<T>(data_); }
    [[nodiscard]] bool is_ok() const { return has_value(); }
    [[nodiscard]] bool is_error() const { return !has_value(); }
    [[nodiscard]] explicit operator bool() const { return has_value(); }

    // Access the value (throws if error)
    [[nodiscard]] T& value() & {
        if (is_error()) throw std::runtime_error("Result contains error: " + error().format());
        return std::get<T>(data_);
    }

    [[nodiscard]] const T& value() const& {
        if (is_error()) throw std::runtime_error("Result contains error: " + error().format());
        return std::get<T>(data_);
    }

    [[nodiscard]] T&& value() && {
        if (is_error()) throw std::runtime_error("Result contains error: " + error().format());
        return std::get<T>(std::move(data_));
    }

    // Access the error (throws if value)
    [[nodiscard]] const Error& error() const {
        if (has_value()) throw std::runtime_error("Result contains value, not error");
        return std::get<Error>(data_);
    }

    // Get value or default
    [[nodiscard]] T value_or(T default_value) const& {
        return has_value() ? std::get<T>(data_) : std::move(default_value);
    }

    // Pointer-like access
    [[nodiscard]] T* operator->() { return &value(); }
    [[nodiscard]] const T* operator->() const { return &value(); }
    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }

    // Map the value with a function
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))> {
        if (is_error()) return error();
        return func(value());
    }

    // Chain operations
    template<typename F>
    auto and_then(F&& func) const -> decltype(func(std::declval<T>())) {
        if (is_error()) return error();
        return func(value());
    }

private:
    std::variant<T, Error> data_;
};

/**
 * @brief Specialization for void result (just success or error).
 */
template<>
class Result<void> {
public:
    Result() = default;
    Result(Error error) : error_(std::move(error)) {}

    [[nodiscard]] bool is_ok() const { return !error_.has_value(); }
    [[nodiscard]] bool is_error() const { return error_.has_value(); }
    [[nodiscard]] explicit operator bool() const { return is_ok(); }

    [[nodiscard]] const Error& error() const {
        if (is_ok()) throw std::runtime_error("Result is ok, no error");
        return *error_;
    }

    void value() const {
        if (is_error()) throw std::runtime_error("Result contains error: " + error().format());
    }

private:
    std::optional<Error> error_;
};

// Convenience factory functions
inline Error make_error(ErrorCode code, std::string message = "", std::string details = "") {
    return Error{code, std::move(message), std::move(details)};
}

inline Result<void> ok() { return Result<void>{}; }

template<typename T>
Result<T> ok(T value) { return Result<T>{std::move(value)}; }

} // namespace afs

#endif // RESULT_HPP
