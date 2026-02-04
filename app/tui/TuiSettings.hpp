#ifndef TUI_SETTINGS_HPP
#define TUI_SETTINGS_HPP

#include "IniConfig.hpp"
#include "Types.hpp"
#include <string>
#include <filesystem>
#include <vector>
#include <functional>

/**
 * TUI-specific Settings class that removes Qt dependencies.
 * This is a drop-in replacement for the Qt-based Settings class.
 */
class TuiSettings
{
public:
    TuiSettings();

    bool load();
    bool save();

    LLMChoice get_llm_choice() const;
    void set_llm_choice(LLMChoice choice);
    std::string get_openai_api_key() const;
    void set_openai_api_key(const std::string& key);
    std::string get_openai_model() const;
    void set_openai_model(const std::string& model);
    std::string get_gemini_api_key() const;
    void set_gemini_api_key(const std::string& key);
    std::string get_gemini_model() const;
    void set_gemini_model(const std::string& model);
    std::string get_active_custom_llm_id() const;
    void set_active_custom_llm_id(const std::string& id);
    const std::vector<CustomLLM>& get_custom_llms() const;
    std::string upsert_custom_llm(const CustomLLM& llm);
    void remove_custom_llm(const std::string& id);
    CustomLLM find_custom_llm(const std::string& id) const;
    bool is_llm_chosen() const;

    bool get_use_subcategories() const;
    void set_use_subcategories(bool value);

    bool get_use_consistency_hints() const;
    void set_use_consistency_hints(bool value);

    bool get_categorize_files() const;
    void set_categorize_files(bool value);

    bool get_categorize_directories() const;
    void set_categorize_directories(bool value);

    std::string get_sort_folder() const;
    void set_sort_folder(const std::string &path);

    bool get_consistency_pass_enabled() const;
    void set_consistency_pass_enabled(bool value);

    bool get_use_whitelist() const;
    void set_use_whitelist(bool value);
    std::string get_active_whitelist() const;
    void set_active_whitelist(const std::string& name);

    bool get_development_prompt_logging() const;
    void set_development_prompt_logging(bool value);

    std::string define_config_path();
    std::string get_config_dir();

    void set_skipped_version(const std::string &version);
    std::string get_skipped_version();
    
    int get_total_categorized_files() const;
    void add_categorized_files(int count);
    int get_next_support_prompt_threshold() const;
    void set_next_support_prompt_threshold(int threshold);
    
    std::vector<std::string> get_allowed_categories() const;
    void set_allowed_categories(std::vector<std::string> values);
    std::vector<std::string> get_allowed_subcategories() const;
    void set_allowed_subcategories(std::vector<std::string> values);

    // Category language as string (for TUI display)
    std::string get_category_language_string() const;
    void set_category_language_string(const std::string& lang);

private:
    LLMChoice parse_llm_choice() const;
    void load_basic_settings();
    void load_whitelist_settings();
    void load_custom_llm_settings();
    void log_loaded_settings() const;

    void save_core_settings();
    void save_whitelist_settings();
    void save_custom_llms();

    std::string config_path;
    std::filesystem::path config_dir;
    IniConfig config;

    LLMChoice llm_choice = LLMChoice::Local_7b;
    std::string openai_api_key;
    std::string openai_model{ "gpt-4o-mini" };
    std::string gemini_api_key;
    std::string gemini_model{ "gemini-2.5-flash-lite" };
    bool use_subcategories{true};
    bool categorize_files{true};
    bool categorize_directories{false};
    bool use_consistency_hints{false};
    bool use_whitelist{false};
    std::string default_sort_folder;
    std::string sort_folder;
    std::string skipped_version;
    std::string category_language_str{"English"};
    bool consistency_pass_enabled{false};
    bool development_prompt_logging{false};
    int categorized_file_count{0};
    int next_support_prompt_threshold{200};
    std::vector<std::string> allowed_categories;
    std::vector<std::string> allowed_subcategories;
    std::string active_whitelist;
    std::vector<CustomLLM> custom_llms;
    std::string active_custom_llm_id;
};

#endif // TUI_SETTINGS_HPP
