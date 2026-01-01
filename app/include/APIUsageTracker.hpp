#ifndef APIUSAGETRACKER_HPP
#define APIUSAGETRACKER_HPP

#include "DatabaseManager.hpp"
#include <string>
#include <map>
#include <optional>

/**
 * @brief Tracks API usage for OpenAI and Gemini
 * 
 * This class provides methods to record and retrieve API usage statistics,
 * including token counts, request counts, cost estimates, and quota management.
 */
class APIUsageTracker {
public:
    explicit APIUsageTracker(DatabaseManager& db);

    struct UsageStats {
        int tokens_used_today{0};
        int requests_today{0};
        float estimated_cost_today{0.0f};
        float estimated_cost_month{0.0f};
        int remaining_free_requests{0};  // For Gemini free tier
        std::string reset_time;
        std::string provider;
    };

    /**
     * @brief Record an API request
     * @param provider "openai" or "gemini"
     * @param tokens Number of tokens used (for OpenAI)
     * @param model Model name for cost calculation
     */
    void record_request(const std::string& provider, 
                       int tokens, 
                       const std::string& model);

    /**
     * @brief Get usage statistics for a provider
     * @param provider "openai" or "gemini"
     * @return Usage statistics for today
     */
    UsageStats get_stats(const std::string& provider);

    /**
     * @brief Check if approaching rate limits
     * @param provider "openai" or "gemini"
     * @return true if approaching limit (>80% used)
     */
    bool is_approaching_limit(const std::string& provider);

    /**
     * @brief Get cost estimate for a model
     * @param model Model name (e.g., "gpt-4o-mini", "gemini-1.5-flash")
     * @param tokens Number of tokens
     * @return Estimated cost in USD
     */
    static float estimate_cost(const std::string& model, int tokens);

    // Gemini free tier limits
    static constexpr int GEMINI_FREE_RPM = 15;  // Requests per minute
    static constexpr int GEMINI_FREE_RPD = 1500;  // Requests per day

private:
    DatabaseManager& db_;
    
    // Model pricing (cost per 1M tokens in USD)
    static const std::map<std::string, float> model_costs_;
};

#endif // APIUSAGETRACKER_HPP
