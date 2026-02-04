#include "LLMService.hpp"
#include "Settings.hpp"
#include "LLMClient.hpp"
#include "LocalLLMClient.hpp"
#include "GeminiClient.hpp"
#include "InputValidator.hpp"
#include "StringUtils.hpp"

#include <spdlog/spdlog.h>
#include <chrono>

namespace afs {

namespace {

std::string llm_choice_to_string(LLMChoice choice) {
    switch (choice) {
        case LLMChoice::Local_3b: return "Local LLM (3B)";
        case LLMChoice::Local_7b: return "Local LLM (7B)";
        case LLMChoice::Remote_OpenAI: return "OpenAI";
        case LLMChoice::Remote_Gemini: return "Gemini";
        case LLMChoice::Custom: return "Custom LLM";
        default: return "Unknown";
    }
}

Result<std::unique_ptr<ILLMClient>> create_legacy_client(
    const LLMConfig& config,
    std::shared_ptr<spdlog::logger> logger)
{
    switch (config.choice) {
        case LLMChoice::Remote_OpenAI: {
            auto validation = InputValidator::validate_api_key(config.api_key, "OpenAI");
            if (!validation) {
                return validation.error();
            }
            return std::make_unique<LLMClient>(config.api_key, config.model_name);
        }
        
        case LLMChoice::Remote_Gemini: {
            auto validation = InputValidator::validate_api_key(config.api_key, "Gemini");
            if (!validation) {
                return validation.error();
            }
            return std::make_unique<GeminiClient>(config.api_key, config.model_name);
        }
        
        case LLMChoice::Local_3b:
        case LLMChoice::Local_7b: {
            // LocalLLMClient handles model selection internally
            return std::make_unique<LocalLLMClient>();
        }
        
        case LLMChoice::Custom: {
            if (config.custom_llm_path.empty()) {
                return make_error(ErrorCode::InvalidConfiguration,
                                 "Custom LLM path not specified");
            }
            auto validation = InputValidator::validate_file_path(config.custom_llm_path);
            if (!validation) {
                return validation.error();
            }
            return std::make_unique<LocalLLMClient>(config.custom_llm_path);
        }
        
        case LLMChoice::Unset:
        default:
            return make_error(ErrorCode::InvalidConfiguration,
                             "LLM choice not configured",
                             "Please select an LLM in Settings");
    }
}

} // anonymous namespace

Result<std::unique_ptr<LLMService>> LLMService::create(
    const LLMConfig& config,
    std::shared_ptr<spdlog::logger> logger)
{
    auto client_result = create_legacy_client(config, logger);
    if (!client_result) {
        return client_result.error();
    }

    return std::make_unique<LegacyLLMAdapter>(
        std::move(client_result).value(),
        config,
        logger);
}

Result<std::unique_ptr<LLMService>> LLMService::create_from_settings(
    const Settings& settings,
    std::shared_ptr<spdlog::logger> logger)
{
    LLMConfig config;
    config.choice = settings.get_llm_choice();
    config.enable_prompt_logging = settings.get_development_prompt_logging();

    switch (config.choice) {
        case LLMChoice::Remote_OpenAI:
            config.api_key = settings.get_openai_api_key();
            config.model_name = settings.get_openai_model();
            break;
        
        case LLMChoice::Remote_Gemini:
            config.api_key = settings.get_gemini_api_key();
            config.model_name = settings.get_gemini_model();
            break;
        
        case LLMChoice::Custom: {
            config.custom_llm_id = settings.get_active_custom_llm_id();
            auto custom_llm = settings.find_custom_llm(config.custom_llm_id);
            if (is_valid_custom_llm(custom_llm)) {
                config.custom_llm_path = custom_llm.path;
                config.model_name = custom_llm.name;
            }
            break;
        }
        
        default:
            // Local models don't need additional config
            break;
    }

    return create(config, logger);
}

// LegacyLLMAdapter implementation

LegacyLLMAdapter::LegacyLLMAdapter(
    std::unique_ptr<ILLMClient> client,
    LLMConfig config,
    std::shared_ptr<spdlog::logger> logger)
    : client_(std::move(client))
    , config_(std::move(config))
    , logger_(std::move(logger))
{
    if (client_) {
        client_->set_prompt_logging_enabled(config_.enable_prompt_logging);
    }
}

Result<CategorizationResult> LegacyLLMAdapter::categorize(
    const std::string& file_name,
    const std::string& file_path,
    FileType file_type,
    const std::string& context,
    std::atomic<bool>* cancel_flag)
{
    if (!client_) {
        return make_error(ErrorCode::InvalidState, "LLM client not initialized");
    }

    // Check cancellation before starting
    if (cancel_flag && cancel_flag->load()) {
        return make_error(ErrorCode::Cancelled, "Operation cancelled");
    }

    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string response = client_->categorize_file(
            file_name, file_path, file_type, context);

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        // Parse response into category/subcategory
        CategorizationResult result;
        result.raw_response = response;
        result.duration = duration;

        // Parse "Category : Subcategory" format
        // LLM responses may vary in delimiter formatting, so we check multiple patterns
        // in order of specificity:
        // 1. " : " - canonical format with spaces (most specific)
        // 2. ": "  - colon followed by space
        // 3. " :"  - space followed by colon
        // 4. ":"   - bare colon (least specific, as fallback)
        // The order prevents " : " from matching at the wrong position when only ":" exists
        std::vector<std::string> delimiters = {" : ", ": ", " :", ":"};
        bool found = false;
        
        for (const auto& delimiter : delimiters) {
            size_t pos = response.find(delimiter);
            if (pos != std::string::npos) {
                result.category = response.substr(0, pos);
                result.subcategory = response.substr(pos + delimiter.size());
                found = true;
                break;
            }
        }
        
        if (!found) {
            // No delimiter found, use entire response as category
            result.category = response;
            result.subcategory = "";
        }

        // Use StringUtils for consistent trimming across the codebase
        result.category = StringUtils::trim_copy(result.category);
        result.subcategory = StringUtils::trim_copy(result.subcategory);

        if (logger_) {
            logger_->debug("Categorized '{}' as '{}' / '{}' in {}ms",
                          file_name, result.category, result.subcategory, 
                          duration.count());
        }

        return result;

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error("LLM categorization failed for '{}': {}", 
                          file_name, e.what());
        }
        
        // Map known exception types to error codes
        std::string what = e.what();
        ErrorCode code = ErrorCode::LlmInferenceFailed;
        
        if (what.find("timeout") != std::string::npos || 
            what.find("Timeout") != std::string::npos) {
            code = ErrorCode::LlmTimeout;
        } else if (what.find("rate limit") != std::string::npos ||
                   what.find("429") != std::string::npos) {
            code = ErrorCode::ApiRateLimited;
        } else if (what.find("authentication") != std::string::npos ||
                   what.find("401") != std::string::npos) {
            code = ErrorCode::ApiAuthFailed;
        } else if (what.find("out of memory") != std::string::npos) {
            code = ErrorCode::LlmOutOfMemory;
        }

        return make_error(code, "Categorization failed", what);
    }
}

Result<std::string> LegacyLLMAdapter::complete(
    const std::string& prompt,
    int max_tokens,
    std::atomic<bool>* cancel_flag)
{
    if (!client_) {
        return make_error(ErrorCode::InvalidState, "LLM client not initialized");
    }

    if (cancel_flag && cancel_flag->load()) {
        return make_error(ErrorCode::Cancelled, "Operation cancelled");
    }

    try {
        return client_->complete_prompt(prompt, max_tokens);
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error("LLM completion failed: {}", e.what());
        }
        return make_error(ErrorCode::LlmInferenceFailed, "Completion failed", e.what());
    }
}

Result<void> LegacyLLMAdapter::check_ready() const
{
    if (!client_) {
        return make_error(ErrorCode::InvalidState, "LLM client not initialized");
    }

    // For remote clients, validate API key
    if (is_remote_choice(config_.choice)) {
        auto validation = InputValidator::validate_api_key(
            config_.api_key,
            config_.choice == LLMChoice::Remote_OpenAI ? "OpenAI" : "Gemini");
        if (!validation) {
            return validation.error();
        }
    }

    // For custom LLM, validate path
    if (config_.choice == LLMChoice::Custom) {
        auto validation = InputValidator::validate_file_path(config_.custom_llm_path);
        if (!validation) {
            return validation.error();
        }
    }

    return ok();
}

void LegacyLLMAdapter::set_progress_callback(LLMProgressCallback callback)
{
    progress_callback_ = std::move(callback);
}

void LegacyLLMAdapter::set_prompt_logging(bool enabled)
{
    config_.enable_prompt_logging = enabled;
    if (client_) {
        client_->set_prompt_logging_enabled(enabled);
    }
}

bool LegacyLLMAdapter::is_local() const
{
    return !is_remote_choice(config_.choice);
}

std::string LegacyLLMAdapter::display_name() const
{
    std::string name = llm_choice_to_string(config_.choice);
    if (!config_.model_name.empty()) {
        name += " (" + config_.model_name + ")";
    }
    return name;
}

} // namespace afs
