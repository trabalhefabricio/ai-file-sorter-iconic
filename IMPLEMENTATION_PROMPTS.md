# Implementation Prompts for AI File Sorter Re-fork

Complete set of detailed prompts to copy-paste to AI agents for implementing custom features in order.

---

## PROMPT 1: Database Schema Foundation

Copy everything below to your AI agent:

```
I need to add custom database tables to AI File Sorter for new features.

Repository: Fresh fork of hyperfield/ai-file-sorter
Files to modify: app/lib/DatabaseManager.cpp, app/include/DatabaseManager.hpp, app/include/Types.hpp

Add this method to DatabaseManager.cpp:

void DatabaseManager::init_custom_features_tables() {
    // Tables for implemented features
    const char *user_profile_sql = "CREATE TABLE IF NOT EXISTS user_profile (user_id TEXT PRIMARY KEY, created_at TEXT NOT NULL, last_updated TEXT NOT NULL);";
    
    const char *user_characteristics_sql = "CREATE TABLE IF NOT EXISTS user_characteristics (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id TEXT NOT NULL, trait_name TEXT NOT NULL, value TEXT NOT NULL, confidence REAL NOT NULL, evidence TEXT, timestamp TEXT NOT NULL, FOREIGN KEY(user_id) REFERENCES user_profile(user_id), UNIQUE(user_id, trait_name, value));";
    
    const char *folder_insights_sql = "CREATE TABLE IF NOT EXISTS folder_insights (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id TEXT NOT NULL, folder_path TEXT NOT NULL, description TEXT, dominant_categories TEXT, file_count INTEGER, last_analyzed TEXT NOT NULL, usage_pattern TEXT, FOREIGN KEY(user_id) REFERENCES user_profile(user_id), UNIQUE(user_id, folder_path));";
    
    const char *folder_learning_sql = "CREATE TABLE IF NOT EXISTS folder_learning_settings (folder_path TEXT PRIMARY KEY, inclusion_level TEXT NOT NULL DEFAULT 'full', CHECK(inclusion_level IN ('none', 'partial', 'full')));";
    
    const char *templates_sql = "CREATE TABLE IF NOT EXISTS organizational_templates (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id TEXT NOT NULL, template_name TEXT NOT NULL, description TEXT, suggested_categories TEXT, suggested_subcategories TEXT, confidence REAL NOT NULL, based_on_folders TEXT, usage_count INTEGER DEFAULT 1, FOREIGN KEY(user_id) REFERENCES user_profile(user_id), UNIQUE(user_id, template_name));";
    
    const char *api_usage_sql = "CREATE TABLE IF NOT EXISTS api_usage_tracking (id INTEGER PRIMARY KEY AUTOINCREMENT, provider TEXT NOT NULL, date DATE NOT NULL, tokens_used INTEGER NOT NULL, requests_made INTEGER NOT NULL, cost_estimate REAL, model_used TEXT);";
    
    const char *tinder_sql = "CREATE TABLE IF NOT EXISTS file_tinder_state (session_id TEXT PRIMARY KEY, folder_path TEXT NOT NULL, current_index INTEGER, decisions TEXT, timestamp DATETIME);";
    
    const char *undo_sql = "CREATE TABLE IF NOT EXISTS undo_history (id INTEGER PRIMARY KEY AUTOINCREMENT, session_id TEXT NOT NULL, operation_type TEXT NOT NULL, file_moves TEXT NOT NULL, timestamp DATETIME, executed BOOLEAN DEFAULT 0);";
    
    // Tables for future features
    const char *confidence_sql = "CREATE TABLE IF NOT EXISTS confidence_scores (id INTEGER PRIMARY KEY AUTOINCREMENT, file_name TEXT NOT NULL, file_type TEXT NOT NULL, dir_path TEXT NOT NULL, category_confidence REAL NOT NULL, subcategory_confidence REAL, confidence_factors TEXT, model_version TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, UNIQUE(file_name, file_type, dir_path));";
    
    const char *content_sql = "CREATE TABLE IF NOT EXISTS content_analysis_cache (id INTEGER PRIMARY KEY AUTOINCREMENT, file_path TEXT NOT NULL UNIQUE, content_hash TEXT NOT NULL, mime_type TEXT, keywords TEXT, detected_language TEXT, metadata TEXT, analysis_summary TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    
    const char *sessions_sql = "CREATE TABLE IF NOT EXISTS categorization_sessions (session_id TEXT PRIMARY KEY, folder_path TEXT NOT NULL, settings_json TEXT, progress_json TEXT, status TEXT, created_at DATETIME, updated_at DATETIME);";
    
    const char *corrections_sql = "CREATE TABLE IF NOT EXISTS user_corrections (id INTEGER PRIMARY KEY AUTOINCREMENT, file_path TEXT NOT NULL, original_category TEXT NOT NULL, corrected_category TEXT NOT NULL, original_subcategory TEXT, corrected_subcategory TEXT, correction_reason TEXT, profile_id INTEGER, timestamp DATETIME);";
    
    char *error_msg = nullptr;
    const char* tables[] = {user_profile_sql, user_characteristics_sql, folder_insights_sql, folder_learning_sql, templates_sql, api_usage_sql, tinder_sql, undo_sql, confidence_sql, content_sql, sessions_sql, corrections_sql};
    
    for (const char* sql : tables) {
        if (sqlite3_exec(db, sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
            db_log(spdlog::level::err, "Failed to create table: {}", error_msg);
            sqlite3_free(error_msg);
        }
    }
    
    // Create indices
    const char* indices[] = {
        "CREATE INDEX IF NOT EXISTS idx_user_characteristics_user ON user_characteristics(user_id);",
        "CREATE INDEX IF NOT EXISTS idx_folder_insights_user ON folder_insights(user_id);",
        "CREATE INDEX IF NOT EXISTS idx_templates_user ON organizational_templates(user_id);",
        "CREATE INDEX IF NOT EXISTS idx_api_usage_date ON api_usage_tracking(provider, date);",
        "CREATE INDEX IF NOT EXISTS idx_confidence_file ON confidence_scores(file_name, file_type, dir_path);",
        "CREATE INDEX IF NOT EXISTS idx_content_hash ON content_analysis_cache(content_hash);",
        "CREATE INDEX IF NOT EXISTS idx_corrections_profile ON user_corrections(profile_id);"
    };
    
    for (const char* sql : indices) {
        if (sqlite3_exec(db, sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
            db_log(spdlog::level::err, "Failed to create index: {}", error_msg);
            sqlite3_free(error_msg);
        }
    }
    
    db_log(spdlog::level::info, "Custom features database schema initialized");
}

Call this method from DatabaseManager constructor after existing init methods.

Add to DatabaseManager.hpp public section:

int get_cache_entry_count();
int64_t get_database_size();
std::pair<std::string, std::string> get_cache_date_range();
bool clear_all_cache();
bool clear_cache_older_than(int days);
bool optimize_database();
bool record_api_usage(const std::string& provider, int tokens, int requests, float cost, const std::string& model);
std::pair<int, int> get_api_usage_today(const std::string& provider);
bool save_undo_operation(const std::string& session_id, const std::string& operation_type, const std::string& file_moves_json);
std::optional<std::string> get_latest_undo_operation();

Implement each method in DatabaseManager.cpp with proper SQL queries and error handling.

Add to Types.hpp:

struct UserCharacteristic {
    std::string trait_name;
    std::string value;
    float confidence;
    std::string evidence;
};

struct FolderInsight {
    std::string folder_path;
    std::string description;
    int file_count;
    std::string last_analyzed;
};

Test by building and running, then checking tables exist with: sqlite3 ~/.local/share/aifilesorter/database.db ".tables"

Commit message: "Add database schema and helpers for custom features"
```

---

## PROMPT 2: Enhanced Undo System

Copy everything below to your AI agent:

```
Enhance the existing UndoManager to save undo plans to database persistently.

Files: app/include/UndoManager.hpp, app/lib/UndoManager.cpp, app/lib/MainApp.cpp
Database table: undo_history (already created in Prompt 1)

Modify UndoManager.hpp:
- Add member variable: DatabaseManager& db_manager_;
- Add to constructor signature: DatabaseManager& db
- Add methods:
  bool save_plan_to_database(const std::string& session_id);
  bool load_plan_from_database();
  bool has_persistent_undo() const;

In UndoManager.cpp implement:

std::string serialize_moves(const std::vector<FileMove>& moves) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < moves.size(); ++i) {
        auto escape = [](const std::string& s) {
            std::string result;
            for (char c : s) {
                if (c == '\"' || c == '\\') result += '\\';
                result += c;
            }
            return result;
        };
        oss << "{\"from\":\"" << escape(moves[i].from) 
            << "\",\"to\":\"" << escape(moves[i].to) << "\"}";
        if (i < moves.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

bool UndoManager::save_plan_to_database(const std::string& session_id) {
    std::string json = serialize_moves(undo_plan_);
    return db_manager_.save_undo_operation(session_id, "file_move", json);
}

bool UndoManager::load_plan_from_database() {
    auto json_opt = db_manager_.get_latest_undo_operation();
    if (json_opt) {
        undo_plan_ = deserialize_moves(json_opt.value());
        return true;
    }
    return false;
}

After creating undo plan, automatically save: save_plan_to_database(generate_session_id());

In MainApp.cpp:
- Update UndoManager constructor to pass db_manager_ reference
- In "Undo Last Run" menu handler, try loading from database if no in-memory undo

Test: Categorize files, close app, reopen, click Undo Last Run, verify files restored.

Commit message: "Enhance undo system with persistent storage"
```

---

## PROMPT 3: Dry Run Preview Mode

Copy everything below to your AI agent:

```
Add dry run preview mode to categorization dialog showing file moves without executing them.

Create new file app/include/DryRunPreviewDialog.hpp:

#ifndef DRYRUNPREVIEWDIALOG_HPP
#define DRYRUNPREVIEWDIALOG_HPP

#include <QDialog>
#include <QTableWidget>
#include <vector>
#include "Types.hpp"

class DryRunPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit DryRunPreviewDialog(const std::vector<CategorizedFile>& files,
                                const std::string& base_path,
                                QWidget* parent = nullptr);

private:
    void setup_ui(const std::vector<CategorizedFile>& files, const std::string& base_path);
    QString get_destination_path(const CategorizedFile& file, const std::string& base_path);
    QTableWidget* preview_table_;
};

#endif

Create app/lib/DryRunPreviewDialog.cpp with implementation:
- setWindowTitle("Dry Run Preview - No Files Will Be Moved")
- Create 3-column table: File Name, From, To
- Add orange warning label at top
- Populate table with file move previews
- Calculate destination paths: base_path/category/[subcategory]/filename
- Add Close button

Modify app/lib/CategorizationDialog.cpp:
- Add #include "DryRunPreviewDialog.hpp"
- Add checkbox: dry_run_checkbox_ = new QCheckBox(tr("Dry run (preview only, do not move files)"), this);
- In "Confirm & Sort" handler, before moving files:
  if (dry_run_checkbox_->isChecked()) {
      DryRunPreviewDialog preview(categorized_files_, base_path_, this);
      preview.exec();
      return;
  }

Add app/lib/DryRunPreviewDialog.cpp to CMakeLists.txt

Test: Analyze folder, check dry run, click Confirm & Sort, verify preview shows, no files moved.

Commit message: "Add dry run preview mode to categorization"
```

---

## PROMPT 4: Cache Manager Dialog

Copy everything below to your AI agent:

```
Create cache management dialog for viewing statistics and clearing cache.

Create app/include/CacheManagerDialog.hpp:

#ifndef CACHEMANAGERDIALOG_HPP
#define CACHEMANAGERDIALOG_HPP

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>

class DatabaseManager;

class CacheManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CacheManagerDialog(DatabaseManager& db, QWidget* parent = nullptr);

private slots:
    void on_clear_all_clicked();
    void on_clear_old_clicked();
    void on_optimize_clicked();
    void on_refresh_clicked();

private:
    void setup_ui();
    void update_statistics();
    QString format_bytes(int64_t bytes);
    
    DatabaseManager& db_;
    QLabel* entry_count_label_;
    QLabel* database_size_label_;
    QLabel* oldest_entry_label_;
    QLabel* newest_entry_label_;
    QPushButton* clear_all_button_;
    QPushButton* clear_old_button_;
    QPushButton* optimize_button_;
    QSpinBox* days_spinbox_;
};

#endif

Create app/lib/CacheManagerDialog.cpp:
- setup_ui: Two QGroupBoxes for Statistics and Operations
- Statistics: Show entry count, database size, oldest/newest entry dates
- Operations: Clear All button (red), Clear old with days spinner, Optimize Database button
- update_statistics: Call db methods to get current stats
- on_clear_all_clicked: Confirmation dialog then db_.clear_all_cache()
- on_clear_old_clicked: Get days from spinner, confirm, then db_.clear_cache_older_than(days)
- on_optimize_clicked: Call db_.optimize_database()
- format_bytes: Convert bytes to KB/MB/GB

In MainApp.cpp, add to Settings menu:
QAction* cache_action = settings_menu->addAction(tr("Manage Cache..."));
connect(cache_action, &QAction::triggered, this, [this]() {
    CacheManagerDialog dialog(*db_manager_, this);
    dialog.exec();
});

Add to CMakeLists.txt

Test: Settings → Manage Cache, verify stats, test clear operations.

Commit message: "Add cache manager dialog"
```

---

## PROMPT 5: Google Gemini API Client

Copy everything below to your AI agent:

```
Implement Google Gemini API client with rate limiting and retry logic for free tier.

Create app/include/GeminiClient.hpp:

#ifndef GEMINICLIENT_HPP
#define GEMINICLIENT_HPP

#include "ILLMClient.hpp"
#include <string>
#include <chrono>
#include <map>

class GeminiClient : public ILLMClient {
public:
    explicit GeminiClient(const std::string& api_key, const std::string& model = "gemini-1.5-flash");
    std::string categorize(const std::string& prompt) override;

private:
    struct RateLimitState {
        int tokens_available{15};
        std::chrono::steady_clock::time_point last_refill;
        int timeout_seconds{20};
        int consecutive_timeouts{0};
    };
    
    std::string make_request(const std::string& prompt);
    bool wait_for_rate_limit();
    void refill_tokens();
    void handle_timeout();
    void reset_timeout();
    
    std::string api_key_;
    std::string model_;
    static std::map<std::string, RateLimitState> rate_limit_states_;
};

#endif

Implement app/lib/GeminiClient.cpp:
- Token bucket rate limiting: 15 RPM, refill 1 token every 4 seconds
- Progressive timeout: starts at 20s, doubles on timeout up to 240s
- Exponential backoff retry: 3 retries max
- Use CURL for REST API: POST to https://generativelanguage.googleapis.com/v1/models/{model}:generateContent?key={api_key}
- Request payload: {"contents":[{"parts":[{"text":"prompt"}]}]}
- Parse response JSON to extract text from candidates[0].content.parts[0].text
- Handle timeout errors and retry with increased timeout
- Reset timeout on success

Integrate with LLMSelectionDialog:
- Add "Google Gemini" option
- Add API key input field
- Add model input field (default: gemini-1.5-flash)
- Store in Settings as gemini_api_key and gemini_model

Add to CMakeLists.txt

Test: Get free Gemini API key, enter in settings, categorize files, verify rate limiting works.

Commit message: "Add Google Gemini API integration with rate limiting"
```

---

## PROMPT 6: API Usage Tracking System

Copy everything below to your AI agent:

```
Implement API usage tracking and statistics display.

Create app/include/APIUsageTracker.hpp:

#ifndef APIUSAGETRACKER_HPP
#define APIUSAGETRACKER_HPP

#include "DatabaseManager.hpp"
#include <string>

class APIUsageTracker {
public:
    explicit APIUsageTracker(DatabaseManager& db);

    struct UsageStats {
        int tokens_used_today{0};
        int requests_today{0};
        float estimated_cost_today{0.0f};
        float estimated_cost_month{0.0f};
        int remaining_free_requests{0};
        std::string provider;
    };

    void record_request(const std::string& provider, int tokens, const std::string& model);
    UsageStats get_stats(const std::string& provider);
    static float estimate_cost(const std::string& model, int tokens);
    
    static constexpr int GEMINI_FREE_RPM = 15;
    static constexpr int GEMINI_FREE_RPD = 1500;

private:
    DatabaseManager& db_;
};

#endif

Implement app/lib/APIUsageTracker.cpp:
- record_request: Call db_.record_api_usage with current date
- get_stats: Query database for today's usage, calculate costs
- estimate_cost: Model pricing per 1M tokens (gpt-4o-mini: $0.15 input/$0.60 output, gemini-1.5-flash: FREE)

Create app/include/UsageStatsDialog.hpp:

#ifndef USAGESTATSDIALOG_HPP
#define USAGESTATSDIALOG_HPP

#include <QDialog>
#include "APIUsageTracker.hpp"

class UsageStatsDialog : public QDialog {
    Q_OBJECT
public:
    explicit UsageStatsDialog(DatabaseManager& db, QWidget* parent = nullptr);
private:
    void setup_ui();
    void update_stats();
};

#endif

Implement app/lib/UsageStatsDialog.cpp:
- Show tabs for OpenAI and Gemini
- Display: Today's tokens, Today's requests, Today's cost, Monthly estimate
- For Gemini: Show remaining free tier quota (15 RPM, 1500 RPD)
- Color warnings: >80% yellow, >95% red

Integrate with LLMClient and GeminiClient:
- After each API call, call tracker.record_request(provider, tokens, model)

Add menu in MainApp: Tools → API Usage Statistics

Add to CMakeLists.txt

Test: Make API calls, check Tools → API Usage Statistics, verify tracking.

Commit message: "Add API usage tracking and statistics display"
```

---

## PROMPT 7: User Profiling Core System

Copy everything below to your AI agent:

```
Implement user profiling manager that analyzes files and builds user profile.

Create app/include/UserProfileManager.hpp with class containing:
- initialize_profile(user_id)
- get_profile() → UserProfile
- analyze_and_update_from_folder(folder_path, files)
- get_folder_insight(folder_path)
- generate_user_context_for_llm() → string
- learn_organizational_template(folder_path, files)

Private methods:
- infer_characteristics_from_files(files, folder_path)
- extract_hobbies(files)
- detect_work_patterns(files)
- analyze_organization_style(files, folder_path)
- update_characteristic(trait_name, value, confidence, evidence)

Implement app/lib/UserProfileManager.cpp:

analyze_and_update_from_folder:
1. Calculate category distribution from files
2. Call infer_characteristics_from_files
3. Generate folder description
4. Determine usage pattern
5. Save folder insight to database

extract_hobbies:
- Map categories to hobbies (Photography → "Photography hobby", Music → "Music hobby")
- Calculate confidence based on file count / total files
- Store if confidence > 0.1

detect_work_patterns:
- Look for Documents, Presentations, Spreadsheets
- Check for Projects, Code categories
- Infer "Professional user", "Software Developer", etc.

analyze_organization_style:
- Count subcategories vs flat structure
- Infer "Detail-oriented" or "Minimalist"
- Calculate consistency across folders

generate_user_context_for_llm:
- Get top 10 characteristics by confidence
- Format as: "User profile: [hobby1], [hobby2], prefers [style] organization"
- Return string to inject in LLM prompts

learn_organizational_template:
- Extract common category patterns from folder
- Save as reusable template with confidence score
- Merge similar templates

Add to Types.hpp:

struct UserProfile {
    std::string user_id;
    std::vector<UserCharacteristic> characteristics;
    std::vector<FolderInsight> folder_insights;
};

Add to CMakeLists.txt

Test by calling analyze_and_update_from_folder with test files, check database.

Commit message: "Implement user profiling core system"
```

---

## PROMPT 8: User Profile UI Dialogs

Copy everything below to your AI agent:

```
Create UI dialogs for viewing and configuring user profile.

Create app/include/UserProfileDialog.hpp:

#ifndef USERPROFILEDIALOG_HPP
#define USERPROFILEDIALOG_HPP

#include <QDialog>
#include <QTabWidget>
#include "UserProfileManager.hpp"

class UserProfileDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserProfileDialog(UserProfileManager& profile_mgr, QWidget* parent = nullptr);
private:
    void setup_ui();
    QWidget* create_overview_tab();
    QWidget* create_characteristics_tab();
    QWidget* create_folders_tab();
    UserProfileManager& profile_mgr_;
};

#endif

Implement app/lib/UserProfileDialog.cpp:
- Three tabs: Overview, Characteristics, Folder Insights
- Overview: Summary with top characteristics, folder count, last analyzed
- Characteristics: Table with columns: Trait, Value, Confidence, Evidence
- Folder Insights: List of analyzed folders with stats
- Read-only, for viewing only

Create app/include/FolderLearningDialog.hpp:

#ifndef FOLDERLEARNINGDIALOG_HPP
#define FOLDERLEARNINGDIALOG_HPP

#include <QDialog>
#include <QRadioButton>

class FolderLearningDialog : public QDialog {
    Q_OBJECT
public:
    explicit FolderLearningDialog(const std::string& folder_path, 
                                 const std::string& current_level,
                                 QWidget* parent = nullptr);
    std::string get_selected_level() const;
private:
    QRadioButton* full_radio_;
    QRadioButton* partial_radio_;
    QRadioButton* none_radio_;
};

#endif

Implement app/lib/FolderLearningDialog.cpp:
- Three radio buttons: Full Learning, Partial Learning, No Learning
- Explanation labels for each option
- Full: Use profile for suggestions and store learnings
- Partial: Store learnings but don't use profile
- None: Keep this folder private
- OK/Cancel buttons

Add to CMakeLists.txt

Test by opening dialogs manually to verify display.

Commit message: "Add user profile UI dialogs"
```

---

## PROMPT 9: User Profiling Integration

Copy everything below to your AI agent:

```
Integrate user profiling with main application and categorization.

Modify app/lib/MainApp.cpp:

Add checkbox in main window:
learn_checkbox_ = new QCheckBox(tr("Learn from my organization patterns"), this);

Add settings button next to folder path selector:
settings_button_ = new QPushButton(QIcon(":/icons/settings"), "", this);
connect(settings_button_, &QPushButton::clicked, this, [this]() {
    std::string current_level = profile_manager_->get_folder_learning_level(folder_path_);
    FolderLearningDialog dialog(folder_path_, current_level, this);
    if (dialog.exec() == QDialog::Accepted) {
        profile_manager_->set_folder_learning_level(folder_path_, dialog.get_selected_level());
    }
});

Add Help menu item:
QAction* profile_action = help_menu->addAction(tr("View User Profile"));
connect(profile_action, &QAction::triggered, this, [this]() {
    UserProfileDialog dialog(*profile_manager_, this);
    dialog.exec();
});

Modify app/lib/CategorizationService.cpp:

In categorize_entries method, if learning enabled:
1. Get user context: std::string context = profile_manager_->generate_user_context_for_llm();
2. Inject in prompt: prompt += "\nUser context: " + context;
3. After categorization: profile_manager_->analyze_and_update_from_folder(folder_path, categorized_files);

Check folder learning level before storing data.

Add to CMakeLists.txt

Test end-to-end:
1. Enable learning checkbox
2. Categorize folder with photography files
3. View user profile, verify "Photography" characteristic
4. Categorize more, watch confidence increase
5. Test folder settings button

Commit message: "Integrate user profiling with categorization"
```

---

## PROMPT 10: File Tinder Tool

Copy everything below to your AI agent:

```
Create File Tinder tool for swipe-style file cleanup.

Create app/include/FileTinderDialog.hpp:

#ifndef FILETINDERDIALOG_HPP
#define FILETINDERDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <vector>
#include <string>
#include "DatabaseManager.hpp"

class FileTinderDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileTinderDialog(const std::string& folder_path,
                             DatabaseManager& db,
                             QWidget* parent = nullptr);
protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void on_keep_file();
    void on_delete_file();
    void on_ignore_file();
    void on_revert_decision();
    void on_finish_review();

private:
    enum class Decision { Pending, Keep, Delete, Ignore };
    
    struct FileToReview {
        std::string path;
        Decision decision{Decision::Pending};
        std::string file_name;
        int64_t file_size{0};
    };
    
    void load_files();
    void show_current_file();
    void preview_file(const std::string& path);
    void move_to_next_file();
    void show_review_screen();
    
    std::vector<FileToReview> files_;
    size_t current_index_{0};
    DatabaseManager& db_;
    std::string folder_path_;
    
    QLabel* preview_area_;
    QLabel* file_info_label_;
    QProgressBar* progress_bar_;
    QPushButton* keep_button_;
    QPushButton* delete_button_;
    QPushButton* ignore_button_;
    QPushButton* revert_button_;
};

#endif

Implement app/lib/FileTinderDialog.cpp:
- load_files: Scan folder, populate files_ vector
- show_current_file: Display file preview and info
- preview_file: Show image thumbnail or text preview
- Arrow key controls: → keep, ← delete, ↓ skip, ↑ revert
- Track decisions, save state to database
- show_review_screen: List all files marked for deletion, confirm before deleting
- Buttons match arrow keys for mouse users

Add menu in MainApp: Tools → File Tinder

Add to CMakeLists.txt

Test: Tools → File Tinder, navigate files, mark for deletion, review, execute.

Commit message: "Add File Tinder cleanup tool"
```

---

## PROMPT 11: Error Reporting System (Optional)

Copy everything below to your AI agent:

```
Create enhanced error reporting system that generates detailed error reports.

Create app/include/ErrorReporter.hpp:

#ifndef ERRORREPORTER_HPP
#define ERRORREPORTER_HPP

#include <string>
#include "ErrorCode.hpp"

class ErrorReporter {
public:
    static void report_error(ErrorCode code, const std::string& context, const std::string& details);
    static std::string generate_error_report(ErrorCode code, const std::string& context, const std::string& details);
    static void create_copilot_error_file(const std::string& report);
};

#endif

Create app/include/ErrorCode.hpp with enum of all error codes:

enum class ErrorCode {
    DATABASE_INIT_FAILED,
    LLM_CONNECTION_FAILED,
    FILE_MOVE_FAILED,
    CACHE_CLEAR_FAILED,
    // ... more error codes
};

Implement app/lib/ErrorReporter.cpp:
- report_error: Log error, optionally create error file
- generate_error_report: Format detailed markdown report with error code, timestamp, context, diagnostic info
- create_copilot_error_file: Create COPILOT_ERROR_{timestamp}.md file for easy bug reporting

Use throughout codebase for error handling.

Add to CMakeLists.txt

Test by triggering errors and checking generated error files.

Commit message: "Add enhanced error reporting system"
```

---

## PROMPT 12: Confidence Scoring Foundation (Future)

Copy everything below to your AI agent:

```
Set up foundation for confidence scoring system (implementation to be completed later).

Database table confidence_scores already exists from Prompt 1.

Add to app/include/CategorizationService.hpp:

struct ConfidenceScore {
    float category_confidence;
    float subcategory_confidence;
    std::string confidence_factors;
};

Add method:
ConfidenceScore calculate_confidence(const CategorizedFile& file);

Implement basic calculation in CategorizationService.cpp:
- If category from cache: confidence = 0.9
- If category from LLM: confidence = 0.7
- If subcategory exists: subcategory_confidence = 0.6
- Store factors: "source:cache" or "source:llm"

Save to database after categorization (optional for now).

This creates the foundation. Full UI and filtering by confidence to be added later.

Commit message: "Add confidence scoring foundation"
```

---

## PROMPT 13: Session Management Foundation (Future)

Copy everything below to your AI agent:

```
Set up foundation for session management (implementation to be completed later).

Database table categorization_sessions already exists from Prompt 1.

Add to app/include/CategorizationSession.hpp:

#ifndef CATEGORIZATIONSESSION_HPP
#define CATEGORIZATIONSESSION_HPP

#include <string>
#include "Types.hpp"

class CategorizationSession {
public:
    void save_session(const std::string& folder_path, const std::vector<CategorizedFile>& files);
    bool load_session(const std::string& session_id);
    std::vector<std::string> list_sessions();
};

#endif

Basic implementation:
- save_session: Serialize settings and progress to JSON, save to database
- load_session: Load from database, deserialize
- list_sessions: Query database for all sessions

This creates the foundation. Full resume/replay functionality to be added later.

Commit message: "Add session management foundation"
```

---

## PROMPT 14: User Corrections Learning Foundation (Future)

Copy everything below to your AI agent:

```
Set up foundation for learning from user corrections (implementation to be completed later).

Database table user_corrections already exists from Prompt 1.

Add to app/lib/CategorizationDialog.cpp:

When user manually changes a category in the review dialog:
- Record original category and corrected category
- Save to user_corrections table:
  db_->record_correction(file_path, original_category, corrected_category, original_subcategory, corrected_subcategory);

Add to UserProfileManager:
- Method to analyze corrections and update profile
- Look for patterns in corrections
- Adjust characteristic confidence based on corrections

This creates the foundation. Full learning algorithm to be added later.

Commit message: "Add user corrections learning foundation"
```

---

## USAGE INSTRUCTIONS

1. Fork hyperfield/ai-file-sorter on GitHub
2. Clone your fork locally
3. Copy Prompt 1 to your AI agent (GitHub Copilot, Claude, ChatGPT, etc.)
4. Review generated code, test, commit
5. Move to Prompt 2
6. Repeat until Prompt 11 (or 14 if doing future features)

Each prompt builds on previous ones. Follow the order.

Time estimate:
- Prompts 1-3: Week 1-2
- Prompts 4-6: Week 2-3
- Prompts 7-9: Week 3-5
- Prompts 10-11: Week 6
- Optional 12-14: Future

Total: 6-8 weeks for complete implementation.

