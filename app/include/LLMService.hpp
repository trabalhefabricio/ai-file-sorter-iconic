#ifndef LLM_SERVICE_HPP
#define LLM_SERVICE_HPP

#include "ILLMClient.hpp"
#include "Result.hpp"
#include "Types.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class Settings;
namespace spdlog { class logger; }

namespace afs {

/**
 * @brief Configuration for LLM service operations.
 */
struct LLMConfig {
    LLMChoice choice = LLMChoice::Local_7b;
    std::string api_key;
    std::string model_name;
    std::string custom_llm_path;
    std::string custom_llm_id;
    int timeout_seconds = 120;
    bool enable_prompt_logging = false;
};

/**
 * @brief Result of a categorization operation.
 */
struct CategorizationResult {
    std::string category;
    std::string subcategory;
    std::string raw_response;
    std::chrono::milliseconds duration{0};
    bool from_fallback{false};
};

/**
 * @brief Progress callback for long-running LLM operations.
 */
using LLMProgressCallback = std::function<void(const std::string& message)>;

/**
 * @brief Unified LLM service that wraps different LLM backends.
 * 
 * Provides a consistent interface regardless of the underlying LLM provider
 * (local LLaMa, OpenAI, Gemini, custom models).
 */
class LLMService {
public:
    /**
     * @brief Factory method to create an LLM service with the given configuration.
     * @param config Configuration for the LLM service
     * @param logger Optional logger for diagnostics
     * @return Result containing the service or an error
     */
    static Result<std::unique_ptr<LLMService>> create(
        const LLMConfig& config,
        std::shared_ptr<spdlog::logger> logger = nullptr);

    /**
     * @brief Factory method to create from Settings.
     * @param settings Application settings containing LLM configuration
     * @param logger Optional logger for diagnostics
     * @return Result containing the service or an error
     */
    static Result<std::unique_ptr<LLMService>> create_from_settings(
        const Settings& settings,
        std::shared_ptr<spdlog::logger> logger = nullptr);

    virtual ~LLMService() = default;

    /**
     * @brief Categorizes a file or directory.
     * @param file_name Name of the file/directory
     * @param file_path Full path to the file/directory
     * @param file_type Whether it's a file or directory
     * @param context Optional context for consistency (previous categorizations)
     * @param cancel_flag Optional flag to signal cancellation
     * @return Result containing categorization or error
     */
    virtual Result<CategorizationResult> categorize(
        const std::string& file_name,
        const std::string& file_path,
        FileType file_type,
        const std::string& context = "",
        std::atomic<bool>* cancel_flag = nullptr) = 0;

    /**
     * @brief Completes a raw prompt.
     * @param prompt The prompt to complete
     * @param max_tokens Maximum tokens in response
     * @param cancel_flag Optional flag to signal cancellation
     * @return Result containing the completion or error
     */
    virtual Result<std::string> complete(
        const std::string& prompt,
        int max_tokens,
        std::atomic<bool>* cancel_flag = nullptr) = 0;

    /**
     * @brief Checks if the LLM backend is ready for requests.
     * @return Result indicating readiness or error with details
     */
    virtual Result<void> check_ready() const = 0;

    /**
     * @brief Gets the current configuration.
     */
    virtual const LLMConfig& config() const = 0;

    /**
     * @brief Sets the progress callback for long operations.
     */
    virtual void set_progress_callback(LLMProgressCallback callback) = 0;

    /**
     * @brief Enables or disables prompt logging (for development).
     */
    virtual void set_prompt_logging(bool enabled) = 0;

    /**
     * @brief Gets whether this is a local (vs. remote/API) LLM.
     */
    [[nodiscard]] virtual bool is_local() const = 0;

    /**
     * @brief Gets a human-readable name for the current LLM.
     */
    [[nodiscard]] virtual std::string display_name() const = 0;

protected:
    LLMService() = default;
};

/**
 * @brief Adapter to wrap legacy ILLMClient implementations.
 * 
 * This allows gradual migration from the old interface to the new one.
 */
class LegacyLLMAdapter : public LLMService {
public:
    LegacyLLMAdapter(std::unique_ptr<ILLMClient> client,
                     LLMConfig config,
                     std::shared_ptr<spdlog::logger> logger = nullptr);

    Result<CategorizationResult> categorize(
        const std::string& file_name,
        const std::string& file_path,
        FileType file_type,
        const std::string& context,
        std::atomic<bool>* cancel_flag) override;

    Result<std::string> complete(
        const std::string& prompt,
        int max_tokens,
        std::atomic<bool>* cancel_flag) override;

    Result<void> check_ready() const override;
    const LLMConfig& config() const override { return config_; }
    void set_progress_callback(LLMProgressCallback callback) override;
    void set_prompt_logging(bool enabled) override;
    bool is_local() const override;
    std::string display_name() const override;

private:
    std::unique_ptr<ILLMClient> client_;
    LLMConfig config_;
    std::shared_ptr<spdlog::logger> logger_;
    LLMProgressCallback progress_callback_;
};

} // namespace afs

#endif // LLM_SERVICE_HPP
