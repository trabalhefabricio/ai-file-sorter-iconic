#ifndef INPUT_VALIDATOR_HPP
#define INPUT_VALIDATOR_HPP

#include "Result.hpp"

#include <filesystem>
#include <regex>
#include <string>
#include <string_view>

namespace afs {

/**
 * @brief Centralized input validation utilities.
 * 
 * Provides consistent validation across the application with clear error messages.
 */
class InputValidator {
public:
    /**
     * @brief Validates a directory path for use in file operations.
     * @param path The path to validate
     * @param must_exist If true, path must exist
     * @param must_be_writable If true, checks write permissions
     * @return Result<void> with error details if validation fails
     */
    static Result<void> validate_directory_path(
        const std::string& path,
        bool must_exist = true,
        bool must_be_writable = false);

    /**
     * @brief Validates a file path for use in file operations.
     * @param path The path to validate
     * @param must_exist If true, file must exist
     * @return Result<void> with error details if validation fails
     */
    static Result<void> validate_file_path(
        const std::string& path,
        bool must_exist = true);

    /**
     * @brief Validates an API key format.
     * @param key The API key to validate
     * @param provider The provider name for error messages ("OpenAI", "Gemini")
     * @return Result<void> with error details if validation fails
     */
    static Result<void> validate_api_key(
        const std::string& key,
        std::string_view provider = "API");

    /**
     * @brief Validates a category/subcategory label.
     * @param label The label to validate
     * @param field_name The field name for error messages
     * @return Result<void> with error details if validation fails
     */
    static Result<void> validate_category_label(
        const std::string& label,
        std::string_view field_name = "category");

    /**
     * @brief Validates that a string is not empty or whitespace-only.
     * @param value The string to validate
     * @param field_name The field name for error messages
     * @return Result<void> with error details if validation fails
     */
    static Result<void> validate_non_empty(
        const std::string& value,
        std::string_view field_name);

    /**
     * @brief Validates a model name/identifier.
     * @param model The model name to validate
     * @return Result<void> with error details if validation fails
     */
    static Result<void> validate_model_name(const std::string& model);

    /**
     * @brief Checks if a filename contains reserved Windows names.
     * @param name The filename to check
     * @return true if the name is a reserved name
     */
    static bool is_reserved_filename(const std::string& name);

    /**
     * @brief Checks if a string contains only allowed characters for paths.
     * @param value The string to check
     * @return true if string contains only allowed characters
     */
    static bool contains_only_path_safe_chars(const std::string& value);

    /**
     * @brief Sanitizes a string for use as a directory/file name.
     * @param name The name to sanitize
     * @return Sanitized name safe for filesystem use
     */
    static std::string sanitize_filename(const std::string& name);

private:
    static constexpr size_t kMaxLabelLength = 80;
    static constexpr size_t kMaxPathLength = 4096;
    static constexpr size_t kMinApiKeyLength = 20;
};

} // namespace afs

#endif // INPUT_VALIDATOR_HPP
