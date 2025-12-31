#include "DatabaseManager.hpp"
#include "Types.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace {
constexpr double kSimilarityThreshold = 0.85;

template <typename... Args>
void db_log(spdlog::level::level_enum level, const char* fmt, Args&&... args) {
    auto message = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->log(level, "{}", message);
    } else {
        std::fprintf(stderr, "%s\n", message.c_str());
    }
}

bool is_duplicate_column_error(const char *error_msg) {
    if (!error_msg) {
        return false;
    }
    std::string message(error_msg);
    std::transform(message.begin(), message.end(), message.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return message.find("duplicate column name") != std::string::npos;
}

std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string extract_extension_lower(const std::string& file_name) {
    const auto pos = file_name.find_last_of('.');
    if (pos == std::string::npos || pos + 1 >= file_name.size()) {
        return std::string();
    }
    std::string ext = file_name.substr(pos);
    return to_lower_copy(ext);
}

struct StatementDeleter {
    void operator()(sqlite3_stmt* stmt) const {
        if (stmt) {
            sqlite3_finalize(stmt);
        }
    }
};

using StatementPtr = std::unique_ptr<sqlite3_stmt, StatementDeleter>;

StatementPtr prepare_statement(sqlite3* db, const char* sql) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
        return StatementPtr{};
    }
    return StatementPtr(raw);
}

std::string trim_copy(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool has_label_content(const std::string& value) {
    return !trim_copy(value).empty();
}

std::optional<CategorizedFile> build_categorized_entry(sqlite3_stmt* stmt) {
    const char *file_dir_path = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    const char *file_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const char *file_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    const char *category = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    const char *subcategory = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

    std::string dir_path = file_dir_path ? file_dir_path : "";
    std::string name = file_name ? file_name : "";
    std::string type_str = file_type ? file_type : "";
    std::string cat = category ? category : "";
    std::string subcat = subcategory ? subcategory : "";

    if (!has_label_content(cat) || !has_label_content(subcat)) {
        return std::nullopt;
    }

    int taxonomy_id = 0;
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
        taxonomy_id = sqlite3_column_int(stmt, 5);
    }
    bool used_consistency = false;
    if (sqlite3_column_count(stmt) > 6 && sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
        used_consistency = sqlite3_column_int(stmt, 6) != 0;
    }

    FileType file_type_enum = (type_str == "F") ? FileType::File : FileType::Directory;
    CategorizedFile entry{dir_path, name, file_type_enum, cat, subcat, taxonomy_id};
    entry.from_cache = true;
    entry.used_consistency_hints = used_consistency;
    return entry;
}

} // namespace

DatabaseManager::DatabaseManager(std::string config_dir)
    : db(nullptr),
      config_dir(std::move(config_dir)),
      db_file(this->config_dir + "/" +
              (std::getenv("CATEGORIZATION_CACHE_FILE")
                   ? std::getenv("CATEGORIZATION_CACHE_FILE")
                   : "categorization_results.db")) {
    if (db_file.empty()) {
        db_log(spdlog::level::err, "Error: Database path is empty");
        return;
    }

    if (sqlite3_open(db_file.c_str(), &db) != SQLITE_OK) {
        db_log(spdlog::level::err, "Can't open database: {}", sqlite3_errmsg(db));
        db = nullptr;
        return;
    }

    sqlite3_extended_result_codes(db, 1);

    initialize_schema();
    initialize_taxonomy_schema();
    initialize_user_profile_schema();
    load_taxonomy_cache();
}

DatabaseManager::~DatabaseManager() {
    close();
}

void DatabaseManager::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

void DatabaseManager::initialize() {
    if (db) {
        close();
    }
    
    if (db_file.empty()) {
        db_log(spdlog::level::err, "Error: Database path is empty");
        return;
    }

    if (sqlite3_open(db_file.c_str(), &db) != SQLITE_OK) {
        db_log(spdlog::level::err, "Can't open database: {}", sqlite3_errmsg(db));
        db = nullptr;
        return;
    }

    sqlite3_extended_result_codes(db, 1);

    initialize_schema();
    initialize_taxonomy_schema();
    load_taxonomy_cache();
}

void DatabaseManager::initialize_schema() {
    if (!db) return;

    const char *create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS file_categorization (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_name TEXT NOT NULL,
            file_type TEXT NOT NULL,
            dir_path TEXT NOT NULL,
            category TEXT NOT NULL,
            subcategory TEXT,
            taxonomy_id INTEGER,
            categorization_style INTEGER DEFAULT 0,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(file_name, file_type, dir_path)
        );
    )";

    char *error_msg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create file_categorization table: {}", error_msg);
        sqlite3_free(error_msg);
    }

    const char *add_column_sql = "ALTER TABLE file_categorization ADD COLUMN taxonomy_id INTEGER;";
    if (sqlite3_exec(db, add_column_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (!is_duplicate_column_error(error_msg)) {
            db_log(spdlog::level::warn, "Failed to add taxonomy_id column: {}", error_msg ? error_msg : "");
        }
        if (error_msg) {
            sqlite3_free(error_msg);
        }
    }

    const char *add_style_column_sql =
        "ALTER TABLE file_categorization ADD COLUMN categorization_style INTEGER DEFAULT 0;";
    if (sqlite3_exec(db, add_style_column_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (!is_duplicate_column_error(error_msg)) {
            db_log(spdlog::level::warn, "Failed to add categorization_style column: {}", error_msg ? error_msg : "");
        }
        if (error_msg) {
            sqlite3_free(error_msg);
        }
    }

    const char *add_user_provided_column_sql =
        "ALTER TABLE file_categorization ADD COLUMN user_provided INTEGER DEFAULT 0;";
    if (sqlite3_exec(db, add_user_provided_column_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (!is_duplicate_column_error(error_msg)) {
            db_log(spdlog::level::warn, "Failed to add user_provided column: {}", error_msg ? error_msg : "");
        }
        if (error_msg) {
            sqlite3_free(error_msg);
        }
    }

    const char *create_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_file_categorization_taxonomy ON file_categorization(taxonomy_id);";
    if (sqlite3_exec(db, create_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create taxonomy index: {}", error_msg);
        sqlite3_free(error_msg);
    }
}

void DatabaseManager::initialize_taxonomy_schema() {
    if (!db) return;

    const char *taxonomy_sql = R"(
        CREATE TABLE IF NOT EXISTS category_taxonomy (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            canonical_category TEXT NOT NULL,
            canonical_subcategory TEXT NOT NULL,
            normalized_category TEXT NOT NULL,
            normalized_subcategory TEXT NOT NULL,
            frequency INTEGER DEFAULT 0,
            UNIQUE(normalized_category, normalized_subcategory)
        );
    )";

    char *error_msg = nullptr;
    if (sqlite3_exec(db, taxonomy_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create category_taxonomy table: {}", error_msg);
        sqlite3_free(error_msg);
    }

    const char *alias_sql = R"(
        CREATE TABLE IF NOT EXISTS category_alias (
            alias_category_norm TEXT NOT NULL,
            alias_subcategory_norm TEXT NOT NULL,
            taxonomy_id INTEGER NOT NULL,
            PRIMARY KEY(alias_category_norm, alias_subcategory_norm),
            FOREIGN KEY(taxonomy_id) REFERENCES category_taxonomy(id)
        );
    )";
    if (sqlite3_exec(db, alias_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create category_alias table: {}", error_msg);
        sqlite3_free(error_msg);
    }

    const char *alias_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_category_alias_taxonomy ON category_alias(taxonomy_id);";
    if (sqlite3_exec(db, alias_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create alias index: {}", error_msg);
        sqlite3_free(error_msg);
    }
}

void DatabaseManager::load_taxonomy_cache() {
    taxonomy_entries.clear();
    canonical_lookup.clear();
    alias_lookup.clear();
    taxonomy_index.clear();

    if (!db) return;

    sqlite3_stmt *stmt = nullptr;
    const char *select_taxonomy =
        "SELECT id, canonical_category, canonical_subcategory, "
        "normalized_category, normalized_subcategory, frequency FROM category_taxonomy;";

    if (sqlite3_prepare_v2(db, select_taxonomy, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TaxonomyEntry entry;
            entry.id = sqlite3_column_int(stmt, 0);
            entry.category = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            entry.subcategory = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            entry.normalized_category = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            entry.normalized_subcategory = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

            taxonomy_index[entry.id] = taxonomy_entries.size();
            taxonomy_entries.push_back(entry);
            canonical_lookup[make_key(entry.normalized_category, entry.normalized_subcategory)] = entry.id;
        }
    } else {
        db_log(spdlog::level::err, "Failed to load taxonomy cache: {}", sqlite3_errmsg(db));
    }
    if (stmt) sqlite3_finalize(stmt);

    const char *select_alias =
        "SELECT alias_category_norm, alias_subcategory_norm, taxonomy_id FROM category_alias;";
    if (sqlite3_prepare_v2(db, select_alias, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string alias_cat = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::string alias_subcat = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            int taxonomy_id = sqlite3_column_int(stmt, 2);

            alias_lookup[make_key(alias_cat, alias_subcat)] = taxonomy_id;
        }
    } else {
        db_log(spdlog::level::err, "Failed to load category aliases: {}", sqlite3_errmsg(db));
    }
    if (stmt) sqlite3_finalize(stmt);
}

std::string DatabaseManager::normalize_label(const std::string &input) const {
    std::string result;
    result.reserve(input.size());

    bool last_was_space = true;
    for (unsigned char ch : input) {
        if (std::isalnum(ch)) {
            result.push_back(static_cast<char>(std::tolower(ch)));
            last_was_space = false;
        } else if (std::isspace(ch)) {
            if (!last_was_space) {
                result.push_back(' ');
                last_was_space = true;
            }
        }
    }

    // Trim leading/trailing space if any
    while (!result.empty() && result.front() == ' ') {
        result.erase(result.begin());
    }
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

double DatabaseManager::string_similarity(const std::string &a, const std::string &b) {
    if (a == b) {
        return 1.0;
    }
    if (a.empty() || b.empty()) {
        return 0.0;
    }

    const size_t m = a.size();
    const size_t n = b.size();
    std::vector<size_t> prev(n + 1), curr(n + 1);

    for (size_t j = 0; j <= n; ++j) {
        prev[j] = j;
    }

    for (size_t i = 1; i <= m; ++i) {
        curr[0] = i;
        for (size_t j = 1; j <= n; ++j) {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }

    const double dist = static_cast<double>(prev[n]);
    const double max_len = static_cast<double>(std::max(m, n));
    return 1.0 - (dist / max_len);
}

std::string DatabaseManager::make_key(const std::string &norm_category,
                                      const std::string &norm_subcategory) {
    return norm_category + "::" + norm_subcategory;
}

int DatabaseManager::create_taxonomy_entry(const std::string &category,
                                           const std::string &subcategory,
                                           const std::string &norm_category,
                                           const std::string &norm_subcategory) {
    if (!db) return -1;

    const char *sql = R"(
        INSERT INTO category_taxonomy
            (canonical_category, canonical_subcategory, normalized_category, normalized_subcategory, frequency)
        VALUES (?, ?, ?, ?, 0);
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare taxonomy insert: {}", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, subcategory.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, norm_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, norm_subcategory.c_str(), -1, SQLITE_TRANSIENT);

    int step_rc = sqlite3_step(stmt);
    int extended_rc = sqlite3_extended_errcode(db);
    sqlite3_finalize(stmt);

    if (step_rc != SQLITE_DONE) {
        if (extended_rc == SQLITE_CONSTRAINT_UNIQUE ||
            extended_rc == SQLITE_CONSTRAINT_PRIMARYKEY ||
            extended_rc == SQLITE_CONSTRAINT) {
            return find_existing_taxonomy_id(norm_category, norm_subcategory);
        }

        db_log(spdlog::level::err, "Failed to insert taxonomy entry: {}", sqlite3_errmsg(db));
        return -1;
    }

    int new_id = static_cast<int>(sqlite3_last_insert_rowid(db));
    TaxonomyEntry entry{new_id, category, subcategory, norm_category, norm_subcategory};
    taxonomy_index[new_id] = taxonomy_entries.size();
    taxonomy_entries.push_back(entry);
    canonical_lookup[make_key(norm_category, norm_subcategory)] = new_id;
    return new_id;
}

int DatabaseManager::find_existing_taxonomy_id(const std::string &norm_category,
                                               const std::string &norm_subcategory) const {
    if (!db) return -1;

    const char *select_sql =
        "SELECT id FROM category_taxonomy WHERE normalized_category = ? AND normalized_subcategory = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    int existing_id = -1;

    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, norm_category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, norm_subcategory.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            existing_id = sqlite3_column_int(stmt, 0);
        }
    }

    if (stmt) {
        sqlite3_finalize(stmt);
    }
    return existing_id;
}

void DatabaseManager::ensure_alias_mapping(int taxonomy_id,
                                           const std::string &norm_category,
                                           const std::string &norm_subcategory) {
    if (!db) return;

    std::string key = make_key(norm_category, norm_subcategory);

    auto canonical_it = canonical_lookup.find(key);
    if (canonical_it != canonical_lookup.end() && canonical_it->second == taxonomy_id) {
        return; // Already canonical form
    }

    if (alias_lookup.find(key) != alias_lookup.end()) {
        return;
    }

    const char *sql = R"(
        INSERT OR IGNORE INTO category_alias (alias_category_norm, alias_subcategory_norm, taxonomy_id)
        VALUES (?, ?, ?);
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare alias insert: {}", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, norm_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, norm_subcategory.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, taxonomy_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        db_log(spdlog::level::err, "Failed to insert alias: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);
    alias_lookup[key] = taxonomy_id;
}

const DatabaseManager::TaxonomyEntry *DatabaseManager::find_taxonomy_entry(int taxonomy_id) const {
    auto it = taxonomy_index.find(taxonomy_id);
    if (it == taxonomy_index.end()) {
        return nullptr;
    }
    size_t idx = it->second;
    if (idx >= taxonomy_entries.size()) {
        return nullptr;
    }
    return &taxonomy_entries[idx];
}

std::pair<int, double> DatabaseManager::find_fuzzy_match(
    const std::string& norm_category,
    const std::string& norm_subcategory) const {
    if (taxonomy_entries.empty()) {
        return {-1, 0.0};
    }

    double best_score = 0.0;
    int best_id = -1;
    for (const auto &entry : taxonomy_entries) {
        double category_score = string_similarity(norm_category, entry.normalized_category);
        double subcategory_score =
            string_similarity(norm_subcategory, entry.normalized_subcategory);
        double combined = (category_score + subcategory_score) / 2.0;
        if (combined > best_score) {
            best_score = combined;
            best_id = entry.id;
        }
    }

    if (best_id != -1 && best_score >= kSimilarityThreshold) {
        return {best_id, best_score};
    }
    return {-1, best_score};
}

int DatabaseManager::resolve_existing_taxonomy(const std::string& key,
                                               const std::string& norm_category,
                                               const std::string& norm_subcategory) const {
    auto alias_it = alias_lookup.find(key);
    if (alias_it != alias_lookup.end()) {
        return alias_it->second;
    }

    auto canonical_it = canonical_lookup.find(key);
    if (canonical_it != canonical_lookup.end()) {
        return canonical_it->second;
    }

    auto [best_id, score] = find_fuzzy_match(norm_category, norm_subcategory);
    return best_id;
}

DatabaseManager::ResolvedCategory DatabaseManager::build_resolved_category(
    int taxonomy_id,
    const std::string& fallback_category,
    const std::string& fallback_subcategory,
    const std::string& norm_category,
    const std::string& norm_subcategory) {

    ResolvedCategory result{-1, fallback_category, fallback_subcategory};

    if (taxonomy_id == -1) {
        taxonomy_id = create_taxonomy_entry(fallback_category, fallback_subcategory,
                                            norm_category, norm_subcategory);
    }

    if (taxonomy_id != -1) {
        ensure_alias_mapping(taxonomy_id, norm_category, norm_subcategory);
        if (const auto *entry = find_taxonomy_entry(taxonomy_id)) {
            result.taxonomy_id = entry->id;
            result.category = entry->category;
            result.subcategory = entry->subcategory;
        } else {
            result.taxonomy_id = taxonomy_id;
        }
    } else {
        result.category = fallback_category;
        result.subcategory = fallback_subcategory;
    }

    return result;
}

DatabaseManager::ResolvedCategory
DatabaseManager::resolve_category(const std::string &category,
                                  const std::string &subcategory) {
    ResolvedCategory result{-1, category, subcategory};
    if (!db) {
        return result;
    }

    auto trim_copy = [](std::string value) {
        auto is_space = [](unsigned char ch) { return std::isspace(ch); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                                [&](unsigned char ch) { return !is_space(ch); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
                                 [&](unsigned char ch) { return !is_space(ch); }).base(),
                    value.end());
        return value;
    };

    std::string trimmed_category = trim_copy(category);
    std::string trimmed_subcategory = trim_copy(subcategory);

    if (trimmed_category.empty()) {
        trimmed_category = "Uncategorized";
    }
    if (trimmed_subcategory.empty()) {
        trimmed_subcategory = "General";
    }

    std::string norm_category = normalize_label(trimmed_category);
    std::string norm_subcategory = normalize_label(trimmed_subcategory);
    std::string key = make_key(norm_category, norm_subcategory);

    int taxonomy_id = resolve_existing_taxonomy(key, norm_category, norm_subcategory);
    return build_resolved_category(taxonomy_id,
                                   trimmed_category,
                                   trimmed_subcategory,
                                   norm_category,
                                   norm_subcategory);
}

bool DatabaseManager::insert_or_update_file_with_categorization(
    const std::string &file_name,
    const std::string &file_type,
    const std::string &dir_path,
    const ResolvedCategory &resolved,
    bool used_consistency_hints,
    bool user_provided) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO file_categorization
            (file_name, file_type, dir_path, category, subcategory, taxonomy_id, categorization_style, user_provided)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(file_name, file_type, dir_path)
        DO UPDATE SET
            category = excluded.category,
            subcategory = excluded.subcategory,
            taxonomy_id = excluded.taxonomy_id,
            categorization_style = excluded.categorization_style,
            user_provided = excluded.user_provided;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "SQL prepare error: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, file_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, file_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, dir_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, resolved.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, resolved.subcategory.c_str(), -1, SQLITE_TRANSIENT);

    if (resolved.taxonomy_id > 0) {
        sqlite3_bind_int(stmt, 6, resolved.taxonomy_id);
    } else {
        sqlite3_bind_null(stmt, 6);
    }
    sqlite3_bind_int(stmt, 7, used_consistency_hints ? 1 : 0);
    sqlite3_bind_int(stmt, 8, user_provided ? 1 : 0);

    bool success = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        db_log(spdlog::level::err, "SQL error during insert/update: {}", sqlite3_errmsg(db));
        success = false;
    }

    sqlite3_finalize(stmt);

    if (success && resolved.taxonomy_id > 0) {
        increment_taxonomy_frequency(resolved.taxonomy_id);
    }

    return success;
}

bool DatabaseManager::remove_file_categorization(const std::string& dir_path,
                                                 const std::string& file_name,
                                                 const FileType file_type) {
    if (!db) {
        return false;
    }

    const char* sql =
        "DELETE FROM file_categorization WHERE dir_path = ? AND file_name = ? AND file_type = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare delete categorization statement: {}", sqlite3_errmsg(db));
        return false;
    }

    const std::string type_str = (file_type == FileType::File) ? "F" : "D";

    sqlite3_bind_text(stmt, 1, dir_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, file_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, type_str.c_str(), -1, SQLITE_TRANSIENT);

    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    if (!success) {
        db_log(spdlog::level::err, "Failed to delete cached categorization for '{}': {}", file_name, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::clear_directory_categorizations(const std::string& dir_path) {
    if (!db) {
        return false;
    }

    const char* sql = "DELETE FROM file_categorization WHERE dir_path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare directory cache clear statement: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, dir_path.c_str(), -1, SQLITE_TRANSIENT);
    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    if (!success) {
        db_log(spdlog::level::err, "Failed to clear cached categorizations for '{}': {}", dir_path, sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    cached_results.clear();
    return success;
}

std::optional<bool> DatabaseManager::get_directory_categorization_style(const std::string& dir_path) const {
    if (!db) {
        return std::nullopt;
    }

    const char* sql =
        "SELECT categorization_style FROM file_categorization WHERE dir_path = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn, "Failed to prepare cached style query: {}", sqlite3_errmsg(db));
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, dir_path.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<bool> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // If the column exists but is NULL (older rows), treat as "false" (refined) to compare
        // against the user's current preference.
        result = (sqlite3_column_type(stmt, 0) != SQLITE_NULL)
                     ? (sqlite3_column_int(stmt, 0) != 0)
                     : false;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<CategorizedFile>
DatabaseManager::remove_empty_categorizations(const std::string& dir_path) {
    std::vector<CategorizedFile> removed;
    if (!db) {
        return removed;
    }

    const char* sql = R"(
        SELECT file_name, file_type, IFNULL(category, ''), IFNULL(subcategory, ''), taxonomy_id
        FROM file_categorization
        WHERE dir_path = ?
          AND (category IS NULL OR TRIM(category) = '' OR subcategory IS NULL OR TRIM(subcategory) = '');
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare empty categorization query: {}", sqlite3_errmsg(db));
        return removed;
    }

    if (sqlite3_bind_text(stmt, 1, dir_path.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to bind directory path for empty categorization query: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return removed;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* subcategory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        std::string file_name = name ? name : "";
        std::string type_str = type ? type : "";
        FileType entry_type = (type_str == "D") ? FileType::Directory : FileType::File;

        int taxonomy_id = 0;
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
            taxonomy_id = sqlite3_column_int(stmt, 4);
        }

        removed.push_back({dir_path,
                           file_name,
                           entry_type,
                           category ? category : "",
                           subcategory ? subcategory : "",
                           taxonomy_id});
    }

    sqlite3_finalize(stmt);
    for (const auto& entry : removed) {
        remove_file_categorization(entry.file_path, entry.file_name, entry.type);
    }
    return removed;
}

void DatabaseManager::increment_taxonomy_frequency(int taxonomy_id) {
    if (!db || taxonomy_id <= 0) return;

    const char *sql =
        "UPDATE category_taxonomy "
        "SET frequency = (SELECT COUNT(*) FROM file_categorization WHERE taxonomy_id = ?) "
        "WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare frequency update: {}", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, taxonomy_id);
    sqlite3_bind_int(stmt, 2, taxonomy_id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        db_log(spdlog::level::err, "Failed to increment taxonomy frequency: {}", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

std::vector<CategorizedFile>
DatabaseManager::get_categorized_files(const std::string &directory_path) {
    std::vector<CategorizedFile> categorized_files;
    if (!db) return categorized_files;

    const char *sql =
        "SELECT dir_path, file_name, file_type, category, subcategory, taxonomy_id, categorization_style "
        "FROM file_categorization WHERE dir_path = ?;";
    StatementPtr stmt = prepare_statement(db, sql);
    if (!stmt) {
        return categorized_files;
    }

    if (sqlite3_bind_text(stmt.get(), 1, directory_path.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to bind directory_path: {}", sqlite3_errmsg(db));
        return categorized_files;
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        if (auto entry = build_categorized_entry(stmt.get())) {
            categorized_files.push_back(std::move(*entry));
        }
    }

    return categorized_files;
}

std::vector<std::string>
DatabaseManager::get_categorization_from_db(const std::string &file_name, const FileType file_type) {
    std::vector<std::string> categorization;
    if (!db) return categorization;

    const char *sql =
        "SELECT category, subcategory FROM file_categorization WHERE file_name = ? AND file_type = ?;";
    sqlite3_stmt *stmtcat = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmtcat, nullptr) != SQLITE_OK) {
        return categorization;
    }

    if (sqlite3_bind_text(stmtcat, 1, file_name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmtcat);
        return categorization;
    }

    std::string file_type_str = (file_type == FileType::File) ? "F" : "D";
    if (sqlite3_bind_text(stmtcat, 2, file_type_str.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmtcat);
        return categorization;
    }

    if (sqlite3_step(stmtcat) == SQLITE_ROW) {
        const char *category = reinterpret_cast<const char *>(sqlite3_column_text(stmtcat, 0));
        const char *subcategory = reinterpret_cast<const char *>(sqlite3_column_text(stmtcat, 1));
        categorization.emplace_back(category ? category : "");
        categorization.emplace_back(subcategory ? subcategory : "");
    }

    sqlite3_finalize(stmtcat);
    return categorization;
}

bool DatabaseManager::is_file_already_categorized(const std::string &file_name) {
    if (!db) return false;

    const char *sql = "SELECT 1 FROM file_categorization WHERE file_name = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, file_name.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

std::vector<std::string> DatabaseManager::get_dir_contents_from_db(const std::string &dir_path) {
    std::vector<std::string> results;
    if (!db) return results;

    const char *sql = "SELECT file_name FROM file_categorization WHERE dir_path = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    sqlite3_bind_text(stmt, 1, dir_path.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        results.emplace_back(name ? name : "");
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::pair<std::string, std::string>> DatabaseManager::get_taxonomy_snapshot(std::size_t max_entries) const
{
    std::vector<std::pair<std::string, std::string>> snapshot;
    if (max_entries == 0) {
        max_entries = taxonomy_entries.size();
    }
    snapshot.reserve(std::min(max_entries, taxonomy_entries.size()));
    for (const auto& entry : taxonomy_entries) {
        if (snapshot.size() >= max_entries) {
            break;
        }
        snapshot.emplace_back(entry.category, entry.subcategory);
    }
    return snapshot;
}

bool DatabaseManager::is_duplicate_category(
    const std::vector<std::pair<std::string, std::string>>& results,
    const std::pair<std::string, std::string>& candidate)
{
    return std::any_of(results.begin(), results.end(), [&candidate](const auto& existing) {
        return existing.first == candidate.first && existing.second == candidate.second;
    });
}

std::optional<std::pair<std::string, std::string>> DatabaseManager::build_recent_category_candidate(
    const char* file_name_text,
    const char* category_text,
    const char* subcategory_text,
    const std::string& normalized_extension,
    bool has_extension) const
{
    std::string file_name = file_name_text ? file_name_text : "";
    if (file_name.empty()) {
        return std::nullopt;
    }

    const std::string candidate_extension = extract_extension_lower(file_name);
    if (has_extension) {
        if (candidate_extension != normalized_extension) {
            return std::nullopt;
        }
    } else if (!candidate_extension.empty()) {
        return std::nullopt;
    }

    std::string category = category_text ? category_text : "";
    if (category.empty()) {
        return std::nullopt;
    }

    std::string subcategory = subcategory_text ? subcategory_text : "";
    return std::make_pair(std::move(category), std::move(subcategory));
}

std::vector<std::pair<std::string, std::string>>
DatabaseManager::get_recent_categories_for_extension(const std::string& extension,
                                                     FileType file_type,
                                                     std::size_t limit) const
{
    std::vector<std::pair<std::string, std::string>> results;
    if (!db || limit == 0) {
        return results;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT file_name, category, subcategory FROM file_categorization "
        "WHERE file_type = ? ORDER BY timestamp DESC LIMIT ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn,
               "Failed to prepare recent category lookup: {}",
               sqlite3_errmsg(db));
        return results;
    }

    const std::string type_code(1, file_type == FileType::File ? 'F' : 'D');
    sqlite3_bind_text(stmt, 1, type_code.c_str(), -1, SQLITE_TRANSIENT);
    const std::size_t fetch_limit = std::max<std::size_t>(limit * 5, limit);
    sqlite3_bind_int(stmt, 2, static_cast<int>(fetch_limit));

    const std::string normalized_extension = to_lower_copy(extension);
    const bool has_extension = !normalized_extension.empty();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* file_name_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* category_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* subcategory_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        const auto candidate = build_recent_category_candidate(file_name_text,
                                                               category_text,
                                                               subcategory_text,
                                                               normalized_extension,
                                                               has_extension);
        if (!candidate.has_value()) {
            continue;
        }
        if (is_duplicate_category(results, *candidate)) {
            continue;
        }

        results.push_back(*candidate);
        if (results.size() >= limit) {
            break;
        }
    }

    sqlite3_finalize(stmt);
    return results;
}

std::string DatabaseManager::get_cached_category(const std::string &file_name) {
    auto iter = cached_results.find(file_name);
    if (iter != cached_results.end()) {
        return iter->second;
    }
    return {};
}

void DatabaseManager::load_cache() {
    cached_results.clear();
}

bool DatabaseManager::file_exists_in_db(const std::string &file_name, const std::string &file_path) {
    if (!db) return false;

    const char *sql =
        "SELECT 1 FROM file_categorization WHERE file_name = ? AND dir_path = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, file_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, file_path.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

void DatabaseManager::initialize_user_profile_schema() {
    if (!db) return;

    // User profile table
    const char *user_profile_sql = R"(
        CREATE TABLE IF NOT EXISTS user_profile (
            user_id TEXT PRIMARY KEY,
            created_at TEXT NOT NULL,
            last_updated TEXT NOT NULL
        );
    )";

    char *error_msg = nullptr;
    if (sqlite3_exec(db, user_profile_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create user_profile table: {}", error_msg);
        sqlite3_free(error_msg);
        return;
    }

    // User characteristics table
    const char *characteristics_sql = R"(
        CREATE TABLE IF NOT EXISTS user_characteristics (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT NOT NULL,
            trait_name TEXT NOT NULL,
            value TEXT NOT NULL,
            confidence REAL NOT NULL,
            evidence TEXT,
            timestamp TEXT NOT NULL,
            FOREIGN KEY(user_id) REFERENCES user_profile(user_id),
            UNIQUE(user_id, trait_name, value)
        );
    )";

    if (sqlite3_exec(db, characteristics_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create user_characteristics table: {}", error_msg);
        sqlite3_free(error_msg);
        return;
    }

    // Folder insights table
    const char *folder_insights_sql = R"(
        CREATE TABLE IF NOT EXISTS folder_insights (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT NOT NULL,
            folder_path TEXT NOT NULL,
            description TEXT,
            dominant_categories TEXT,
            file_count INTEGER,
            last_analyzed TEXT NOT NULL,
            usage_pattern TEXT,
            FOREIGN KEY(user_id) REFERENCES user_profile(user_id),
            UNIQUE(user_id, folder_path)
        );
    )";

    if (sqlite3_exec(db, folder_insights_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create folder_insights table: {}", error_msg);
        sqlite3_free(error_msg);
        return;
    }

    // Create indices for better performance
    const char *create_chars_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_user_characteristics_user ON user_characteristics(user_id);";
    if (sqlite3_exec(db, create_chars_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create characteristics index: {}", error_msg);
        sqlite3_free(error_msg);
    }

    const char *create_insights_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_folder_insights_user ON folder_insights(user_id);";
    if (sqlite3_exec(db, create_insights_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create insights index: {}", error_msg);
        sqlite3_free(error_msg);
    }

    // Folder learning settings table for per-folder exclusions
    const char *folder_learning_sql = R"(
        CREATE TABLE IF NOT EXISTS folder_learning_settings (
            folder_path TEXT PRIMARY KEY,
            inclusion_level TEXT NOT NULL DEFAULT 'full',
            CHECK(inclusion_level IN ('none', 'partial', 'full'))
        );
    )";
    if (sqlite3_exec(db, folder_learning_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create folder_learning_settings table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Organizational templates table
    const char *templates_sql = R"(
        CREATE TABLE IF NOT EXISTS organizational_templates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT NOT NULL,
            template_name TEXT NOT NULL,
            description TEXT,
            suggested_categories TEXT,
            suggested_subcategories TEXT,
            confidence REAL NOT NULL,
            based_on_folders TEXT,
            usage_count INTEGER DEFAULT 1,
            FOREIGN KEY(user_id) REFERENCES user_profile(user_id),
            UNIQUE(user_id, template_name)
        );
    )";
    if (sqlite3_exec(db, templates_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create organizational_templates table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    const char *create_templates_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_organizational_templates_user ON organizational_templates(user_id);";
    if (sqlite3_exec(db, create_templates_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create templates index: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Phase 1.1: Enhanced schema for new features
    
    // Confidence scores table
    const char *confidence_scores_sql = R"(
        CREATE TABLE IF NOT EXISTS confidence_scores (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_name TEXT NOT NULL,
            file_type TEXT NOT NULL,
            dir_path TEXT NOT NULL,
            category_confidence REAL NOT NULL,
            subcategory_confidence REAL,
            confidence_factors TEXT,
            model_version TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(file_name, file_type, dir_path) REFERENCES file_categorization(file_name, file_type, dir_path),
            UNIQUE(file_name, file_type, dir_path)
        );
    )";
    if (sqlite3_exec(db, confidence_scores_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create confidence_scores table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Content analysis cache table
    const char *content_analysis_sql = R"(
        CREATE TABLE IF NOT EXISTS content_analysis_cache (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL UNIQUE,
            content_hash TEXT NOT NULL,
            mime_type TEXT,
            keywords TEXT,
            detected_language TEXT,
            metadata TEXT,
            analysis_summary TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    if (sqlite3_exec(db, content_analysis_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create content_analysis_cache table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // API usage tracking table
    const char *api_usage_sql = R"(
        CREATE TABLE IF NOT EXISTS api_usage_tracking (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            provider TEXT NOT NULL,
            date DATE NOT NULL,
            tokens_used INTEGER DEFAULT 0,
            requests_made INTEGER DEFAULT 0,
            cost_estimate REAL DEFAULT 0.0,
            daily_limit INTEGER,
            remaining INTEGER,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(provider, date)
        );
    )";
    if (sqlite3_exec(db, api_usage_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create api_usage_tracking table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Enhanced user profiles table for multiple profile support
    const char *user_profiles_sql = R"(
        CREATE TABLE IF NOT EXISTS user_profiles (
            profile_id INTEGER PRIMARY KEY AUTOINCREMENT,
            profile_name TEXT UNIQUE NOT NULL,
            is_active INTEGER DEFAULT 0,
            created_at DATETIME NOT NULL,
            last_used DATETIME,
            CHECK(is_active IN (0, 1))
        );
    )";
    if (sqlite3_exec(db, user_profiles_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create user_profiles table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // User corrections table for learning
    const char *user_corrections_sql = R"(
        CREATE TABLE IF NOT EXISTS user_corrections (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL,
            file_name TEXT NOT NULL,
            original_category TEXT NOT NULL,
            original_subcategory TEXT,
            corrected_category TEXT NOT NULL,
            corrected_subcategory TEXT,
            file_extension TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            profile_id INTEGER,
            FOREIGN KEY(profile_id) REFERENCES user_profiles(profile_id)
        );
    )";
    if (sqlite3_exec(db, user_corrections_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create user_corrections table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Categorization sessions table
    const char *sessions_sql = R"(
        CREATE TABLE IF NOT EXISTS categorization_sessions (
            session_id TEXT PRIMARY KEY,
            folder_path TEXT NOT NULL,
            started_at DATETIME NOT NULL,
            completed_at DATETIME,
            consistency_mode TEXT,
            consistency_strength REAL DEFAULT 0.5,
            files_processed INTEGER DEFAULT 0,
            CHECK(consistency_mode IN ('refined', 'consistent', 'hybrid'))
        );
    )";
    if (sqlite3_exec(db, sessions_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create categorization_sessions table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Enhanced undo history table
    const char *undo_history_sql = R"(
        CREATE TABLE IF NOT EXISTS undo_history (
            undo_id INTEGER PRIMARY KEY AUTOINCREMENT,
            plan_path TEXT NOT NULL,
            description TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_undone INTEGER DEFAULT 0,
            CHECK(is_undone IN (0, 1))
        );
    )";
    if (sqlite3_exec(db, undo_history_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create undo_history table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // File Tinder state table
    const char *file_tinder_sql = R"(
        CREATE TABLE IF NOT EXISTS file_tinder_state (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_path TEXT NOT NULL,
            file_path TEXT NOT NULL,
            decision TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            CHECK(decision IN ('keep', 'delete', 'ignore', 'pending')),
            UNIQUE(folder_path, file_path)
        );
    )";
    if (sqlite3_exec(db, file_tinder_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create file_tinder_state table: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Create indices for performance
    const char *create_confidence_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_confidence_scores_file ON confidence_scores(file_name, file_type, dir_path);";
    if (sqlite3_exec(db, create_confidence_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create confidence index: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    const char *create_content_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_content_analysis_hash ON content_analysis_cache(content_hash);";
    if (sqlite3_exec(db, create_content_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create content analysis index: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    const char *create_api_usage_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_api_usage_date ON api_usage_tracking(provider, date);";
    if (sqlite3_exec(db, create_api_usage_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create API usage index: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    const char *create_corrections_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_user_corrections_profile ON user_corrections(profile_id);";
    if (sqlite3_exec(db, create_corrections_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create corrections index: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    const char *create_sessions_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_sessions_folder ON categorization_sessions(folder_path);";
    if (sqlite3_exec(db, create_sessions_index_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to create sessions index: {}", error_msg);
        sqlite3_free(error_msg);
    }
}

bool DatabaseManager::save_user_profile(const UserProfile& profile) {
    if (!db) return false;

    // On conflict, we only update last_updated to preserve the original created_at
    const char *sql = R"(
        INSERT INTO user_profile (user_id, created_at, last_updated)
        VALUES (?, ?, ?)
        ON CONFLICT(user_id) DO UPDATE SET last_updated = excluded.last_updated;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save user profile: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, profile.user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, profile.created_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, profile.last_updated.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (success) {
        // Save characteristics
        for (const auto& characteristic : profile.characteristics) {
            save_user_characteristic(profile.user_id, characteristic);
        }

        // Save folder insights
        for (const auto& insight : profile.folder_insights) {
            save_folder_insight(profile.user_id, insight);
        }
        
        // Save organizational templates
        for (const auto& templ : profile.learned_templates) {
            save_organizational_template(profile.user_id, templ);
        }
    }

    return success;
}

UserProfile DatabaseManager::load_user_profile(const std::string& user_id) {
    UserProfile profile;
    profile.user_id = user_id;

    if (!db) return profile;

    const char *sql = "SELECT created_at, last_updated FROM user_profile WHERE user_id = ?;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn, "Failed to prepare load user profile: {}", sqlite3_errmsg(db));
        return profile;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *created = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *updated = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        profile.created_at = created ? created : "";
        profile.last_updated = updated ? updated : "";
    }

    sqlite3_finalize(stmt);

    // Load characteristics, folder insights, and templates
    profile.characteristics = load_user_characteristics(user_id);
    profile.folder_insights = load_folder_insights(user_id);
    profile.learned_templates = load_organizational_templates(user_id);

    return profile;
}

bool DatabaseManager::save_user_characteristic(const std::string& user_id,
                                               const UserCharacteristic& characteristic) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO user_characteristics (user_id, trait_name, value, confidence, evidence, timestamp)
        VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT(user_id, trait_name, value) DO UPDATE SET
            confidence = excluded.confidence,
            evidence = excluded.evidence,
            timestamp = excluded.timestamp;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save characteristic: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, characteristic.trait_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, characteristic.value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, characteristic.confidence);
    sqlite3_bind_text(stmt, 5, characteristic.evidence.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, characteristic.timestamp.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return success;
}

std::vector<UserCharacteristic> DatabaseManager::load_user_characteristics(const std::string& user_id) {
    std::vector<UserCharacteristic> characteristics;

    if (!db) return characteristics;

    const char *sql = R"(
        SELECT trait_name, value, confidence, evidence, timestamp
        FROM user_characteristics
        WHERE user_id = ?
        ORDER BY confidence DESC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn, "Failed to prepare load characteristics: {}", sqlite3_errmsg(db));
        return characteristics;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserCharacteristic characteristic;
        const char *trait = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *evidence = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        characteristic.trait_name = trait ? trait : "";
        characteristic.value = value ? value : "";
        characteristic.confidence = static_cast<float>(sqlite3_column_double(stmt, 2));
        characteristic.evidence = evidence ? evidence : "";
        characteristic.timestamp = timestamp ? timestamp : "";

        characteristics.push_back(characteristic);
    }

    sqlite3_finalize(stmt);
    return characteristics;
}

bool DatabaseManager::save_folder_insight(const std::string& user_id,
                                          const FolderInsight& insight) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO folder_insights (user_id, folder_path, description, dominant_categories,
                                    file_count, last_analyzed, usage_pattern)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(user_id, folder_path) DO UPDATE SET
            description = excluded.description,
            dominant_categories = excluded.dominant_categories,
            file_count = excluded.file_count,
            last_analyzed = excluded.last_analyzed,
            usage_pattern = excluded.usage_pattern;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save folder insight: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, insight.folder_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, insight.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, insight.dominant_categories.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, insight.file_count);
    sqlite3_bind_text(stmt, 6, insight.last_analyzed.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, insight.usage_pattern.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return success;
}

std::vector<FolderInsight> DatabaseManager::load_folder_insights(const std::string& user_id) {
    std::vector<FolderInsight> insights;

    if (!db) return insights;

    const char *sql = R"(
        SELECT folder_path, description, dominant_categories, file_count,
               last_analyzed, usage_pattern
        FROM folder_insights
        WHERE user_id = ?
        ORDER BY last_analyzed DESC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn, "Failed to prepare load folder insights: {}", sqlite3_errmsg(db));
        return insights;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FolderInsight insight;
        const char *path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *cats = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *analyzed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char *pattern = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

        insight.folder_path = path ? path : "";
        insight.description = desc ? desc : "";
        insight.dominant_categories = cats ? cats : "";
        insight.file_count = sqlite3_column_int(stmt, 3);
        insight.last_analyzed = analyzed ? analyzed : "";
        insight.usage_pattern = pattern ? pattern : "";

        insights.push_back(insight);
    }

    sqlite3_finalize(stmt);
    return insights;
}

std::string DatabaseManager::get_folder_inclusion_level(const std::string& folder_path) {
    if (!db) return "full";  // Default to full inclusion

    const char *sql = "SELECT inclusion_level FROM folder_learning_settings WHERE folder_path = ?;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn, "Failed to prepare get folder inclusion level: {}", sqlite3_errmsg(db));
        return "full";
    }

    sqlite3_bind_text(stmt, 1, folder_path.c_str(), -1, SQLITE_TRANSIENT);

    std::string level = "full";  // Default
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (result) {
            level = result;
        }
    }

    sqlite3_finalize(stmt);
    return level;
}

void DatabaseManager::set_folder_inclusion_level(const std::string& folder_path, const std::string& level) {
    if (!db) return;

    const char *sql = R"(
        INSERT INTO folder_learning_settings (folder_path, inclusion_level)
        VALUES (?, ?)
        ON CONFLICT(folder_path) DO UPDATE SET inclusion_level = excluded.inclusion_level;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare set folder inclusion level: {}", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, folder_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, level.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        db_log(spdlog::level::err, "Failed to set folder inclusion level: {}", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

bool DatabaseManager::save_organizational_template(const std::string& user_id,
                                                   const OrganizationalTemplate& templ) {
    if (!db) return false;

    // Convert vectors to semicolon-separated strings (safer than commas which may appear in category names)
    std::string categories_str;
    for (size_t i = 0; i < templ.suggested_categories.size(); ++i) {
        if (i > 0) categories_str += ";";
        categories_str += templ.suggested_categories[i];
    }
    
    std::string subcategories_str;
    for (size_t i = 0; i < templ.suggested_subcategories.size(); ++i) {
        if (i > 0) subcategories_str += ";";
        subcategories_str += templ.suggested_subcategories[i];
    }

    const char *sql = R"(
        INSERT INTO organizational_templates 
        (user_id, template_name, description, suggested_categories, 
         suggested_subcategories, confidence, based_on_folders, usage_count)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(user_id, template_name) DO UPDATE SET
            description = excluded.description,
            suggested_categories = excluded.suggested_categories,
            suggested_subcategories = excluded.suggested_subcategories,
            confidence = excluded.confidence,
            based_on_folders = excluded.based_on_folders,
            usage_count = excluded.usage_count;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save template: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, templ.template_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, templ.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, categories_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, subcategories_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 6, templ.confidence);
    sqlite3_bind_text(stmt, 7, templ.based_on_folders.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, templ.usage_count);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return success;
}

std::vector<OrganizationalTemplate> DatabaseManager::load_organizational_templates(
    const std::string& user_id) {
    std::vector<OrganizationalTemplate> templates;

    if (!db) return templates;

    const char *sql = R"(
        SELECT template_name, description, suggested_categories, 
               suggested_subcategories, confidence, based_on_folders, usage_count
        FROM organizational_templates
        WHERE user_id = ?
        ORDER BY confidence DESC, usage_count DESC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::warn, "Failed to prepare load templates: {}", sqlite3_errmsg(db));
        return templates;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OrganizationalTemplate templ;
        
        const char *name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *cats = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *subcats = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *folders = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

        templ.template_name = name ? name : "";
        templ.description = desc ? desc : "";
        templ.confidence = sqlite3_column_double(stmt, 4);
        templ.based_on_folders = folders ? folders : "";
        templ.usage_count = sqlite3_column_int(stmt, 6);

        // Parse semicolon-separated categories (using semicolon to avoid conflicts with category names containing commas)
        if (cats) {
            std::string cats_str = cats;
            size_t pos = 0;
            while ((pos = cats_str.find(";")) != std::string::npos) {
                templ.suggested_categories.push_back(cats_str.substr(0, pos));
                cats_str.erase(0, pos + 1);
            }
            if (!cats_str.empty()) {
                templ.suggested_categories.push_back(cats_str);
            }
        }

        // Parse semicolon-separated subcategories
        if (subcats) {
            std::string subcats_str = subcats;
            size_t pos = 0;
            while ((pos = subcats_str.find(";")) != std::string::npos) {
                templ.suggested_subcategories.push_back(subcats_str.substr(0, pos));
                subcats_str.erase(0, pos + 1);
            }
            if (!subcats_str.empty()) {
                templ.suggested_subcategories.push_back(subcats_str);
            }
        }

        templates.push_back(templ);
    }

    sqlite3_finalize(stmt);
    return templates;
}

// Phase 1.1: Enhanced feature method implementations

bool DatabaseManager::save_confidence_score(const std::string& file_name,
                                           const std::string& file_type,
                                           const std::string& dir_path,
                                           const ConfidenceScore& score) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO confidence_scores (file_name, file_type, dir_path, category_confidence, 
                                      subcategory_confidence, confidence_factors, model_version)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(file_name, file_type, dir_path) DO UPDATE SET
            category_confidence = excluded.category_confidence,
            subcategory_confidence = excluded.subcategory_confidence,
            confidence_factors = excluded.confidence_factors,
            model_version = excluded.model_version,
            timestamp = CURRENT_TIMESTAMP;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save confidence score: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, file_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, file_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, dir_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, score.category_confidence);
    sqlite3_bind_double(stmt, 5, score.subcategory_confidence);
    sqlite3_bind_text(stmt, 6, score.confidence_factors.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, score.model_version.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::optional<DatabaseManager::ConfidenceScore> DatabaseManager::get_confidence_score(
    const std::string& file_name,
    const std::string& file_type,
    const std::string& dir_path) {
    if (!db) return std::nullopt;

    const char *sql = R"(
        SELECT category_confidence, subcategory_confidence, confidence_factors, model_version
        FROM confidence_scores
        WHERE file_name = ? AND file_type = ? AND dir_path = ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, file_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, file_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, dir_path.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<ConfidenceScore> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ConfidenceScore score;
        score.category_confidence = sqlite3_column_double(stmt, 0);
        score.subcategory_confidence = sqlite3_column_double(stmt, 1);
        const char *factors = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        score.confidence_factors = factors ? factors : "";
        score.model_version = version ? version : "";
        result = score;
    }

    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::save_content_analysis(const std::string& file_path,
                                           const ContentAnalysis& analysis) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO content_analysis_cache (file_path, content_hash, mime_type, keywords,
                                           detected_language, metadata, analysis_summary)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(file_path) DO UPDATE SET
            content_hash = excluded.content_hash,
            mime_type = excluded.mime_type,
            keywords = excluded.keywords,
            detected_language = excluded.detected_language,
            metadata = excluded.metadata,
            analysis_summary = excluded.analysis_summary,
            timestamp = CURRENT_TIMESTAMP;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save content analysis: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, analysis.content_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, analysis.mime_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, analysis.keywords.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, analysis.detected_language.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, analysis.metadata.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, analysis.analysis_summary.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::optional<DatabaseManager::ContentAnalysis> DatabaseManager::get_content_analysis(
    const std::string& file_path) {
    if (!db) return std::nullopt;

    const char *sql = R"(
        SELECT content_hash, mime_type, keywords, detected_language, metadata, analysis_summary
        FROM content_analysis_cache
        WHERE file_path = ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<ContentAnalysis> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ContentAnalysis analysis;
        const char *hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *mime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *keywords = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *lang = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *meta = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char *summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        
        analysis.content_hash = hash ? hash : "";
        analysis.mime_type = mime ? mime : "";
        analysis.keywords = keywords ? keywords : "";
        analysis.detected_language = lang ? lang : "";
        analysis.metadata = meta ? meta : "";
        analysis.analysis_summary = summary ? summary : "";
        result = analysis;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<DatabaseManager::ContentAnalysis> DatabaseManager::get_content_analysis_by_hash(
    const std::string& content_hash) {
    if (!db) return std::nullopt;

    const char *sql = R"(
        SELECT content_hash, mime_type, keywords, detected_language, metadata, analysis_summary
        FROM content_analysis_cache
        WHERE content_hash = ?
        LIMIT 1;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, content_hash.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<ContentAnalysis> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ContentAnalysis analysis;
        const char *hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *mime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *keywords = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *lang = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *meta = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char *summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        
        analysis.content_hash = hash ? hash : "";
        analysis.mime_type = mime ? mime : "";
        analysis.keywords = keywords ? keywords : "";
        analysis.detected_language = lang ? lang : "";
        analysis.metadata = meta ? meta : "";
        analysis.analysis_summary = summary ? summary : "";
        result = analysis;
    }

    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::record_api_usage(const std::string& provider,
                                      int tokens,
                                      int requests,
                                      float cost) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO api_usage_tracking (provider, date, tokens_used, requests_made, cost_estimate)
        VALUES (?, DATE('now'), ?, ?, ?)
        ON CONFLICT(provider, date) DO UPDATE SET
            tokens_used = tokens_used + excluded.tokens_used,
            requests_made = requests_made + excluded.requests_made,
            cost_estimate = cost_estimate + excluded.cost_estimate,
            timestamp = CURRENT_TIMESTAMP;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare record API usage: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, provider.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, tokens);
    sqlite3_bind_int(stmt, 3, requests);
    sqlite3_bind_double(stmt, 4, cost);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::optional<DatabaseManager::APIUsage> DatabaseManager::get_api_usage_today(
    const std::string& provider) {
    if (!db) return std::nullopt;

    const char *sql = R"(
        SELECT provider, date, tokens_used, requests_made, cost_estimate, daily_limit, remaining
        FROM api_usage_tracking
        WHERE provider = ? AND date = DATE('now');
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, provider.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<APIUsage> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        APIUsage usage;
        const char *prov = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        usage.provider = prov ? prov : "";
        usage.date = date ? date : "";
        usage.tokens_used = sqlite3_column_int(stmt, 2);
        usage.requests_made = sqlite3_column_int(stmt, 3);
        usage.cost_estimate = static_cast<float>(sqlite3_column_double(stmt, 4));
        usage.daily_limit = sqlite3_column_int(stmt, 5);
        usage.remaining = sqlite3_column_int(stmt, 6);
        result = usage;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<DatabaseManager::APIUsage> DatabaseManager::get_api_usage_history(
    const std::string& provider,
    int days) {
    std::vector<APIUsage> history;
    if (!db) return history;

    const char *sql = R"(
        SELECT provider, date, tokens_used, requests_made, cost_estimate, daily_limit, remaining
        FROM api_usage_tracking
        WHERE provider = ? AND date >= DATE('now', ?)
        ORDER BY date DESC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return history;
    }

    std::string days_param = "-" + std::to_string(days) + " days";
    sqlite3_bind_text(stmt, 1, provider.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, days_param.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        APIUsage usage;
        const char *prov = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        usage.provider = prov ? prov : "";
        usage.date = date ? date : "";
        usage.tokens_used = sqlite3_column_int(stmt, 2);
        usage.requests_made = sqlite3_column_int(stmt, 3);
        usage.cost_estimate = static_cast<float>(sqlite3_column_double(stmt, 4));
        usage.daily_limit = sqlite3_column_int(stmt, 5);
        usage.remaining = sqlite3_column_int(stmt, 6);
        history.push_back(usage);
    }

    sqlite3_finalize(stmt);
    return history;
}

int DatabaseManager::create_user_profile(const std::string& profile_name) {
    if (!db) return -1;

    const char *sql = R"(
        INSERT INTO user_profiles (profile_name, created_at, last_used)
        VALUES (?, DATETIME('now'), DATETIME('now'));
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare create profile: {}", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, profile_name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }

    int profile_id = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return profile_id;
}

bool DatabaseManager::set_active_profile(int profile_id) {
    if (!db) return false;

    // First, deactivate all profiles
    const char *deactivate_sql = "UPDATE user_profiles SET is_active = 0;";
    char *error_msg = nullptr;
    if (sqlite3_exec(db, deactivate_sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to deactivate profiles: {}", error_msg);
        sqlite3_free(error_msg);
        return false;
    }

    // Then activate the specified profile
    const char *activate_sql = R"(
        UPDATE user_profiles 
        SET is_active = 1, last_used = DATETIME('now')
        WHERE profile_id = ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, activate_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, profile_id);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::optional<DatabaseManager::UserProfileInfo> DatabaseManager::get_active_profile() {
    if (!db) return std::nullopt;

    const char *sql = R"(
        SELECT profile_id, profile_name, is_active, created_at, last_used
        FROM user_profiles
        WHERE is_active = 1
        LIMIT 1;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    std::optional<UserProfileInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserProfileInfo info;
        info.profile_id = sqlite3_column_int(stmt, 0);
        const char *name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *created = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *used = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        info.profile_name = name ? name : "";
        info.is_active = sqlite3_column_int(stmt, 2) != 0;
        info.created_at = created ? created : "";
        info.last_used = used ? used : "";
        result = info;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<DatabaseManager::UserProfileInfo> DatabaseManager::get_all_profiles() {
    std::vector<UserProfileInfo> profiles;
    if (!db) return profiles;

    const char *sql = R"(
        SELECT profile_id, profile_name, is_active, created_at, last_used
        FROM user_profiles
        ORDER BY last_used DESC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return profiles;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserProfileInfo info;
        info.profile_id = sqlite3_column_int(stmt, 0);
        const char *name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *created = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *used = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        info.profile_name = name ? name : "";
        info.is_active = sqlite3_column_int(stmt, 2) != 0;
        info.created_at = created ? created : "";
        info.last_used = used ? used : "";
        profiles.push_back(info);
    }

    sqlite3_finalize(stmt);
    return profiles;
}

bool DatabaseManager::delete_profile(int profile_id) {
    if (!db) return false;

    const char *sql = "DELETE FROM user_profiles WHERE profile_id = ?;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, profile_id);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::record_correction(const UserCorrection& correction, int profile_id) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO user_corrections (file_path, file_name, original_category, original_subcategory,
                                     corrected_category, corrected_subcategory, file_extension, profile_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare record correction: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, correction.file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, correction.file_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, correction.original_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, correction.original_subcategory.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, correction.corrected_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, correction.corrected_subcategory.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, correction.file_extension.c_str(), -1, SQLITE_TRANSIENT);
    
    if (profile_id >= 0) {
        sqlite3_bind_int(stmt, 8, profile_id);
    } else {
        sqlite3_bind_null(stmt, 8);
    }

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<DatabaseManager::UserCorrection> DatabaseManager::get_corrections(
    int profile_id, int limit) {
    std::vector<UserCorrection> corrections;
    if (!db) return corrections;

    std::string sql = R"(
        SELECT file_path, file_name, original_category, original_subcategory,
               corrected_category, corrected_subcategory, file_extension, timestamp
        FROM user_corrections
    )";
    
    if (profile_id >= 0) {
        sql += " WHERE profile_id = ?";
    }
    
    sql += " ORDER BY timestamp DESC LIMIT ?;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return corrections;
    }

    int param_index = 1;
    if (profile_id >= 0) {
        sqlite3_bind_int(stmt, param_index++, profile_id);
    }
    sqlite3_bind_int(stmt, param_index, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserCorrection correction;
        const char *path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *orig_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *orig_sub = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *corr_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char *corr_sub = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char *ext = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const char *ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        
        correction.file_path = path ? path : "";
        correction.file_name = name ? name : "";
        correction.original_category = orig_cat ? orig_cat : "";
        correction.original_subcategory = orig_sub ? orig_sub : "";
        correction.corrected_category = corr_cat ? corr_cat : "";
        correction.corrected_subcategory = corr_sub ? corr_sub : "";
        correction.file_extension = ext ? ext : "";
        correction.timestamp = ts ? ts : "";
        corrections.push_back(correction);
    }

    sqlite3_finalize(stmt);
    return corrections;
}

std::map<std::string, int> DatabaseManager::get_correction_patterns() {
    std::map<std::string, int> patterns;
    if (!db) return patterns;

    const char *sql = R"(
        SELECT original_category || ' -> ' || corrected_category as pattern, COUNT(*) as count
        FROM user_corrections
        GROUP BY pattern
        ORDER BY count DESC
        LIMIT 20;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return patterns;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *pattern = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int count = sqlite3_column_int(stmt, 1);
        if (pattern) {
            patterns[pattern] = count;
        }
    }

    sqlite3_finalize(stmt);
    return patterns;
}

bool DatabaseManager::create_session(const std::string& session_id,
                                    const std::string& folder_path,
                                    const std::string& consistency_mode,
                                    float consistency_strength) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO categorization_sessions (session_id, folder_path, started_at,
                                            consistency_mode, consistency_strength)
        VALUES (?, ?, DATETIME('now'), ?, ?);
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare create session: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, folder_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, consistency_mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, consistency_strength);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::complete_session(const std::string& session_id, int files_processed) {
    if (!db) return false;

    const char *sql = R"(
        UPDATE categorization_sessions
        SET completed_at = DATETIME('now'), files_processed = ?
        WHERE session_id = ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, files_processed);
    sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::optional<DatabaseManager::SessionInfo> DatabaseManager::get_session(
    const std::string& session_id) {
    if (!db) return std::nullopt;

    const char *sql = R"(
        SELECT session_id, folder_path, started_at, completed_at, consistency_mode,
               consistency_strength, files_processed
        FROM categorization_sessions
        WHERE session_id = ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<SessionInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionInfo info;
        const char *sid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *folder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *started = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *completed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *mode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        info.session_id = sid ? sid : "";
        info.folder_path = folder ? folder : "";
        info.started_at = started ? started : "";
        info.completed_at = completed ? completed : "";
        info.consistency_mode = mode ? mode : "";
        info.consistency_strength = static_cast<float>(sqlite3_column_double(stmt, 5));
        info.files_processed = sqlite3_column_int(stmt, 6);
        result = info;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<DatabaseManager::SessionInfo> DatabaseManager::get_recent_sessions(int limit) {
    std::vector<SessionInfo> sessions;
    if (!db) return sessions;

    const char *sql = R"(
        SELECT session_id, folder_path, started_at, completed_at, consistency_mode,
               consistency_strength, files_processed
        FROM categorization_sessions
        ORDER BY started_at DESC
        LIMIT ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sessions;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionInfo info;
        const char *sid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *folder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *started = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *completed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char *mode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        info.session_id = sid ? sid : "";
        info.folder_path = folder ? folder : "";
        info.started_at = started ? started : "";
        info.completed_at = completed ? completed : "";
        info.consistency_mode = mode ? mode : "";
        info.consistency_strength = static_cast<float>(sqlite3_column_double(stmt, 5));
        info.files_processed = sqlite3_column_int(stmt, 6);
        sessions.push_back(info);
    }

    sqlite3_finalize(stmt);
    return sessions;
}

bool DatabaseManager::record_undo_plan(const std::string& plan_path,
                                      const std::string& description) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO undo_history (plan_path, description)
        VALUES (?, ?);
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare record undo plan: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, plan_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::mark_plan_undone(int undo_id) {
    if (!db) return false;

    const char *sql = "UPDATE undo_history SET is_undone = 1 WHERE undo_id = ?;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, undo_id);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<std::pair<int, std::string>> DatabaseManager::get_undo_history(int limit) {
    std::vector<std::pair<int, std::string>> history;
    if (!db) return history;

    const char *sql = R"(
        SELECT undo_id, description, timestamp, is_undone
        FROM undo_history
        ORDER BY timestamp DESC
        LIMIT ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return history;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        bool undone = sqlite3_column_int(stmt, 3) != 0;
        
        std::string description = (desc ? desc : "") + std::string(" (") + 
                                 (ts ? ts : "") + ")" + 
                                 (undone ? " [UNDONE]" : "");
        history.emplace_back(id, description);
    }

    sqlite3_finalize(stmt);
    return history;
}

bool DatabaseManager::save_tinder_decision(const FileTinderDecision& decision) {
    if (!db) return false;

    const char *sql = R"(
        INSERT INTO file_tinder_state (folder_path, file_path, decision)
        VALUES (?, ?, ?)
        ON CONFLICT(folder_path, file_path) DO UPDATE SET
            decision = excluded.decision,
            timestamp = CURRENT_TIMESTAMP;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare save tinder decision: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, decision.folder_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, decision.file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, decision.decision.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<DatabaseManager::FileTinderDecision> DatabaseManager::get_tinder_decisions(
    const std::string& folder_path) {
    std::vector<FileTinderDecision> decisions;
    if (!db) return decisions;

    const char *sql = R"(
        SELECT folder_path, file_path, decision, timestamp
        FROM file_tinder_state
        WHERE folder_path = ?
        ORDER BY timestamp DESC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return decisions;
    }

    sqlite3_bind_text(stmt, 1, folder_path.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileTinderDecision decision;
        const char *folder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char *file = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char *dec = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char *ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        
        decision.folder_path = folder ? folder : "";
        decision.file_path = file ? file : "";
        decision.decision = dec ? dec : "";
        decision.timestamp = ts ? ts : "";
        decisions.push_back(decision);
    }

    sqlite3_finalize(stmt);
    return decisions;
}

bool DatabaseManager::clear_tinder_session(const std::string& folder_path) {
    if (!db) return false;

    const char *sql = "DELETE FROM file_tinder_state WHERE folder_path = ?;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, folder_path.c_str(), -1, SQLITE_TRANSIENT);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

// Cache management methods
DatabaseManager::CacheStatistics DatabaseManager::get_cache_statistics() {
    CacheStatistics stats{};
    stats.entry_count = 0;
    stats.database_size_bytes = 0;
    stats.distinct_folders = 0;
    
    if (!db) return stats;
    
    // Get entry count
    const char *count_sql = "SELECT COUNT(*) FROM file_categorization;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.entry_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    // Get database file size
    const char *size_sql = "SELECT page_count * page_size as size FROM pragma_page_count(), pragma_page_size();";
    if (sqlite3_prepare_v2(db, size_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.database_size_bytes = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    // Get oldest and newest entry dates
    const char *date_sql = R"(
        SELECT 
            MIN(timestamp) as oldest, 
            MAX(timestamp) as newest
        FROM file_categorization;
    )";
    if (sqlite3_prepare_v2(db, date_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *oldest = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char *newest = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            stats.oldest_entry_date = oldest ? oldest : "N/A";
            stats.newest_entry_date = newest ? newest : "N/A";
        }
        sqlite3_finalize(stmt);
    }
    
    // Get distinct folder count
    const char *folder_sql = "SELECT COUNT(DISTINCT dir_path) FROM file_categorization;";
    if (sqlite3_prepare_v2(db, folder_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.distinct_folders = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return stats;
}

bool DatabaseManager::clear_all_cache() {
    if (!db) return false;
    
    char *error_msg = nullptr;
    const char *sql = "DELETE FROM file_categorization;";
    
    bool success = sqlite3_exec(db, sql, nullptr, nullptr, &error_msg) == SQLITE_OK;
    if (!success && error_msg) {
        db_log(spdlog::level::err, "Failed to clear all cache: {}", error_msg);
        sqlite3_free(error_msg);
    }
    
    // Also clear content analysis cache
    const char *content_sql = "DELETE FROM content_analysis_cache;";
    sqlite3_exec(db, content_sql, nullptr, nullptr, nullptr);
    
    return success;
}

bool DatabaseManager::clear_cache_for_folder(const std::string& folder_path) {
    if (!db) return false;
    
    const char *sql = "DELETE FROM file_categorization WHERE dir_path = ?;";
    
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare clear folder cache: {}", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, folder_path.c_str(), -1, SQLITE_TRANSIENT);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    if (!success) {
        db_log(spdlog::level::err, "Failed to clear cache for folder '{}'", folder_path);
    }
    
    return success;
}

bool DatabaseManager::clear_cache_older_than(int days) {
    if (!db || days < 0) return false;
    
    const char *sql = R"(
        DELETE FROM file_categorization 
        WHERE timestamp < datetime('now', '-' || ? || ' days');
    )";
    
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        db_log(spdlog::level::err, "Failed to prepare clear old cache: {}", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, days);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    if (!success) {
        db_log(spdlog::level::err, "Failed to clear cache older than {} days", days);
    }
    
    // Also clear old content analysis
    const char *content_sql = R"(
        DELETE FROM content_analysis_cache 
        WHERE timestamp < datetime('now', '-' || ? || ' days');
    )";
    if (sqlite3_prepare_v2(db, content_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, days);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    return success;
}

bool DatabaseManager::optimize_database() {
    if (!db) return false;
    
    char *error_msg = nullptr;
    bool success = sqlite3_exec(db, "VACUUM;", nullptr, nullptr, &error_msg) == SQLITE_OK;
    
    if (!success && error_msg) {
        db_log(spdlog::level::err, "Failed to optimize database: {}", error_msg);
        sqlite3_free(error_msg);
    } else {
        db_log(spdlog::level::info, "Database optimized successfully");
    }
    
    return success;
}
