#include "InputValidator.hpp"
#include "StringUtils.hpp"

#include <algorithm>
#include <cctype>
#include <array>

namespace afs {

namespace {

// Reserved Windows device names that cannot be used as filenames
constexpr std::array<const char*, 22> kReservedNames = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
};

// Characters not allowed in filenames on various platforms
constexpr std::array<char, 9> kInvalidChars = {
    '<', '>', ':', '"', '/', '\\', '|', '?', '*'
};

// Maximum filename length - 255 is the limit for most filesystems:
// - ext4 (Linux): 255 bytes
// - NTFS (Windows): 255 characters (UTF-16 code units)
// - HFS+/APFS (macOS): 255 characters
// Some filesystems allow longer names, but 255 is a safe universal limit.
constexpr size_t kMaxFilenameLength = 255;

std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

bool is_whitespace_only(const std::string& str) {
    return std::all_of(str.begin(), str.end(), 
                       [](unsigned char c) { return std::isspace(c); });
}

} // anonymous namespace

Result<void> InputValidator::validate_directory_path(
    const std::string& path,
    bool must_exist,
    bool must_be_writable)
{
    // Check for empty path
    if (path.empty()) {
        return make_error(ErrorCode::EmptyInput, "Directory path cannot be empty");
    }

    // Check path length
    if (path.length() > kMaxPathLength) {
        return make_error(ErrorCode::InvalidPath, 
                         "Path exceeds maximum length",
                         "Maximum allowed: " + std::to_string(kMaxPathLength) + " characters");
    }

    try {
        std::filesystem::path fs_path(path);
        
        if (must_exist) {
            if (!std::filesystem::exists(fs_path)) {
                return make_error(ErrorCode::PathNotFound,
                                 "Directory does not exist",
                                 "Path: " + path);
            }

            if (!std::filesystem::is_directory(fs_path)) {
                return make_error(ErrorCode::InvalidPath,
                                 "Path is not a directory",
                                 "Path: " + path);
            }
        }

        if (must_be_writable && must_exist) {
            // Attempt to check write permissions
            auto perms = std::filesystem::status(fs_path).permissions();
            bool owner_write = (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
            if (!owner_write) {
                return make_error(ErrorCode::PermissionDenied,
                                 "No write permission for directory",
                                 "Path: " + path);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return make_error(ErrorCode::PathNotAccessible,
                         "Cannot access path",
                         e.what());
    }

    return ok();
}

Result<void> InputValidator::validate_file_path(
    const std::string& path,
    bool must_exist)
{
    if (path.empty()) {
        return make_error(ErrorCode::EmptyInput, "File path cannot be empty");
    }

    if (path.length() > kMaxPathLength) {
        return make_error(ErrorCode::InvalidPath,
                         "Path exceeds maximum length",
                         "Maximum allowed: " + std::to_string(kMaxPathLength) + " characters");
    }

    try {
        std::filesystem::path fs_path(path);

        if (must_exist) {
            if (!std::filesystem::exists(fs_path)) {
                return make_error(ErrorCode::FileNotFound,
                                 "File does not exist",
                                 "Path: " + path);
            }

            if (!std::filesystem::is_regular_file(fs_path)) {
                return make_error(ErrorCode::InvalidPath,
                                 "Path is not a regular file",
                                 "Path: " + path);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return make_error(ErrorCode::PathNotAccessible,
                         "Cannot access path",
                         e.what());
    }

    return ok();
}

Result<void> InputValidator::validate_api_key(
    const std::string& key,
    std::string_view provider)
{
    if (key.empty()) {
        return make_error(ErrorCode::EmptyInput,
                         std::string(provider) + " API key cannot be empty");
    }

    if (is_whitespace_only(key)) {
        return make_error(ErrorCode::InvalidApiKey,
                         std::string(provider) + " API key cannot be whitespace only");
    }

    if (key.length() < kMinApiKeyLength) {
        return make_error(ErrorCode::InvalidApiKey,
                         std::string(provider) + " API key appears invalid",
                         "Key length: " + std::to_string(key.length()) + 
                         ", minimum expected: " + std::to_string(kMinApiKeyLength));
    }

    // Check for obvious placeholder patterns
    std::string lower_key;
    lower_key.reserve(key.size());
    for (char c : key) {
        lower_key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    if (lower_key.find("your") != std::string::npos ||
        lower_key.find("api_key") != std::string::npos ||
        lower_key.find("apikey") != std::string::npos ||
        lower_key.find("placeholder") != std::string::npos ||
        lower_key.find("xxx") != std::string::npos) {
        return make_error(ErrorCode::InvalidApiKey,
                         "API key appears to be a placeholder",
                         "Please enter your actual " + std::string(provider) + " API key");
    }

    return ok();
}

Result<void> InputValidator::validate_category_label(
    const std::string& label,
    std::string_view field_name)
{
    if (label.empty()) {
        return make_error(ErrorCode::EmptyInput,
                         std::string(field_name) + " cannot be empty");
    }

    if (label.length() > kMaxLabelLength) {
        return make_error(ErrorCode::InvalidInput,
                         std::string(field_name) + " exceeds maximum length",
                         "Maximum allowed: " + std::to_string(kMaxLabelLength) + " characters");
    }

    // Check for leading/trailing whitespace
    if (std::isspace(static_cast<unsigned char>(label.front())) ||
        std::isspace(static_cast<unsigned char>(label.back()))) {
        return make_error(ErrorCode::InvalidInput,
                         std::string(field_name) + " has leading or trailing whitespace");
    }

    // Check for reserved names
    if (is_reserved_filename(label)) {
        return make_error(ErrorCode::InvalidInput,
                         std::string(field_name) + " uses a reserved name",
                         "Reserved names cannot be used as folder names on Windows");
    }

    // Check for invalid characters
    if (!contains_only_path_safe_chars(label)) {
        return make_error(ErrorCode::InvalidInput,
                         std::string(field_name) + " contains invalid characters",
                         "Characters < > : \" / \\ | ? * are not allowed");
    }

    return ok();
}

Result<void> InputValidator::validate_non_empty(
    const std::string& value,
    std::string_view field_name)
{
    if (value.empty()) {
        return make_error(ErrorCode::EmptyInput,
                         std::string(field_name) + " cannot be empty");
    }

    if (is_whitespace_only(value)) {
        return make_error(ErrorCode::EmptyInput,
                         std::string(field_name) + " cannot be whitespace only");
    }

    return ok();
}

Result<void> InputValidator::validate_model_name(const std::string& model)
{
    if (model.empty()) {
        return make_error(ErrorCode::EmptyInput, "Model name cannot be empty");
    }

    // Basic validation: model names typically contain letters, numbers, dashes, dots
    for (char c : model) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != '-' && c != '_' && c != '.' && c != '/') {
            return make_error(ErrorCode::InvalidInput,
                             "Model name contains invalid character",
                             "Character: " + std::string(1, c));
        }
    }

    return ok();
}

bool InputValidator::is_reserved_filename(const std::string& name)
{
    std::string upper_name = to_upper(name);
    
    // Also check with extension stripped (e.g., "CON.txt" is still reserved)
    std::string base_name = upper_name;
    size_t dot_pos = base_name.find('.');
    if (dot_pos != std::string::npos) {
        base_name = base_name.substr(0, dot_pos);
    }

    for (const char* reserved : kReservedNames) {
        if (upper_name == reserved || base_name == reserved) {
            return true;
        }
    }

    return false;
}

bool InputValidator::contains_only_path_safe_chars(const std::string& value)
{
    for (char c : value) {
        // Check against invalid characters
        for (char invalid : kInvalidChars) {
            if (c == invalid) {
                return false;
            }
        }
        // Also reject control characters
        if (static_cast<unsigned char>(c) < 32) {
            return false;
        }
    }
    return true;
}

std::string InputValidator::sanitize_filename(const std::string& name)
{
    if (name.empty()) {
        return "unnamed";
    }

    std::string result;
    result.reserve(name.size());

    for (char c : name) {
        bool is_invalid = false;
        for (char invalid : kInvalidChars) {
            if (c == invalid) {
                is_invalid = true;
                break;
            }
        }

        if (is_invalid || static_cast<unsigned char>(c) < 32) {
            result += '_';
        } else {
            result += c;
        }
    }

    // Trim leading/trailing spaces and dots
    size_t start = result.find_first_not_of(" .");
    size_t end = result.find_last_not_of(" .");
    
    if (start == std::string::npos) {
        return "unnamed";
    }

    result = result.substr(start, end - start + 1);

    // Handle reserved names
    if (is_reserved_filename(result)) {
        result = "_" + result;
    }

    // Truncate if too long for filesystem compatibility
    if (result.length() > kMaxFilenameLength) {
        result = result.substr(0, kMaxFilenameLength);
    }

    return result.empty() ? "unnamed" : result;
}

} // namespace afs
