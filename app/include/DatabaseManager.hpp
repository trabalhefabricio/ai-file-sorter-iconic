#ifndef DATABASEMANAGER_HPP
#define DATABASEMANAGER_HPP

#include "Types.hpp"
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <optional>
#include <sqlite3.h>

class DatabaseManager {
public:
    explicit DatabaseManager(std::string config_dir);
    ~DatabaseManager();

    void close();
    void initialize();
    std::string get_database_path() const { return db_file; }

    bool is_file_already_categorized(const std::string &file_name);
    struct ResolvedCategory {
        int taxonomy_id;
        std::string category;
        std::string subcategory;
    };

    ResolvedCategory resolve_category(const std::string& category,
                                      const std::string& subcategory);

    bool insert_or_update_file_with_categorization(const std::string& file_name,
                                                   const std::string& file_type,
                                                   const std::string& dir_path,
                                                   const ResolvedCategory& resolved,
                                                   bool used_consistency_hints,
                                                   bool user_provided = false);
    std::vector<std::string> get_dir_contents_from_db(const std::string &dir_path);
    bool remove_file_categorization(const std::string& dir_path,
                                    const std::string& file_name,
                                    const FileType file_type);
    std::vector<CategorizedFile> remove_empty_categorizations(const std::string& dir_path);

    std::vector<CategorizedFile> get_categorized_files(const std::string &directory_path);

    std::vector<std::string>
        get_categorization_from_db(const std::string& file_name, const FileType file_type);
    void increment_taxonomy_frequency(int taxonomy_id);
    std::vector<std::pair<std::string, std::string>>
        get_taxonomy_snapshot(std::size_t max_entries) const;
    std::vector<std::pair<std::string, std::string>>
        get_recent_categories_for_extension(const std::string& extension,
                                            FileType file_type,
                                            std::size_t limit) const;
    bool clear_directory_categorizations(const std::string& dir_path);
    std::optional<bool> get_directory_categorization_style(const std::string& dir_path) const;

    // User profile methods
    bool save_user_profile(const UserProfile& profile);
    UserProfile load_user_profile(const std::string& user_id);
    bool save_user_characteristic(const std::string& user_id,
                                  const UserCharacteristic& characteristic);
    std::vector<UserCharacteristic> load_user_characteristics(const std::string& user_id);
    bool save_folder_insight(const std::string& user_id, const FolderInsight& insight);
    std::vector<FolderInsight> load_folder_insights(const std::string& user_id);

    // Folder learning settings
    std::string get_folder_inclusion_level(const std::string& folder_path);
    void set_folder_inclusion_level(const std::string& folder_path, const std::string& level);
    
    // Organizational template methods
    bool save_organizational_template(const std::string& user_id,
                                     const OrganizationalTemplate& templ);
    std::vector<OrganizationalTemplate> load_organizational_templates(const std::string& user_id);
    
    // Phase 1.1: Enhanced feature methods
    
    // Confidence scoring methods
    struct ConfidenceScore {
        float category_confidence;
        float subcategory_confidence;
        std::string confidence_factors;  // JSON
        std::string model_version;
    };
    bool save_confidence_score(const std::string& file_name,
                              const std::string& file_type,
                              const std::string& dir_path,
                              const ConfidenceScore& score);
    std::optional<ConfidenceScore> get_confidence_score(const std::string& file_name,
                                                        const std::string& file_type,
                                                        const std::string& dir_path);
    
    // Content analysis methods
    struct ContentAnalysis {
        std::string content_hash;
        std::string mime_type;
        std::string keywords;  // JSON array
        std::string detected_language;
        std::string metadata;  // JSON
        std::string analysis_summary;
    };
    bool save_content_analysis(const std::string& file_path,
                              const ContentAnalysis& analysis);
    std::optional<ContentAnalysis> get_content_analysis(const std::string& file_path);
    std::optional<ContentAnalysis> get_content_analysis_by_hash(const std::string& content_hash);
    
    // API usage tracking methods
    struct APIUsage {
        std::string provider;
        std::string date;
        int tokens_used;
        int requests_made;
        float cost_estimate;
        int daily_limit;
        int remaining;
    };
    bool record_api_usage(const std::string& provider,
                         int tokens,
                         int requests = 1,
                         float cost = 0.0f);
    std::optional<APIUsage> get_api_usage_today(const std::string& provider);
    std::vector<APIUsage> get_api_usage_history(const std::string& provider,
                                                int days = 30);
    
    // Multiple profiles methods
    struct UserProfileInfo {
        int profile_id;
        std::string profile_name;
        bool is_active;
        std::string created_at;
        std::string last_used;
    };
    int create_user_profile(const std::string& profile_name);
    bool set_active_profile(int profile_id);
    std::optional<UserProfileInfo> get_active_profile();
    std::vector<UserProfileInfo> get_all_profiles();
    bool delete_profile(int profile_id);
    
    // User corrections methods
    struct UserCorrection {
        std::string file_path;
        std::string file_name;
        std::string original_category;
        std::string original_subcategory;
        std::string corrected_category;
        std::string corrected_subcategory;
        std::string file_extension;
        std::string timestamp;
    };
    bool record_correction(const UserCorrection& correction, int profile_id = -1);
    std::vector<UserCorrection> get_corrections(int profile_id = -1,
                                               int limit = 100);
    std::map<std::string, int> get_correction_patterns();
    
    // Session management methods
    struct SessionInfo {
        std::string session_id;
        std::string folder_path;
        std::string started_at;
        std::string completed_at;
        std::string consistency_mode;
        float consistency_strength;
        int files_processed;
    };
    bool create_session(const std::string& session_id,
                       const std::string& folder_path,
                       const std::string& consistency_mode,
                       float consistency_strength);
    bool complete_session(const std::string& session_id, int files_processed);
    std::optional<SessionInfo> get_session(const std::string& session_id);
    std::vector<SessionInfo> get_recent_sessions(int limit = 10);
    
    // Undo history methods
    bool record_undo_plan(const std::string& plan_path,
                         const std::string& description);
    bool mark_plan_undone(int undo_id);
    std::vector<std::pair<int, std::string>> get_undo_history(int limit = 20);
    
    // File Tinder methods
    struct FileTinderDecision {
        std::string folder_path;
        std::string file_path;
        std::string decision;  // keep, delete, ignore, pending
        std::string timestamp;
    };
    bool save_tinder_decision(const FileTinderDecision& decision);
    std::vector<FileTinderDecision> get_tinder_decisions(const std::string& folder_path);
    bool clear_tinder_session(const std::string& folder_path);

private:
    struct TaxonomyEntry {
        int id;
        std::string category;
        std::string subcategory;
        std::string normalized_category;
        std::string normalized_subcategory;
    };

    void initialize_schema();
    void initialize_taxonomy_schema();
    void initialize_user_profile_schema();
    void load_taxonomy_cache();
    std::string normalize_label(const std::string& input) const;
    static double string_similarity(const std::string& a, const std::string& b);
    static std::string make_key(const std::string& norm_category,
                                const std::string& norm_subcategory);
    std::pair<int, double> find_fuzzy_match(const std::string& norm_category,
                                            const std::string& norm_subcategory) const;
    int resolve_existing_taxonomy(const std::string& key,
                                   const std::string& norm_category,
                                   const std::string& norm_subcategory) const;
    ResolvedCategory build_resolved_category(int taxonomy_id,
                                             const std::string& fallback_category,
                                             const std::string& fallback_subcategory,
                                             const std::string& norm_category,
                                             const std::string& norm_subcategory);
    int create_taxonomy_entry(const std::string& category,
                              const std::string& subcategory,
                              const std::string& norm_category,
                              const std::string& norm_subcategory);
    int find_existing_taxonomy_id(const std::string& norm_category,
                                  const std::string& norm_subcategory) const;
    void ensure_alias_mapping(int taxonomy_id,
                              const std::string& norm_category,
                              const std::string& norm_subcategory);
    const TaxonomyEntry* find_taxonomy_entry(int taxonomy_id) const;

    std::map<std::string, std::string> cached_results;
    std::string get_cached_category(const std::string &file_name);
    void load_cache();
    bool file_exists_in_db(const std::string &file_name, const std::string &file_path);

    sqlite3* db;
    const std::string config_dir;
    const std::string db_file;
    std::vector<TaxonomyEntry> taxonomy_entries;
    std::unordered_map<std::string, int> canonical_lookup;
    std::unordered_map<std::string, int> alias_lookup;
    std::unordered_map<int, size_t> taxonomy_index;

    static bool is_duplicate_category(
        const std::vector<std::pair<std::string, std::string>>& results,
        const std::pair<std::string, std::string>& candidate);
    std::optional<std::pair<std::string, std::string>> build_recent_category_candidate(
        const char* file_name_text,
        const char* category_text,
        const char* subcategory_text,
        const std::string& normalized_extension,
        bool has_extension) const;
};

#endif
