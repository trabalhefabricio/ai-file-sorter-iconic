# AI Implementation Prompts - Implementation Order
## Copy-Paste Ready Prompts for Re-forking

**Created:** January 7, 2026  
**Purpose:** Highly detailed AI prompts ordered by optimal implementation sequence  
**For:** Re-forking `hyperfield/ai-file-sorter` with all custom features

---

## 📖 How to Use

1. Fork `hyperfield/ai-file-sorter` → Clone locally
2. **Work through prompts IN THIS EXACT ORDER**
3. For each prompt: Copy entire text → Give to AI → Review code → Test → Commit
4. Each prompt builds on previous ones

**Total: 14 Prompts** | **Timeline: 6-8 weeks**

---

## Implementation Sequence

### 🔧 PHASE 1: Foundation (Week 1)
**[Prompt 1](#prompt-1)** - Database Schema & Helpers (4-6 hours)

### 🔄 PHASE 2: Modified Features First (Week 2)
**[Prompt 2](#prompt-2)** - Enhanced Undo System (3-4 hours)  
**[Prompt 3](#prompt-3)** - Dry Run Preview Mode (2-3 hours)

### ➕ PHASE 3: New Features - Simple (Week 2-3)
**[Prompt 4](#prompt-4)** - Cache Manager Dialog (3-4 hours)  
**[Prompt 5](#prompt-5)** - Gemini API Client (4-6 hours)  
**[Prompt 6](#prompt-6)** - API Usage Tracking (4-5 hours)

### ➕ PHASE 4: New Features - Complex (Week 3-5)
**[Prompt 7](#prompt-7)** - User Profiling Core (1-2 days)  
**[Prompt 8](#prompt-8)** - User Profile UI (1-2 days)  
**[Prompt 9](#prompt-9)** - Profiling Integration (1 day)

### 🛠️ PHASE 5: New Features - Utilities (Week 6)
**[Prompt 10](#prompt-10)** - File Tinder Tool (2-3 days)  
**[Prompt 11](#prompt-11)** - Error Reporting (2 days, optional)

### 🔮 PHASE 6: Yet-to-be-Implemented (Future)
**[Prompt 12](#prompt-12)** - Confidence Scoring (Foundation)  
**[Prompt 13](#prompt-13)** - Session Management (Foundation)  
**[Prompt 14](#prompt-14)** - User Corrections Learning (Foundation)

---

# Prompt 1: Database Schema & Helpers

**Time:** 4-6 hours | **Complexity:** MEDIUM | **Prerequisites:** Fresh fork only

```
TASK: Create Custom Features Database Schema

OBJECTIVE: Add 12 tables (8 for features, 4 for future) + helper methods to DatabaseManager

LOCATION: app/lib/DatabaseManager.cpp, app/include/DatabaseManager.hpp, app/include/Types.hpp

STEP 1: Add init_custom_features_tables() method to DatabaseManager.cpp

[Include complete CREATE TABLE SQL for all 12 tables with proper foreign keys and indices]

STEP 2: Add helper methods to DatabaseManager class

Public methods to add in DatabaseManager.hpp:
- int get_cache_entry_count()
- int64_t get_database_size()  
- bool clear_all_cache()
- bool clear_cache_older_than(int days)
- bool optimize_database()
- bool record_api_usage(provider, tokens, requests, cost, model)
- std::pair<int,int> get_api_usage_today(provider)
- bool save_undo_operation(session_id, operation_type, file_moves_json)
- std::optional<std::string> get_latest_undo_operation()

[Implement each with SQLite prepared statements, error handling, logging]

STEP 3: Add structs to Types.hpp

struct UserCharacteristic { trait_name, value, confidence, evidence };
struct FolderInsight { folder_path, description, file_count, last_analyzed };

TEST: Build → Run → Check tables exist with sqlite3 CLI

COMMIT: "Add database schema and helpers for custom features"
```

---

# Prompt 2: Enhanced Undo System

**Time:** 3-4 hours | **Complexity:** MEDIUM | **Prerequisites:** Prompt 1

```
TASK: Enhance UndoManager with Persistent Storage

OBJECTIVE: Save undo plans to database, enable undo after app restart

LOCATION: app/include/UndoManager.hpp, app/lib/UndoManager.cpp, app/lib/MainApp.cpp

STEP 1: Modify UndoManager.hpp
- Add member: DatabaseManager& db_manager_
- Add to constructor: DatabaseManager& db parameter
- Add methods:
  bool save_plan_to_database(const std::string& session_id)
  bool load_plan_from_database()

STEP 2: Implement in UndoManager.cpp

Constructor: Store db_manager_ reference

After creating undo plan successfully:
  save_plan_to_database(generate_session_id())

Serialization (JSON format):
  [{"from":"/path/file.txt","to":"/path/category/file.txt"},...]
  
Helper function:
std::string serialize_moves(const std::vector<FileMove>& moves) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < moves.size(); ++i) {
        oss << "{\"from\":\"" << escape_json(moves[i].from) 
            << "\",\"to\":\"" << escape_json(moves[i].to) << "\"}";
        if (i < moves.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

escape_json: Replace " with \" and \ with \\

save_plan_to_database:
  string json = serialize_moves(undo_plan_);
  return db_manager_.save_undo_operation(session_id, "file_move", json);

load_plan_from_database:
  auto json_opt = db_manager_.get_latest_undo_operation();
  if (json_opt) {
      undo_plan_ = deserialize_moves(json_opt.value());
      return true;
  }
  return false;

STEP 3: Update MainApp.cpp
- Pass db_manager_ to UndoManager constructor
- In "Undo Last Run" handler: if (!undo_manager_->has_undo()) undo_manager_->load_plan_from_database();

TEST:
1. Categorize files → Execute moves
2. Close app
3. Reopen app
4. Edit → Undo Last Run
5. Verify files restored

COMMIT: "Enhance undo system with persistent storage"
```

---

# Prompt 3: Dry Run Preview Mode

**Time:** 2-3 hours | **Complexity:** LOW-MEDIUM | **Prerequisites:** Prompts 1-2

```
TASK: Add Dry Run Preview to Categorization Dialog

OBJECTIVE: Preview file moves without executing them

LOCATION: New files + app/lib/CategorizationDialog.cpp

STEP 1: Create DryRunPreviewDialog.hpp

#ifndef DRYRUNPREVIEWDIALOG_HPP
#define DRYRUNPREVIEWDIALOG_HPP

#include <QDialog>
#include <QTableWidget>
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

STEP 2: Create DryRunPreviewDialog.cpp

Constructor: setWindowTitle("Dry Run Preview - No Files Will Be Moved"); resize(900, 600);

setup_ui:
  - Create QTableWidget with 3 columns: "File Name", "From", "To"
  - Add warning label (orange/bold): "Preview only - no files will be moved"
  - Populate rows with file data
  - For each file, calculate destination: base_path/category/[subcategory]/filename
  - Add Close button

get_destination_path:
  fs::path dest = base_path;
  dest /= file.category;
  if (!file.subcategory.empty()) dest /= file.subcategory;
  dest /= file.file_entry.file_name;
  return QString::fromStdString(dest.string());

STEP 3: Modify CategorizationDialog.cpp

Add include: #include "DryRunPreviewDialog.hpp"

In dialog layout, add checkbox:
  dry_run_checkbox_ = new QCheckBox(tr("Dry run (preview only, do not move files)"), this);

In "Confirm & Sort" button handler, BEFORE moving files:
  if (dry_run_checkbox_->isChecked()) {
      DryRunPreviewDialog preview(categorized_files_, base_path_, this);
      preview.exec();
      return;  // Don't execute moves
  }
  // ... existing move logic ...

STEP 4: Add to CMakeLists.txt: app/lib/DryRunPreviewDialog.cpp

TEST:
1. Analyze folder
2. Check "Dry run" checkbox
3. Click "Confirm & Sort!"
4. Verify preview dialog shows From→To paths
5. Verify NO files moved
6. Uncheck dry run → verify normal sorting works

COMMIT: "Add dry run preview mode to categorization"
```

---

# Prompt 4: Cache Manager Dialog

**Time:** 3-4 hours | **Complexity:** LOW-MEDIUM | **Prerequisites:** Prompts 1-3

```
TASK: Create Cache Manager Dialog

OBJECTIVE: UI for viewing cache statistics and managing cache

LOCATION: New files + app/lib/MainApp.cpp

STEP 1: Create CacheManagerDialog.hpp

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

STEP 2: Create CacheManagerDialog.cpp

Constructor: setWindowTitle("Cache Manager"); resize(500, 400); setup_ui(); update_statistics();

setup_ui:
  Two QGroupBoxes: "Cache Statistics" and "Cache Operations"
  
  Statistics section:
    - entry_count_label_ (Total cached entries: X)
    - database_size_label_ (Database size: X MB)
    - oldest_entry_label_ (Oldest entry: YYYY-MM-DD)
    - newest_entry_label_ (Newest entry: YYYY-MM-DD)
  
  Operations section:
    - Clear All Cache button (red background)
    - Clear cache older than [spinbox 1-365 days] days [Clear Old button]
    - Optimize Database (VACUUM) button
  
  Bottom: Refresh Statistics button + Close button

update_statistics:
  entry_count = db_.get_cache_entry_count();
  db_size = db_.get_database_size();
  date_range = db_.get_cache_date_range();
  Update all labels

on_clear_all_clicked:
  QMessageBox::question confirmation
  if Yes: db_.clear_all_cache() → update_statistics()

on_clear_old_clicked:
  days = days_spinbox_->value()
  QMessageBox::question confirmation
  if Yes: db_.clear_cache_older_than(days) → update_statistics()

on_optimize_clicked:
  db_.optimize_database() → QMessageBox::information → update_statistics()

format_bytes:
  < 1024: X B
  < 1024*1024: X KB
  < 1024*1024*1024: X MB
  else: X GB

STEP 3: Integrate with MainApp.cpp

In create_menus(), add to Settings menu:
  QAction* cache_action = settings_menu->addAction(tr("Manage Cache..."));
  connect(cache_action, &QAction::triggered, this, [this]() {
      CacheManagerDialog dialog(*db_manager_, this);
      dialog.exec();
  });

STEP 4: Add to CMakeLists.txt

TEST:
1. Settings → Manage Cache
2. Verify statistics display
3. Test Clear All (with confirmation)
4. Test Clear Old (try 30 days)
5. Test Optimize Database
6. Test Refresh Statistics

COMMIT: "Add cache manager dialog"
```

---

[Continue with Prompts 5-14 in same detailed format...]

**NOTE:** This is a condensed version showing the format. Each prompt includes:
- Exact time estimate
- Prerequisites
- Step-by-step implementation with code snippets
- Testing instructions
- Commit message

The complete document would include all 14 prompts following this same structure.

**Would you like me to:**
1. Complete all 14 prompts in this file?
2. Show you prompts 5-11 next (New Features)?
3. Create a separate file for Phase 6 (Yet-to-be-Implemented)?

