#include "APIUsageTracker.hpp"
#include "Logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Model pricing per 1M tokens (approximate, as of 2025)
const std::map<std::string, float> APIUsageTracker::model_costs_ = {
    // OpenAI models (combined input + output average)
    {"gpt-4o-mini", 0.30f},      // ~$0.15 input, ~$0.60 output avg
    {"gpt-4o", 5.00f},           // ~$2.50 input, ~$10.00 output avg
    {"gpt-4", 30.00f},           // ~$30 input, ~$60 output avg
    {"gpt-3.5-turbo", 1.00f},    // ~$0.50 input, ~$1.50 output avg
    {"o3-mini", 1.00f},          // Approximate pricing
    
    // Gemini models (free tier, but track for awareness)
    {"gemini-1.5-flash", 0.00f}, // Free tier
    {"gemini-1.5-pro", 0.00f},   // Free tier
    {"gemini-pro", 0.00f},       // Free tier
};

APIUsageTracker::APIUsageTracker(DatabaseManager& db) : db_(db) {}

void APIUsageTracker::record_request(const std::string& provider, 
                                     int tokens, 
                                     const std::string& model) {
    float cost = estimate_cost(model, tokens);
    
    if (!db_.record_api_usage(provider, tokens, 1, cost)) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->warn("Failed to record API usage for {}", provider);
        }
    } else {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->debug("Recorded {} API usage: {} tokens, ${:.4f}", 
                    provider, tokens, cost);
        }
    }
}

APIUsageTracker::UsageStats APIUsageTracker::get_stats(const std::string& provider) {
    UsageStats stats;
    stats.provider = provider;
    
    // Get today's usage
    auto today_usage = db_.get_api_usage_today(provider);
    if (today_usage) {
        stats.tokens_used_today = today_usage->tokens_used;
        stats.requests_today = today_usage->requests_made;
        stats.estimated_cost_today = today_usage->cost_estimate;
    }
    
    // Get monthly usage (last 30 days)
    auto history = db_.get_api_usage_history(provider, 30);
    float monthly_cost = 0.0f;
    for (const auto& entry : history) {
        monthly_cost += entry.cost_estimate;
    }
    stats.estimated_cost_month = monthly_cost;
    
    // Calculate remaining free requests for Gemini
    if (provider == "gemini") {
        stats.remaining_free_requests = GEMINI_FREE_RPD - stats.requests_today;
        if (stats.remaining_free_requests < 0) {
            stats.remaining_free_requests = 0;
        }
    }
    
    // Calculate reset time (midnight today)
    auto now = std::chrono::system_clock::now();
    auto tomorrow = now + std::chrono::hours(24);
    auto tomorrow_time = std::chrono::system_clock::to_time_t(tomorrow);
    std::tm tm_tomorrow;
    
#ifdef _WIN32
    localtime_s(&tm_tomorrow, &tomorrow_time);
#else
    localtime_r(&tomorrow_time, &tm_tomorrow);
#endif
    
    tm_tomorrow.tm_hour = 0;
    tm_tomorrow.tm_min = 0;
    tm_tomorrow.tm_sec = 0;
    
    auto reset_time = std::mktime(&tm_tomorrow);
    std::ostringstream oss;
    oss << std::put_time(&tm_tomorrow, "%Y-%m-%d %H:%M:%S");
    stats.reset_time = oss.str();
    
    return stats;
}

bool APIUsageTracker::is_approaching_limit(const std::string& provider) {
    if (provider != "gemini") {
        return false;  // OpenAI doesn't have hard free tier limits we track
    }
    
    auto stats = get_stats(provider);
    // Consider "approaching limit" as 80% of daily quota
    return stats.requests_today >= (GEMINI_FREE_RPD * 0.8);
}

float APIUsageTracker::estimate_cost(const std::string& model, int tokens) {
    // Find the model in our pricing map
    auto it = model_costs_.find(model);
    if (it != model_costs_.end()) {
        // Cost is per 1M tokens
        return (tokens / 1000000.0f) * it->second;
    }
    
    // Try to find a partial match (e.g., "gpt-4o-mini-2024-07-18" should match "gpt-4o-mini")
    for (const auto& [key, cost] : model_costs_) {
        if (model.find(key) != std::string::npos) {
            return (tokens / 1000000.0f) * cost;
        }
    }
    
    // Default to a conservative estimate if unknown
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->warn("Unknown model '{}' for cost estimation, using default", model);
    }
    return (tokens / 1000000.0f) * 1.0f;  // Default $1 per 1M tokens
}
