#ifndef CONFIG_SCHEMA_HPP
#define CONFIG_SCHEMA_HPP

#include "Types.hpp"
#include "Language.hpp"
#include "CategoryLanguage.hpp"
#include "Result.hpp"

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace afs {

/**
 * @brief Type-safe configuration value with validation.
 * 
 * Wraps a configuration value with optional validation and default value.
 */
template<typename T>
class ConfigValue {
public:
    using Validator = std::function<Result<void>(const T&)>;

    ConfigValue() = default;
    
    explicit ConfigValue(T default_value, Validator validator = nullptr)
        : value_(std::move(default_value))
        , default_value_(value_)
        , validator_(std::move(validator))
    {}

    /**
     * @brief Gets the current value.
     */
    [[nodiscard]] const T& get() const { return value_; }
    [[nodiscard]] operator const T&() const { return value_; }

    /**
     * @brief Sets the value with validation.
     * @return Result indicating success or validation error
     */
    Result<void> set(T new_value) {
        if (validator_) {
            auto result = validator_(new_value);
            if (!result) {
                return result;
            }
        }
        value_ = std::move(new_value);
        modified_ = true;
        return ok();
    }

    /**
     * @brief Sets the value without validation (use with caution).
     */
    void set_unchecked(T new_value) {
        value_ = std::move(new_value);
        modified_ = true;
    }

    /**
     * @brief Resets to default value.
     */
    void reset() {
        value_ = default_value_;
        modified_ = false;
    }

    /**
     * @brief Gets the default value.
     */
    [[nodiscard]] const T& default_value() const { return default_value_; }

    /**
     * @brief Checks if the value has been modified from default.
     */
    [[nodiscard]] bool is_modified() const { return modified_; }

    /**
     * @brief Validates the current value.
     */
    [[nodiscard]] Result<void> validate() const {
        if (validator_) {
            return validator_(value_);
        }
        return ok();
    }

private:
    T value_{};
    T default_value_{};
    Validator validator_;
    bool modified_{false};
};

/**
 * @brief Configuration schema defining all application settings.
 * 
 * This provides a central location for all configuration values with
 * proper types, defaults, and validation. Use this instead of accessing
 * raw INI values directly.
 */
struct ConfigSchema {
    // LLM Configuration
    ConfigValue<LLMChoice> llm_choice{LLMChoice::Local_7b};
    ConfigValue<std::string> openai_api_key{""};
    ConfigValue<std::string> openai_model{"gpt-4o-mini"};
    ConfigValue<std::string> gemini_api_key{""};
    ConfigValue<std::string> gemini_model{"gemini-2.5-flash-lite"};
    ConfigValue<std::string> active_custom_llm_id{""};

    // Categorization Settings
    ConfigValue<bool> use_subcategories{true};
    ConfigValue<bool> use_consistency_hints{false};
    ConfigValue<bool> categorize_files{true};
    ConfigValue<bool> categorize_directories{false};
    ConfigValue<bool> consistency_pass_enabled{false};

    // Whitelist Settings
    ConfigValue<bool> use_whitelist{false};
    ConfigValue<std::string> active_whitelist{""};
    ConfigValue<std::vector<std::string>> allowed_categories{{}};
    ConfigValue<std::vector<std::string>> allowed_subcategories{{}};

    // UI Settings
    ConfigValue<bool> show_file_explorer{true};
    ConfigValue<Language> language{Language::English};
    ConfigValue<CategoryLanguage> category_language{CategoryLanguage::English};
    
    // Paths
    ConfigValue<std::string> sort_folder{""};

    // Development/Debug
    ConfigValue<bool> development_prompt_logging{false};

    // Usage Tracking
    ConfigValue<int> categorized_file_count{0};
    ConfigValue<int> next_support_prompt_threshold{200};

    // Version Management
    ConfigValue<std::string> skipped_version{""};

    /**
     * @brief Validates all configuration values.
     * @return Vector of errors if any validation fails
     */
    [[nodiscard]] std::vector<Error> validate_all() const {
        std::vector<Error> errors;
        
        auto check = [&errors](const auto& value, const char* name) {
            auto result = value.validate();
            if (!result) {
                Error err = result.error();
                err.details = std::string(name) + ": " + err.details;
                errors.push_back(err);
            }
        };

        check(llm_choice, "llm_choice");
        check(openai_api_key, "openai_api_key");
        check(openai_model, "openai_model");
        check(gemini_api_key, "gemini_api_key");
        check(gemini_model, "gemini_model");
        check(sort_folder, "sort_folder");

        return errors;
    }

    /**
     * @brief Resets all values to defaults.
     */
    void reset_all() {
        llm_choice.reset();
        openai_api_key.reset();
        openai_model.reset();
        gemini_api_key.reset();
        gemini_model.reset();
        active_custom_llm_id.reset();
        use_subcategories.reset();
        use_consistency_hints.reset();
        categorize_files.reset();
        categorize_directories.reset();
        consistency_pass_enabled.reset();
        use_whitelist.reset();
        active_whitelist.reset();
        allowed_categories.reset();
        allowed_subcategories.reset();
        show_file_explorer.reset();
        language.reset();
        category_language.reset();
        sort_folder.reset();
        development_prompt_logging.reset();
        categorized_file_count.reset();
        next_support_prompt_threshold.reset();
        skipped_version.reset();
    }
};

/**
 * @brief Factory for creating validators for common types.
 */
namespace validators {

inline ConfigValue<std::string>::Validator non_empty(const char* field_name) {
    return [field_name](const std::string& value) -> Result<void> {
        if (value.empty()) {
            return make_error(ErrorCode::EmptyInput, 
                            std::string(field_name) + " cannot be empty");
        }
        return ok();
    };
}

inline ConfigValue<int>::Validator min_value(int min, const char* field_name) {
    return [min, field_name](int value) -> Result<void> {
        if (value < min) {
            return make_error(ErrorCode::InvalidInput,
                            std::string(field_name) + " must be at least " + 
                            std::to_string(min));
        }
        return ok();
    };
}

inline ConfigValue<int>::Validator range(int min, int max, const char* field_name) {
    return [min, max, field_name](int value) -> Result<void> {
        if (value < min || value > max) {
            return make_error(ErrorCode::InvalidInput,
                            std::string(field_name) + " must be between " + 
                            std::to_string(min) + " and " + std::to_string(max));
        }
        return ok();
    };
}

} // namespace validators

} // namespace afs

#endif // CONFIG_SCHEMA_HPP
