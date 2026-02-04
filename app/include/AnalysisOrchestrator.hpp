#ifndef ANALYSIS_ORCHESTRATOR_HPP
#define ANALYSIS_ORCHESTRATOR_HPP

#include "Result.hpp"
#include "Types.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace spdlog { class logger; }

namespace afs {

class LLMService;
class ICategorizationRepository;
class Settings;
class FileScanner;

/**
 * @brief Progress information during analysis.
 */
struct AnalysisProgress {
    int total_files{0};
    int processed_files{0};
    int cached_files{0};
    int categorized_files{0};
    int failed_files{0};
    std::string current_file;
    std::string current_status;
    bool is_complete{false};
};

/**
 * @brief Configuration for an analysis run.
 */
struct AnalysisConfig {
    std::string directory_path;
    FileScanOptions scan_options{FileScanOptions::Files};
    bool use_consistency_hints{false};
    bool use_whitelist{false};
    std::vector<std::string> allowed_categories;
    std::vector<std::string> allowed_subcategories;
    int batch_size{10};  // Files to process before saving progress
};

/**
 * @brief Result of an analysis run.
 */
struct AnalysisResult {
    std::vector<CategorizedFile> categorized_files;
    std::vector<CategorizedFile> from_cache;
    std::vector<std::string> failed_files;
    AnalysisProgress final_progress;
    std::chrono::milliseconds duration{0};
};

/**
 * @brief Callbacks for analysis events.
 */
struct AnalysisCallbacks {
    std::function<void(const AnalysisProgress&)> on_progress;
    std::function<void(const CategorizedFile&)> on_file_categorized;
    std::function<void(const std::string&, const Error&)> on_file_failed;
    std::function<void(const std::string&)> on_status_message;
};

/**
 * @brief Orchestrates the file analysis workflow.
 * 
 * Coordinates between:
 * - FileScanner: Discovers files in the target directory
 * - Repository: Checks cache and stores results
 * - LLMService: Performs categorization
 * 
 * Responsibilities:
 * - Managing the analysis lifecycle (start, progress, cancel, complete)
 * - Batching and throttling LLM requests
 * - Reporting progress
 * - Handling errors and retries
 */
class AnalysisOrchestrator {
public:
    /**
     * @brief Creates an orchestrator with required dependencies.
     * @param settings Application settings
     * @param repository Data access for categorizations
     * @param llm_service LLM service for categorization
     * @param scanner File scanner
     * @param logger Optional logger
     */
    AnalysisOrchestrator(
        Settings& settings,
        std::shared_ptr<ICategorizationRepository> repository,
        std::shared_ptr<LLMService> llm_service,
        std::shared_ptr<FileScanner> scanner,
        std::shared_ptr<spdlog::logger> logger = nullptr);

    ~AnalysisOrchestrator();

    /**
     * @brief Validates the configuration before running.
     * @param config The analysis configuration
     * @return Result with any validation errors
     */
    [[nodiscard]] Result<void> validate_config(const AnalysisConfig& config) const;

    /**
     * @brief Runs the analysis synchronously.
     * @param config The analysis configuration
     * @param callbacks Optional callbacks for progress
     * @param cancel_flag Optional flag to signal cancellation
     * @return Analysis result or error
     */
    Result<AnalysisResult> run(
        const AnalysisConfig& config,
        const AnalysisCallbacks& callbacks = {},
        std::atomic<bool>* cancel_flag = nullptr);

    /**
     * @brief Gets the current progress (if analysis is running).
     */
    [[nodiscard]] AnalysisProgress current_progress() const;

    /**
     * @brief Checks if an analysis is currently running.
     */
    [[nodiscard]] bool is_running() const { return running_.load(); }

private:
    // Internal workflow steps
    Result<std::vector<FileEntry>> scan_directory(
        const AnalysisConfig& config,
        const AnalysisCallbacks& callbacks);
    
    Result<std::vector<CategorizedFile>> load_cached_entries(
        const std::string& directory_path,
        const AnalysisCallbacks& callbacks);
    
    std::vector<FileEntry> filter_uncategorized(
        const std::vector<FileEntry>& all_files,
        const std::vector<CategorizedFile>& cached);
    
    Result<void> categorize_batch(
        const std::vector<FileEntry>& batch,
        const AnalysisConfig& config,
        std::vector<CategorizedFile>& results,
        std::vector<std::string>& failures,
        const AnalysisCallbacks& callbacks,
        std::atomic<bool>* cancel_flag);

    void report_progress(const AnalysisCallbacks& callbacks);
    void report_status(const AnalysisCallbacks& callbacks, const std::string& message);
    bool should_abort(std::atomic<bool>* cancel_flag) const;
    std::string build_consistency_context(const std::vector<CategorizedFile>& recent) const;

    Settings& settings_;
    std::shared_ptr<ICategorizationRepository> repository_;
    std::shared_ptr<LLMService> llm_service_;
    std::shared_ptr<FileScanner> scanner_;
    std::shared_ptr<spdlog::logger> logger_;

    std::atomic<bool> running_{false};
    AnalysisProgress progress_;
    mutable std::mutex progress_mutex_;
};

} // namespace afs

#endif // ANALYSIS_ORCHESTRATOR_HPP
