#include "TuiSettings.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <filesystem>
#include <cstdio>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <chrono>
#include <random>
#include <cstdlib>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

namespace {

template <typename... Args>
void settings_log(spdlog::level::level_enum level, const char* fmt_str, Args&&... args) {
    auto message = fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...);
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->log(level, "{}", message);
    } else {
        std::fprintf(stderr, "%s\n", message.c_str());
    }
}

int parse_int_or(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

std::vector<std::string> parse_list(const std::string& value) {
    std::vector<std::string> result;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, ',')) {
        auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
        item.erase(item.begin(), std::find_if(item.begin(), item.end(), not_space));
        item.erase(std::find_if(item.rbegin(), item.rend(), not_space).base(), item.end());
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result;
}

std::string join_list(const std::vector<std::string>& items) {
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << items[i];
    }
    return oss.str();
}

std::string to_bool_string(bool value) {
    return value ? "true" : "false";
}

std::string llm_choice_to_string(LLMChoice choice) {
    switch (choice) {
        case LLMChoice::Remote_OpenAI: return "Remote_OpenAI";
        case LLMChoice::Remote_Gemini: return "Remote_Gemini";
        case LLMChoice::Local_3b: return "Local_3b";
        case LLMChoice::Local_7b: return "Local_7b";
        case LLMChoice::Custom: return "Custom";
        default: return "Unset";
    }
}

void set_bool_setting(IniConfig& config, const std::string& section, const char* key, bool value) {
    config.setValue(section, key, to_bool_string(value));
}

void set_optional_setting(IniConfig& config, const std::string& section, const char* key, const std::string& value) {
    if (!value.empty()) {
        config.setValue(section, key, value);
    }
}

std::string generate_custom_llm_id() {
    using clock = std::chrono::steady_clock;
    const auto now = clock::now().time_since_epoch().count();
    std::mt19937_64 rng(static_cast<std::mt19937_64::result_type>(now));
    const uint64_t value = rng();
    std::ostringstream oss;
    oss << "llm_" << std::hex << value;
    return oss.str();
}

std::string get_default_downloads_folder() {
    // Try common paths without Qt
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
        std::filesystem::path downloads = std::filesystem::path(path).parent_path() / "Downloads";
        if (std::filesystem::exists(downloads)) {
            return downloads.string();
        }
    }
#else
    if (const char* home = std::getenv("HOME")) {
        std::filesystem::path downloads = std::filesystem::path(home) / "Downloads";
        if (std::filesystem::exists(downloads)) {
            return downloads.string();
        }
        return home;
    }
#endif
    return std::filesystem::current_path().string();
}

} // anonymous namespace


TuiSettings::TuiSettings()
    : use_subcategories(true),
      categorize_files(true),
      categorize_directories(false),
      use_consistency_hints(false),
      use_whitelist(false),
      default_sort_folder(""),
      sort_folder("")
{
    config_path = define_config_path();
    config_dir = std::filesystem::path(config_path).parent_path();

    try {
        if (!std::filesystem::exists(config_dir)) {
            std::filesystem::create_directories(config_dir);
        }
    } catch (const std::filesystem::filesystem_error &e) {
        settings_log(spdlog::level::err, "Error creating configuration directory: {}", e.what());
    }

    default_sort_folder = get_default_downloads_folder();
    sort_folder = default_sort_folder;
    category_language_str = "English";
}

LLMChoice TuiSettings::parse_llm_choice() const
{
    const std::string value = config.getValue("Settings", "LLMChoice", "Unset");
    if (value == "Remote" || value == "Remote_OpenAI") return LLMChoice::Remote_OpenAI;
    if (value == "Remote_Gemini") return LLMChoice::Remote_Gemini;
    if (value == "Local_3b") return LLMChoice::Local_3b;
    if (value == "Local_7b") return LLMChoice::Local_7b;
    if (value == "Custom")   return LLMChoice::Custom;
    return LLMChoice::Unset;
}

void TuiSettings::load_basic_settings()
{
    const auto load_bool = [&](const char* key, bool def) {
        return config.getValue("Settings", key, def ? "true" : "false") == "true";
    };

    const auto load_int = [&](const char* key, int def, int min_val = std::numeric_limits<int>::min()) {
        int value = parse_int_or(config.getValue("Settings", key, std::to_string(def)), def);
        return value < min_val ? min_val : value;
    };

    llm_choice = parse_llm_choice();
    set_openai_api_key(config.getValue("Settings", "RemoteApiKey", ""));
    set_openai_model(config.getValue("Settings", "RemoteModel", "gpt-4o-mini"));
    set_gemini_api_key(config.getValue("Settings", "GeminiApiKey", ""));
    set_gemini_model(config.getValue("Settings", "GeminiModel", "gemini-2.5-flash-lite"));
    use_subcategories = load_bool("UseSubcategories", false);
    use_consistency_hints = load_bool("UseConsistencyHints", false);
    categorize_files = load_bool("CategorizeFiles", true);
    categorize_directories = load_bool("CategorizeDirectories", false);
    sort_folder = config.getValue("Settings", "SortFolder", default_sort_folder.empty() ? std::string("/") : default_sort_folder);
    consistency_pass_enabled = load_bool("ConsistencyPass", false);
    development_prompt_logging = load_bool("DevelopmentPromptLogging", false);
    skipped_version = config.getValue("Settings", "SkippedVersion", "0.0.0");
    category_language_str = config.getValue("Settings", "CategoryLanguage", "English");
    categorized_file_count = load_int("CategorizedFileCount", 0, 0);
    next_support_prompt_threshold = load_int("SupportPromptThreshold", 200, 200);
}

void TuiSettings::load_whitelist_settings()
{
    const auto load_bool = [&](const char* key, bool def) {
        return config.getValue("Settings", key, def ? "true" : "false") == "true";
    };

    allowed_categories = parse_list(config.getValue("Settings", "AllowedCategories", ""));
    allowed_subcategories = parse_list(config.getValue("Settings", "AllowedSubcategories", ""));
    use_whitelist = load_bool("UseWhitelist", false);
    active_whitelist = config.getValue("Settings", "ActiveWhitelist", "");
}

void TuiSettings::load_custom_llm_settings()
{
    active_custom_llm_id = config.getValue("LLMs", "ActiveCustomId", "");

    custom_llms.clear();
    const auto custom_ids = parse_list(config.getValue("LLMs", "CustomIds", ""));
    for (const auto& id : custom_ids) {
        const std::string section = "LLM_" + id;
        CustomLLM entry;
        entry.id = id;
        entry.name = config.getValue(section, "Name", "");
        entry.description = config.getValue(section, "Description", "");
        entry.path = config.getValue(section, "Path", "");
        if (!entry.name.empty() && !entry.path.empty()) {
            custom_llms.push_back(entry);
        }
    }
}

void TuiSettings::log_loaded_settings() const
{
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->info("TUI: Loaded settings from '{}' (allowed categories: {}, allowed subcategories: {}, use whitelist: {}, active whitelist: '{}', custom llms: {})",
                     config_path,
                     allowed_categories.size(),
                     allowed_subcategories.size(),
                     use_whitelist,
                     active_whitelist,
                     custom_llms.size());
    }
}

void TuiSettings::save_core_settings()
{
    static const std::string settings_section = "Settings";

    config.setValue(settings_section, "LLMChoice", llm_choice_to_string(llm_choice));
    config.setValue(settings_section, "RemoteApiKey", openai_api_key);
    config.setValue(settings_section, "RemoteModel", openai_model.empty() ? "gpt-4o-mini" : openai_model);
    config.setValue(settings_section, "GeminiApiKey", gemini_api_key);
    config.setValue(settings_section, "GeminiModel", gemini_model.empty() ? "gemini-2.5-flash-lite" : gemini_model);
    set_bool_setting(config, settings_section, "UseSubcategories", use_subcategories);
    set_bool_setting(config, settings_section, "UseConsistencyHints", use_consistency_hints);
    set_bool_setting(config, settings_section, "CategorizeFiles", categorize_files);
    set_bool_setting(config, settings_section, "CategorizeDirectories", categorize_directories);
    config.setValue(settings_section, "SortFolder", this->sort_folder);
    set_optional_setting(config, settings_section, "SkippedVersion", skipped_version);
    set_bool_setting(config, settings_section, "ConsistencyPass", consistency_pass_enabled);
    set_bool_setting(config, settings_section, "DevelopmentPromptLogging", development_prompt_logging);
    config.setValue(settings_section, "CategoryLanguage", category_language_str);
    config.setValue(settings_section, "CategorizedFileCount", std::to_string(categorized_file_count));
    config.setValue(settings_section, "SupportPromptThreshold", std::to_string(next_support_prompt_threshold));
}

void TuiSettings::save_whitelist_settings()
{
    static const std::string settings_section = "Settings";

    config.setValue(settings_section, "AllowedCategories", join_list(allowed_categories));
    config.setValue(settings_section, "AllowedSubcategories", join_list(allowed_subcategories));
    set_bool_setting(config, settings_section, "UseWhitelist", use_whitelist);
    set_optional_setting(config, settings_section, "ActiveWhitelist", active_whitelist);
}

void TuiSettings::save_custom_llms()
{
    static const std::string llm_section = "LLMs";

    set_optional_setting(config, llm_section, "ActiveCustomId", active_custom_llm_id);

    std::vector<std::string> ids;
    ids.reserve(custom_llms.size());
    for (const auto& entry : custom_llms) {
        if (!is_valid_custom_llm(entry)) {
            continue;
        }
        ids.push_back(entry.id);
        const std::string section = "LLM_" + entry.id;
        config.setValue(section, "Name", entry.name);
        config.setValue(section, "Description", entry.description);
        config.setValue(section, "Path", entry.path);
    }
    config.setValue(llm_section, "CustomIds", join_list(ids));
}


std::string TuiSettings::define_config_path()
{
    std::string AppName = "AIFileSorter";
    if (const char* override_root = std::getenv("AI_FILE_SORTER_CONFIG_DIR")) {
        std::filesystem::path base = override_root;
        return (base / AppName / "config.ini").string();
    }
#ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return (std::filesystem::path(appDataPath) / AppName / "config.ini").string();
    }
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        return (std::filesystem::path(home) / "Library" / "Application Support" / AppName / "config.ini").string();
    }
#else
    if (const char* home = std::getenv("HOME")) {
        return (std::filesystem::path(home) / ".config" / AppName / "config.ini").string();
    }
#endif
    return "config.ini";
}


std::string TuiSettings::get_config_dir()
{
    return config_dir.string();
}


bool TuiSettings::load()
{
    if (!config.load(config_path)) {
        sort_folder = default_sort_folder.empty() ? std::string("/") : default_sort_folder;
        return false;
    }

    load_basic_settings();
    load_whitelist_settings();
    load_custom_llm_settings();
    log_loaded_settings();

    return true;
}


bool TuiSettings::save()
{
    save_core_settings();
    save_whitelist_settings();
    save_custom_llms();
    return config.save(config_path);
}


LLMChoice TuiSettings::get_llm_choice() const
{
    return llm_choice;
}


void TuiSettings::set_llm_choice(LLMChoice choice)
{
    llm_choice = choice;
}

std::string TuiSettings::get_openai_api_key() const
{
    return openai_api_key;
}

void TuiSettings::set_openai_api_key(const std::string& key)
{
    auto trimmed = key;
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), not_space));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), not_space).base(), trimmed.end());
    openai_api_key = trimmed;
}

std::string TuiSettings::get_openai_model() const
{
    return openai_model;
}

void TuiSettings::set_openai_model(const std::string& model)
{
    auto trimmed = model;
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), not_space));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), not_space).base(), trimmed.end());
    if (trimmed.empty()) {
        trimmed = "gpt-4o-mini";
    }
    openai_model = trimmed;
}

std::string TuiSettings::get_gemini_api_key() const
{
    return gemini_api_key;
}

void TuiSettings::set_gemini_api_key(const std::string& key)
{
    auto trimmed = key;
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), not_space));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), not_space).base(), trimmed.end());
    gemini_api_key = trimmed;
}

std::string TuiSettings::get_gemini_model() const
{
    return gemini_model;
}

void TuiSettings::set_gemini_model(const std::string& model)
{
    auto trimmed = model;
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), not_space));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), not_space).base(), trimmed.end());
    if (trimmed.empty()) {
        trimmed = "gemini-2.5-flash-lite";
    }
    gemini_model = trimmed;
}

std::string TuiSettings::get_active_custom_llm_id() const
{
    return active_custom_llm_id;
}

void TuiSettings::set_active_custom_llm_id(const std::string& id)
{
    active_custom_llm_id = id;
}

const std::vector<CustomLLM>& TuiSettings::get_custom_llms() const
{
    return custom_llms;
}

CustomLLM TuiSettings::find_custom_llm(const std::string& id) const
{
    const auto it = std::find_if(custom_llms.begin(), custom_llms.end(),
                                 [&id](const CustomLLM& item) { return item.id == id; });
    if (it != custom_llms.end()) {
        return *it;
    }
    return {};
}

std::string TuiSettings::upsert_custom_llm(const CustomLLM& llm)
{
    CustomLLM copy = llm;
    if (copy.id.empty()) {
        copy.id = generate_custom_llm_id();
    }
    const auto it = std::find_if(custom_llms.begin(), custom_llms.end(),
                                 [&copy](const CustomLLM& item) { return item.id == copy.id; });
    if (it != custom_llms.end()) {
        *it = copy;
    } else {
        custom_llms.push_back(copy);
    }
    return copy.id;
}

void TuiSettings::remove_custom_llm(const std::string& id)
{
    custom_llms.erase(std::remove_if(custom_llms.begin(),
                                     custom_llms.end(),
                                     [&id](const CustomLLM& item) { return item.id == id; }),
                      custom_llms.end());
    if (active_custom_llm_id == id) {
        active_custom_llm_id.clear();
    }
}


bool TuiSettings::is_llm_chosen() const {
    return llm_choice != LLMChoice::Unset;
}

std::string TuiSettings::get_category_language_string() const
{
    return category_language_str;
}

void TuiSettings::set_category_language_string(const std::string& lang)
{
    category_language_str = lang;
}


bool TuiSettings::get_use_subcategories() const
{
    return use_subcategories;
}


void TuiSettings::set_use_subcategories(bool value)
{
    use_subcategories = value;
}

bool TuiSettings::get_use_consistency_hints() const
{
    return use_consistency_hints;
}

void TuiSettings::set_use_consistency_hints(bool value)
{
    use_consistency_hints = value;
}


bool TuiSettings::get_categorize_files() const
{
    return categorize_files;
}


void TuiSettings::set_categorize_files(bool value)
{
    categorize_files = value;
}


bool TuiSettings::get_categorize_directories() const
{
    return categorize_directories;
}


void TuiSettings::set_categorize_directories(bool value)
{
    categorize_directories = value;
}


std::string TuiSettings::get_sort_folder() const
{
    return sort_folder;
}


void TuiSettings::set_sort_folder(const std::string &path)
{
    this->sort_folder = path;
}

bool TuiSettings::get_consistency_pass_enabled() const
{
    return consistency_pass_enabled;
}

void TuiSettings::set_consistency_pass_enabled(bool value)
{
    consistency_pass_enabled = value;
}

bool TuiSettings::get_development_prompt_logging() const
{
    return development_prompt_logging;
}

void TuiSettings::set_development_prompt_logging(bool value)
{
    development_prompt_logging = value;
}

bool TuiSettings::get_use_whitelist() const
{
    return use_whitelist;
}

void TuiSettings::set_use_whitelist(bool value)
{
    use_whitelist = value;
}

std::string TuiSettings::get_active_whitelist() const
{
    return active_whitelist;
}

void TuiSettings::set_active_whitelist(const std::string& name)
{
    active_whitelist = name;
}


void TuiSettings::set_skipped_version(const std::string &version) {
    skipped_version = version;
}


std::string TuiSettings::get_skipped_version()
{
    return skipped_version;
}

int TuiSettings::get_total_categorized_files() const
{
    return categorized_file_count;
}

void TuiSettings::add_categorized_files(int count)
{
    if (count <= 0) {
        return;
    }
    categorized_file_count += count;
}

int TuiSettings::get_next_support_prompt_threshold() const
{
    return next_support_prompt_threshold;
}

void TuiSettings::set_next_support_prompt_threshold(int threshold)
{
    if (threshold < 100) {
        threshold = 100;
    }
    next_support_prompt_threshold = threshold;
}

std::vector<std::string> TuiSettings::get_allowed_categories() const
{
    return allowed_categories;
}

void TuiSettings::set_allowed_categories(std::vector<std::string> values)
{
    allowed_categories = std::move(values);
}

std::vector<std::string> TuiSettings::get_allowed_subcategories() const
{
    return allowed_subcategories;
}

void TuiSettings::set_allowed_subcategories(std::vector<std::string> values)
{
    allowed_subcategories = std::move(values);
}
