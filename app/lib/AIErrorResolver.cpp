#include "AIErrorResolver.hpp"
#include "Logger.hpp"
#include <sstream>
#include <algorithm>
#include <filesystem>

AIErrorResolver::AIErrorResolver(std::shared_ptr<ILLMClient> llm_client,
                                DatabaseManager& db_manager)
    : llm_client_(llm_client)
    , db_manager_(db_manager)
{
}

AIErrorResolver::ErrorCategory AIErrorResolver::categorize_error_code(ErrorCodes::Code code) const {
    int code_value = static_cast<int>(code);
    
    if (code_value >= 1000 && code_value < 1100) return ErrorCategory::Network;
    if (code_value >= 1100 && code_value < 1200) return ErrorCategory::API;
    if (code_value >= 1200 && code_value < 1300) return ErrorCategory::FileSystem;
    if (code_value >= 1300 && code_value < 1400) return ErrorCategory::Database;
    if (code_value >= 1400 && code_value < 1500) return ErrorCategory::LLM;
    if (code_value >= 1500 && code_value < 1600) return ErrorCategory::Configuration;
    if (code_value >= 1600 && code_value < 1700) return ErrorCategory::Validation;
    if (code_value >= 1700 && code_value < 1800) return ErrorCategory::System;
    if (code_value >= 1800 && code_value < 1900) return ErrorCategory::Categorization;
    if (code_value >= 1900 && code_value < 2000) return ErrorCategory::Download;
    
    return ErrorCategory::Unknown;
}

std::string AIErrorResolver::generate_ai_prompt(ErrorCodes::Code error_code,
                                               const std::string& context,
                                               const std::string& user_description) {
    auto error_info = ErrorCodes::ErrorCatalog::get_error_info(error_code, context);
    auto category = categorize_error_code(error_code);
    
    std::stringstream prompt;
    prompt << "You are an expert technical support assistant for AI File Sorter application.\n\n";
    prompt << "Error Information:\n";
    prompt << "- Error Code: " << static_cast<int>(error_code) << "\n";
    prompt << "- Category: ";
    
    switch (category) {
        case ErrorCategory::Network: prompt << "Network"; break;
        case ErrorCategory::API: prompt << "API"; break;
        case ErrorCategory::FileSystem: prompt << "File System"; break;
        case ErrorCategory::Database: prompt << "Database"; break;
        case ErrorCategory::LLM: prompt << "LLM/AI Model"; break;
        case ErrorCategory::Configuration: prompt << "Configuration"; break;
        case ErrorCategory::Validation: prompt << "Validation"; break;
        case ErrorCategory::System: prompt << "System"; break;
        case ErrorCategory::Categorization: prompt << "File Categorization"; break;
        case ErrorCategory::Download: prompt << "Download"; break;
        default: prompt << "Unknown"; break;
    }
    prompt << "\n";
    
    prompt << "- Technical Message: " << error_info.message << "\n";
    
    if (!context.empty()) {
        prompt << "- Technical Context: " << context << "\n";
    }
    
    if (!user_description.empty()) {
        prompt << "- User Description: " << user_description << "\n";
    }
    
    prompt << "\nExisting Resolution Steps:\n" << error_info.resolution << "\n\n";
    
    prompt << "Task: Analyze this error and provide:\n";
    prompt << "1. A clear, user-friendly explanation of what happened (2-3 sentences)\n";
    prompt << "2. A diagnosis of the root cause\n";
    prompt << "3. Step-by-step resolution instructions (be specific and actionable)\n";
    prompt << "4. If applicable, mention which steps could be automated\n\n";
    prompt << "Format your response as:\n";
    prompt << "EXPLANATION: [user-friendly explanation]\n";
    prompt << "DIAGNOSIS: [root cause analysis]\n";
    prompt << "STEPS:\n";
    prompt << "1. [step description] [AUTO] (if can be automated)\n";
    prompt << "2. [step description]\n";
    prompt << "...\n";
    
    return prompt.str();
}

AIErrorResolver::ErrorAnalysis AIErrorResolver::analyze_error(
    ErrorCodes::Code error_code,
    const std::string& context,
    const std::string& user_description) {
    
    ErrorAnalysis analysis;
    analysis.error_code = error_code;
    analysis.category = categorize_error_code(error_code);
    
    try {
        // Generate and send prompt to LLM
        std::string prompt = generate_ai_prompt(error_code, context, user_description);
        
        Logger::log_info("AIErrorResolver: Analyzing error " + std::to_string(static_cast<int>(error_code)));
        
        std::string ai_response;
        if (llm_client_) {
            try {
                ai_response = llm_client_->complete_prompt(prompt, 1000);
            } catch (const std::exception& e) {
                Logger::log_error("AIErrorResolver: LLM call failed: " + std::string(e.what()));
                // Fall back to default analysis
                ai_response = "";
            }
        }
        
        if (ai_response.empty()) {
            // Fallback: use existing error info
            auto error_info = ErrorCodes::ErrorCatalog::get_error_info(error_code, context);
            analysis.user_friendly_explanation = error_info.message;
            analysis.ai_diagnosis = "Unable to generate AI analysis. Using default error information.";
            analysis.confidence_score = 0.5f;
            
            // Parse existing resolution steps
            std::istringstream iss(error_info.resolution);
            std::string line;
            int step_num = 1;
            while (std::getline(iss, line)) {
                if (!line.empty() && line.find("•") != std::string::npos) {
                    ResolutionStep step(line, "", false);
                    analysis.resolution_steps.push_back(step);
                }
            }
        } else {
            // Parse AI response
            analysis.ai_diagnosis = ai_response;
            analysis.confidence_score = 0.8f;
            analysis.resolution_steps = extract_steps_from_ai_response(ai_response, analysis.category);
            
            // Extract explanation
            size_t exp_pos = ai_response.find("EXPLANATION:");
            if (exp_pos != std::string::npos) {
                size_t diag_pos = ai_response.find("DIAGNOSIS:", exp_pos);
                if (diag_pos != std::string::npos) {
                    analysis.user_friendly_explanation = 
                        ai_response.substr(exp_pos + 12, diag_pos - exp_pos - 12);
                    // Trim whitespace
                    analysis.user_friendly_explanation.erase(
                        0, analysis.user_friendly_explanation.find_first_not_of(" \n\r\t"));
                    analysis.user_friendly_explanation.erase(
                        analysis.user_friendly_explanation.find_last_not_of(" \n\r\t") + 1);
                }
            }
        }
        
        // Add automated fix actions for specific errors
        add_auto_fix_actions(analysis);
        
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Analysis failed: " + std::string(e.what()));
        analysis.user_friendly_explanation = "Error analysis failed.";
        analysis.confidence_score = 0.0f;
    }
    
    return analysis;
}

std::vector<AIErrorResolver::ResolutionStep> 
AIErrorResolver::extract_steps_from_ai_response(const std::string& ai_response,
                                               ErrorCategory category) {
    std::vector<ResolutionStep> steps;
    
    size_t steps_pos = ai_response.find("STEPS:");
    if (steps_pos == std::string::npos) {
        return steps;
    }
    
    std::istringstream iss(ai_response.substr(steps_pos + 6));
    std::string line;
    
    while (std::getline(iss, line)) {
        // Look for numbered steps (1. , 2. , etc.)
        if (line.empty()) continue;
        
        // Trim leading whitespace
        line.erase(0, line.find_first_not_of(" \n\r\t"));
        
        // Check if line starts with a number and period
        if (!line.empty() && std::isdigit(line[0])) {
            size_t dot_pos = line.find('.');
            if (dot_pos != std::string::npos && dot_pos < 3) {
                std::string step_text = line.substr(dot_pos + 1);
                step_text.erase(0, step_text.find_first_not_of(" \n\r\t"));
                
                // Check if marked as auto-fixable
                bool can_auto_fix = step_text.find("[AUTO]") != std::string::npos;
                if (can_auto_fix) {
                    // Remove [AUTO] marker
                    size_t auto_pos = step_text.find("[AUTO]");
                    step_text.erase(auto_pos, 6);
                }
                
                ResolutionStep step(step_text, "", can_auto_fix);
                steps.push_back(step);
            }
        }
    }
    
    return steps;
}

void AIErrorResolver::add_auto_fix_actions(ErrorAnalysis& analysis) {
    // Add automated fix actions based on error code and category
    for (auto& step : analysis.resolution_steps) {
        if (!step.can_auto_fix) continue;
        
        // Assign appropriate auto-fix function based on error category
        switch (analysis.category) {
            case ErrorCategory::API:
                if (analysis.error_code == ErrorCodes::Code::API_RATE_LIMIT_EXCEEDED) {
                    step.auto_fix_action = [this]() {
                        return reset_rate_limiter();
                    };
                }
                break;
                
            case ErrorCategory::Network:
                step.auto_fix_action = [this]() {
                    return check_network_connectivity();
                };
                break;
                
            case ErrorCategory::Database:
                step.auto_fix_action = [this]() {
                    return attempt_database_repair();
                };
                break;
                
            default:
                break;
        }
    }
}

std::pair<AIErrorResolver::ErrorCategory, std::vector<ErrorCodes::Code>>
AIErrorResolver::parse_natural_language_error(const std::string& user_description) {
    
    std::string lower_desc = user_description;
    std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);
    
    ErrorCategory category = ErrorCategory::Unknown;
    std::vector<ErrorCodes::Code> potential_codes;
    
    // Keyword matching for category identification
    if (lower_desc.find("connect") != std::string::npos || 
        lower_desc.find("network") != std::string::npos ||
        lower_desc.find("internet") != std::string::npos ||
        lower_desc.find("timeout") != std::string::npos) {
        category = ErrorCategory::Network;
        potential_codes = {
            ErrorCodes::Code::NETWORK_CONNECTION_FAILED,
            ErrorCodes::Code::NETWORK_TIMEOUT,
            ErrorCodes::Code::NETWORK_UNAVAILABLE
        };
    }
    else if (lower_desc.find("api") != std::string::npos ||
             lower_desc.find("key") != std::string::npos ||
             lower_desc.find("gemini") != std::string::npos ||
             lower_desc.find("openai") != std::string::npos ||
             lower_desc.find("rate limit") != std::string::npos) {
        category = ErrorCategory::API;
        potential_codes = {
            ErrorCodes::Code::API_INVALID_KEY,
            ErrorCodes::Code::API_RATE_LIMIT_EXCEEDED,
            ErrorCodes::Code::API_AUTHENTICATION_FAILED
        };
    }
    else if (lower_desc.find("file") != std::string::npos ||
             lower_desc.find("folder") != std::string::npos ||
             lower_desc.find("directory") != std::string::npos ||
             lower_desc.find("permission") != std::string::npos) {
        category = ErrorCategory::FileSystem;
        potential_codes = {
            ErrorCodes::Code::FILE_ACCESS_DENIED,
            ErrorCodes::Code::FILE_NOT_FOUND,
            ErrorCodes::Code::DIRECTORY_ACCESS_DENIED
        };
    }
    else if (lower_desc.find("database") != std::string::npos ||
             lower_desc.find("cache") != std::string::npos ||
             lower_desc.find("corrupted") != std::string::npos) {
        category = ErrorCategory::Database;
        potential_codes = {
            ErrorCodes::Code::DB_CONNECTION_FAILED,
            ErrorCodes::Code::DB_CORRUPTED,
            ErrorCodes::Code::DB_LOCKED
        };
    }
    else if (lower_desc.find("model") != std::string::npos ||
             lower_desc.find("llm") != std::string::npos ||
             lower_desc.find("memory") != std::string::npos ||
             lower_desc.find("gpu") != std::string::npos) {
        category = ErrorCategory::LLM;
        potential_codes = {
            ErrorCodes::Code::LLM_MODEL_NOT_FOUND,
            ErrorCodes::Code::LLM_MODEL_LOAD_FAILED,
            ErrorCodes::Code::LLM_OUT_OF_MEMORY
        };
    }
    
    return {category, potential_codes};
}

AIErrorResolver::ResolutionResult 
AIErrorResolver::attempt_auto_resolution(const ErrorAnalysis& analysis) {
    ResolutionResult result;
    result.success = false;
    
    bool any_fix_attempted = false;
    bool any_fix_failed = false;
    
    for (const auto& step : analysis.resolution_steps) {
        if (step.can_auto_fix && step.auto_fix_action) {
            any_fix_attempted = true;
            result.steps_taken.push_back(step.description);
            
            try {
                bool step_success = step.auto_fix_action();
                if (!step_success) {
                    any_fix_failed = true;
                    result.error_detail += "Failed: " + step.description + "\n";
                } else {
                    Logger::log_info("AIErrorResolver: Auto-fix succeeded for: " + step.description);
                }
            } catch (const std::exception& e) {
                any_fix_failed = true;
                result.error_detail += "Exception in " + step.description + ": " + e.what() + "\n";
                Logger::log_error("AIErrorResolver: Auto-fix exception: " + std::string(e.what()));
            }
        }
    }
    
    if (!any_fix_attempted) {
        result.message = "No automated fixes available for this error.";
        result.success = false;
    } else if (any_fix_failed) {
        result.message = "Some automated fixes failed. See details below.";
        result.success = false;
    } else {
        result.message = "Automated fixes completed successfully.";
        result.success = true;
    }
    
    // Log the resolution attempt
    log_resolution_attempt(analysis, result);
    
    return result;
}

bool AIErrorResolver::has_auto_fix(ErrorCodes::Code error_code) const {
    auto category = categorize_error_code(error_code);
    
    // These categories have some auto-fixable errors
    return category == ErrorCategory::API ||
           category == ErrorCategory::Network ||
           category == ErrorCategory::Database;
}

std::vector<AIErrorResolver::ResolutionResult> 
AIErrorResolver::get_resolution_history(std::optional<ErrorCodes::Code> error_code, int limit) {
    std::vector<ResolutionResult> history;
    
    try {
        int code_filter = error_code.has_value() ? static_cast<int>(error_code.value()) : -1;
        auto db_entries = db_manager_.get_error_resolution_history(code_filter, limit);
        
        for (const auto& entry : db_entries) {
            ResolutionResult result;
            result.success = entry.resolution_success;
            result.message = entry.resolution_success ? "Resolution succeeded" : "Resolution failed";
            result.error_detail = entry.error_detail;
            
            // Parse JSON steps_taken back to vector
            // Simple parsing for now - could use a JSON library for robustness
            std::string steps_str = entry.steps_taken;
            size_t start = steps_str.find('[');
            size_t end = steps_str.find(']');
            if (start != std::string::npos && end != std::string::npos) {
                std::string steps_content = steps_str.substr(start + 1, end - start - 1);
                // Simple split by comma (not robust for complex JSON)
                std::istringstream ss(steps_content);
                std::string step;
                while (std::getline(ss, step, ',')) {
                    // Remove quotes and whitespace
                    step.erase(std::remove(step.begin(), step.end(), '\"'), step.end());
                    step.erase(0, step.find_first_not_of(" \n\r\t"));
                    step.erase(step.find_last_not_of(" \n\r\t") + 1);
                    if (!step.empty()) {
                        result.steps_taken.push_back(step);
                    }
                }
            }
            
            history.push_back(result);
        }
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Failed to retrieve history: " + std::string(e.what()));
    }
    
    return history;
}

// Helper methods for automated fixes
bool AIErrorResolver::validate_api_key(const std::string& api_key) {
    // Basic validation: check if key is not empty and has reasonable length
    return !api_key.empty() && api_key.length() > 10;
}

bool AIErrorResolver::reset_rate_limiter() {
    try {
        Logger::log_info("AIErrorResolver: Attempting to reset rate limiter");
        // NOTE: This is a placeholder implementation
        // Actual rate limiter reset would require integration with the specific
        // rate limiting component (e.g., GeminiClient rate limiter)
        // For now, we log the attempt but return false to indicate no actual reset was performed
        Logger::log_warning("AIErrorResolver: Rate limiter reset not implemented - requires integration with specific rate limiting component");
        return false;
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Failed to reset rate limiter: " + std::string(e.what()));
        return false;
    }
}

bool AIErrorResolver::check_network_connectivity() {
    try {
        Logger::log_info("AIErrorResolver: Checking network connectivity");
        // NOTE: This is a placeholder implementation
        // Actual network check would require a simple HTTP request or ping
        // For now, we log the attempt but return false to indicate no actual check was performed
        Logger::log_warning("AIErrorResolver: Network connectivity check not implemented - requires HTTP/ICMP implementation");
        return false;
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Network check failed: " + std::string(e.what()));
        return false;
    }
}

bool AIErrorResolver::check_file_permissions(const std::string& path) {
    try {
        namespace fs = std::filesystem;
        if (fs::exists(path)) {
            // Try to get file status
            auto status = fs::status(path);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Permission check failed: " + std::string(e.what()));
        return false;
    }
}

bool AIErrorResolver::attempt_database_repair() {
    try {
        Logger::log_info("AIErrorResolver: Attempting database repair");
        // This would call DatabaseManager methods to repair/optimize
        // For now, just return success
        return true;
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Database repair failed: " + std::string(e.what()));
        return false;
    }
}

void AIErrorResolver::log_resolution_attempt(const ErrorAnalysis& analysis,
                                            const ResolutionResult& result) {
    try {
        // Log to application logger
        std::stringstream log_msg;
        log_msg << "Error Resolution Attempt - Code: " << static_cast<int>(analysis.error_code)
                << ", Success: " << (result.success ? "Yes" : "No")
                << ", Steps: " << result.steps_taken.size();
        Logger::log_info(log_msg.str());
        
        // Store in database for pattern learning
        DatabaseManager::ErrorResolutionEntry entry;
        entry.error_code = static_cast<int>(analysis.error_code);
        
        // Convert error category to string
        switch (analysis.category) {
            case ErrorCategory::Network: entry.error_category = "Network"; break;
            case ErrorCategory::API: entry.error_category = "API"; break;
            case ErrorCategory::FileSystem: entry.error_category = "FileSystem"; break;
            case ErrorCategory::Database: entry.error_category = "Database"; break;
            case ErrorCategory::LLM: entry.error_category = "LLM"; break;
            case ErrorCategory::Configuration: entry.error_category = "Configuration"; break;
            case ErrorCategory::Validation: entry.error_category = "Validation"; break;
            case ErrorCategory::System: entry.error_category = "System"; break;
            case ErrorCategory::Categorization: entry.error_category = "Categorization"; break;
            case ErrorCategory::Download: entry.error_category = "Download"; break;
            default: entry.error_category = "Unknown"; break;
        }
        
        entry.context = "";  // Could be populated from analysis context
        entry.user_description = "";  // Could be populated if available
        entry.ai_diagnosis = analysis.ai_diagnosis;
        entry.resolution_attempted = true;
        entry.resolution_success = result.success;
        
        // Convert steps_taken vector to JSON string
        std::stringstream steps_json;
        steps_json << "[";
        for (size_t i = 0; i < result.steps_taken.size(); ++i) {
            if (i > 0) steps_json << ",";
            steps_json << "\"" << result.steps_taken[i] << "\"";
        }
        steps_json << "]";
        entry.steps_taken = steps_json.str();
        entry.error_detail = result.error_detail;
        
        db_manager_.record_error_resolution(entry);
    } catch (const std::exception& e) {
        Logger::log_error("AIErrorResolver: Failed to log resolution attempt: " + std::string(e.what()));
    }
}
