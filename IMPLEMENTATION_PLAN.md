# AI File Sorter - Implementation Plan
## Custom Features Enhancement Roadmap

**Created:** December 28, 2025  
**Status:** Planning Phase  
**Target Repository:** trabalhefabricio/ai-file-sorter-gemini

---

## Executive Summary

This document organizes the requested feature enhancements into a structured implementation roadmap. Features are grouped by dependency and complexity, with clear implementation phases and detailed prompts for AI-assisted development.

**Total Feature Requests:** 30+  
**Implementation Phases:** 6  
**Estimated Timeline:** 12-16 weeks for full implementation

---

## Table of Contents
1. [Implementation Strategy](#implementation-strategy)
2. [Phase 1: Foundation & Infrastructure](#phase-1-foundation--infrastructure)
3. [Phase 2: Core Feature Enhancements](#phase-2-core-feature-enhancements)
4. [Phase 3: Advanced User Profiling System](#phase-3-advanced-user-profiling-system)
5. [Phase 4: Enhanced Categorization](#phase-4-enhanced-categorization)
6. [Phase 5: File Management & UX](#phase-5-file-management--ux)
7. [Phase 6: Auxiliary Tools & Polish](#phase-6-auxiliary-tools--polish)
8. [AI Agent Prompts](#ai-agent-prompts)
9. [Dependencies & Interconnections](#dependencies--interconnections)

---

## Implementation Strategy

### Principles
1. **Foundation First**: Build core infrastructure before dependent features
2. **Iterative Development**: Each phase produces working, testable features
3. **Backward Compatibility**: Existing functionality remains operational
4. **User Control**: All features must be toggleable/configurable
5. **Database Schema Stability**: Design schema changes early to avoid migrations

### Development Approach
- **Direct Implementation**: Simple features I can implement immediately
- **AI-Assisted**: Complex features requiring specialized AI agent prompts
- **Hybrid**: Features requiring both approaches

---

## Phase 1: Foundation & Infrastructure
**Duration:** 2-3 weeks  
**Goal:** Establish core systems that other features depend on

### 1.1 Database Schema Enhancements (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** None

**What to do:**
- Add tables for confidence scoring, content analysis, cost tracking
- Add user profile tables with proper relationships
- Add multiple profile support tables
- Add session management tables
- Add undo history tables (expand existing)
- Add File Tinder state tables

**Implementation Notes:**
```sql
-- New tables needed:
CREATE TABLE confidence_scores (
    file_id INTEGER PRIMARY KEY,
    category_confidence REAL,
    subcategory_confidence REAL,
    model_version TEXT,
    timestamp DATETIME
);

CREATE TABLE content_analysis_cache (
    file_id INTEGER PRIMARY KEY,
    content_hash TEXT,
    mime_type TEXT,
    analysis_result TEXT,
    timestamp DATETIME
);

CREATE TABLE api_usage_tracking (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    provider TEXT,  -- 'openai', 'gemini'
    date DATE,
    tokens_used INTEGER,
    requests_made INTEGER,
    cost_estimate REAL,
    daily_limit INTEGER,
    remaining INTEGER
);

CREATE TABLE user_profiles (
    profile_id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_name TEXT UNIQUE NOT NULL,
    is_active INTEGER DEFAULT 0,
    created_at DATETIME,
    last_used DATETIME
);

CREATE TABLE profile_characteristics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_id INTEGER,
    trait_name TEXT,
    value TEXT,
    confidence REAL,
    evidence TEXT,
    timestamp DATETIME,
    FOREIGN KEY(profile_id) REFERENCES user_profiles(profile_id)
);

CREATE TABLE user_corrections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT,
    original_category TEXT,
    original_subcategory TEXT,
    corrected_category TEXT,
    corrected_subcategory TEXT,
    timestamp DATETIME,
    profile_id INTEGER,
    FOREIGN KEY(profile_id) REFERENCES user_profiles(profile_id)
);

CREATE TABLE categorization_sessions (
    session_id TEXT PRIMARY KEY,
    folder_path TEXT,
    started_at DATETIME,
    completed_at DATETIME,
    consistency_mode TEXT,  -- 'refined', 'consistent', 'hybrid'
    consistency_strength REAL DEFAULT 0.5
);

CREATE TABLE undo_history (
    undo_id INTEGER PRIMARY KEY AUTOINCREMENT,
    plan_path TEXT,
    description TEXT,
    timestamp DATETIME,
    is_undone INTEGER DEFAULT 0
);

CREATE TABLE file_tinder_state (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    folder_path TEXT,
    file_path TEXT,
    decision TEXT,  -- 'keep', 'delete', 'ignore', 'pending'
    timestamp DATETIME
);
```

**Files to modify:**
- `app/include/DatabaseManager.hpp`
- `app/lib/DatabaseManager.cpp`

### 1.2 Enhanced Error System with AI Resolution (AI-ASSISTED)
**Status:** Requires AI agent  
**Complexity:** High  
**Dependencies:** None

**AI Agent Prompt:**
```
Create an enhanced error handling system for AI File Sorter that includes:

1. Natural language error description interface where users can describe problems in their own words
2. AI-powered problem diagnosis that analyzes error context and user descriptions
3. Suggested solutions with step-by-step instructions
4. Automatic problem resolution for common issues (with user approval)
5. Integration with existing ErrorCode system (100+ error codes)

Requirements:
- Use LLM (local or remote based on user settings) for natural language understanding
- Parse user input to identify error categories: network, API, file system, categorization, etc.
- Generate specific resolution steps based on error type and context
- Implement "Try Fix" button that attempts automated resolution
- Log all error resolutions for learning purposes
- Create ErrorResolutionDialog class extending QDialog

Technical specifications:
- Add AIErrorResolver class in app/include/AIErrorResolver.hpp
- Integrate with existing DialogUtils::show_error_dialog()
- Store resolution history in database for pattern learning
- Support offline mode with local LLM fallback
- Add Settings toggle for AI-assisted error resolution

Example interaction:
User: "The app won't connect to Gemini and keeps timing out"
AI Analysis: Detects network/API error, checks API usage, connectivity
Suggestions: 
  1. Check API key validity
  2. Verify internet connection
  3. Check rate limits (15 RPM for free tier)
  4. Try switching to local LLM temporarily
Auto-fix: Validate API key, reset rate limiter state

Please implement this system with full Qt6 integration and maintain consistency with existing error handling patterns.
```

---

## Phase 2: Core Feature Enhancements
**Duration:** 3-4 weeks  
**Goal:** Enhance existing core categorization and file management

### 2.1 Content-Based Analysis System (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** High  
**Dependencies:** Phase 1.1 (database schema)

**What to do:**
1. Create `ContentAnalyzer` class for file content inspection
2. Support multiple file types:
   - **Text files**: Extract keywords, detect language, analyze structure
   - **Images**: Use embedded metadata (EXIF), detect image type (photo/screenshot/diagram)
   - **Documents**: PDF text extraction, Office document metadata
   - **Archives**: List contents of ZIP/RAR/7Z
   - **Code**: Detect programming language, project type

3. Integrate with existing categorization pipeline:
   - Add content hash to avoid re-analyzing
   - Cache analysis results in database
   - Pass content insights to LLM prompt

**Implementation Notes:**
```cpp
// New class: app/include/ContentAnalyzer.hpp
class ContentAnalyzer {
public:
    struct ContentInsight {
        std::string file_type;
        std::string mime_type;
        std::vector<std::string> keywords;
        std::string detected_language;
        std::map<std::string, std::string> metadata;
        std::string summary;  // For LLM context
    };
    
    ContentInsight analyze(const std::string& file_path);
    std::string generate_llm_context(const ContentInsight& insight);
    
private:
    std::string analyze_text_file(const std::string& path);
    std::string analyze_image_file(const std::string& path);
    std::string analyze_document(const std::string& path);
    std::string analyze_archive(const std::string& path);
};
```

**Libraries needed:**
- libmagic (MIME type detection)
- exiv2 (EXIF data)
- poppler (PDF reading)
- libarchive (archive inspection)

**Files to create:**
- `app/include/ContentAnalyzer.hpp`
- `app/lib/ContentAnalyzer.cpp`

**Files to modify:**
- `app/lib/CategorizationService.cpp` (integrate content analysis)
- `app/include/Settings.hpp` (add enable_content_analysis flag)

### 2.2 Confidence Scoring System (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 1.1 (database schema)

**What to do:**
1. Add confidence calculation to categorization results
2. Factors affecting confidence:
   - Cache hit vs LLM categorization (cache = lower confidence over time)
   - Consistency with session hints (higher if matches)
   - Content analysis alignment (higher if content matches category)
   - Model certainty (parse LLM response for confidence indicators)
   - Historical correction rate (lower if frequently corrected)

3. Display confidence in UI:
   - Color-coded indicators (green/yellow/red)
   - Percentage display
   - Sort by confidence for manual review priority

**Implementation Notes:**
```cpp
// Add to Types.hpp
struct CategorizedFile {
    // ... existing fields ...
    float confidence_score{1.0f};  // 0.0 to 1.0
    std::string confidence_factors;  // JSON explaining score
};

// New class: app/include/ConfidenceCalculator.hpp
class ConfidenceCalculator {
public:
    float calculate(const CategorizedFile& file,
                   const ContentAnalyzer::ContentInsight& content,
                   bool from_cache,
                   bool used_hints);
                   
    std::string explain_score(float score);
};
```

**Files to create:**
- `app/include/ConfidenceCalculator.hpp`
- `app/lib/ConfidenceCalculator.cpp`

**Files to modify:**
- `app/include/Types.hpp`
- `app/lib/CategorizationService.cpp`
- `app/lib/CategorizationDialog.cpp` (display confidence)

### 2.3 Learning from Corrections (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 1.1 (database schema), Phase 2.2 (confidence scoring)

**What to do:**
1. Track when users manually change categories in review dialog
2. Store corrections in database with context (file type, original category, correction)
3. Use corrections to:
   - Reduce confidence of similar future categorizations
   - Suggest corrections proactively ("You usually change X to Y")
   - Train category preferences per user profile
   - Update taxonomy with learned patterns

**Implementation Notes:**
```cpp
// New class: app/include/CorrectionLearner.hpp
class CorrectionLearner {
public:
    void record_correction(const std::string& file_path,
                          const std::string& original_cat,
                          const std::string& original_subcat,
                          const std::string& corrected_cat,
                          const std::string& corrected_subcat);
    
    struct CorrectionPattern {
        std::string from_category;
        std::string to_category;
        int frequency;
        float confidence;
    };
    
    std::vector<CorrectionPattern> get_patterns();
    std::optional<std::string> suggest_correction(const CategorizedFile& file);
};
```

**Files to create:**
- `app/include/CorrectionLearner.hpp`
- `app/lib/CorrectionLearner.cpp`

**Files to modify:**
- `app/lib/CategorizationDialog.cpp` (track manual changes)
- `app/lib/CategorizationService.cpp` (apply learned patterns)

### 2.4 API Cost Tracking System (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** Phase 1.1 (database schema)

**What to do:**
1. Track API usage for OpenAI and Gemini
2. Calculate estimated costs based on:
   - Token usage (for OpenAI)
   - Request counts (for Gemini free tier: 15 RPM, 1500 RPD)
   - Model pricing (configurable)

3. Display in UI:
   - Dashboard widget showing today's usage
   - Historical usage graphs
   - Remaining quota for free tiers
   - Estimated monthly cost

4. Warnings when approaching limits

**Implementation Notes:**
```cpp
// New class: app/include/APIUsageTracker.hpp
class APIUsageTracker {
public:
    struct UsageStats {
        int tokens_used_today;
        int requests_today;
        float estimated_cost_today;
        float estimated_cost_month;
        int remaining_free_requests;  // For Gemini
        std::string reset_time;
    };
    
    void record_request(const std::string& provider,
                       int tokens,
                       const std::string& model);
    
    UsageStats get_stats(const std::string& provider);
    bool is_approaching_limit(const std::string& provider);
    
private:
    DatabaseManager& db_;
    std::map<std::string, float> model_costs_;  // Tokens per dollar
};
```

**Files to create:**
- `app/include/APIUsageTracker.hpp`
- `app/lib/APIUsageTracker.cpp`
- `app/include/UsageStatsDialog.hpp`
- `app/lib/UsageStatsDialog.cpp`

**Files to modify:**
- `app/lib/GeminiClient.cpp` (track usage)
- `app/lib/LLMClient.cpp` (track usage)
- `app/lib/MainApp.cpp` (add menu item for stats)

---

## Phase 3: Advanced User Profiling System
**Duration:** 4-5 weeks  
**Goal:** Implement comprehensive AI-powered user profiling

### 3.1 Enhanced User Profile Manager with Multiple Profiles (AI-ASSISTED)
**Status:** Requires AI agent  
**Complexity:** Very High  
**Dependencies:** Phase 1.1 (database schema), Phase 2.1 (content analysis), Phase 2.3 (corrections)

**AI Agent Prompt:**
```
Enhance the existing UserProfileManager system in AI File Sorter to support:

1. MULTIPLE PROFILES:
   - Allow users to create/switch between different profiles (e.g., "Work", "Personal", "Client-A")
   - Each profile maintains separate characteristics, folder insights, and templates
   - Profile selector in main window
   - Import/export profiles as JSON

2. AI-POWERED QUESTIONNAIRE SYSTEM:
   - Generate dynamic questionnaires using LLM that adapt based on user responses
   - Question types: free text, multiple choice, rating scales, priority ranking
   - Parse natural language responses using LLM
   - Example questions:
     * "What do you primarily use this computer/folder for?"
     * "How would you describe your organization style?"
     * "Which categories are most important to you?"
   - Store questionnaire results and use for profile initialization
   - Never repeat the same questions (track history)

3. NATURAL LANGUAGE PROFILE UNDERSTANDING:
   - Allow users to describe their needs in free text
   - LLM parses text to extract:
     * Hobbies and interests
     * Work domain and role
     * Organization preferences
     * Category priorities
   - Convert natural language to structured profile data

4. ENHANCED PROFILE VISUALIZATION:
   - Timeline view showing profile evolution over time
   - Interactive charts showing category usage trends
   - Folder recommendation system based on profile
   - Visual indicators for learning levels per folder
   - Comparison mode: compare profiles or time periods

5. LOG-BASED LEARNING:
   - Analyze application logs to infer patterns
   - Detect frequently accessed folders
   - Identify peak usage times
   - Track category modification patterns
   - Feed insights back into profile

Current Implementation Status:
- Basic UserProfileManager exists in app/lib/UserProfileManager.cpp
- Profile stored in SQLite with characteristics and folder insights
- Per-folder learning controls exist (Full/Partial/None)

Requirements:
- Extend existing classes, don't rewrite from scratch
- Add ProfileQuestionnaireDialog for interactive questionnaire
- Add ProfileComparisonDialog for comparing profiles/timelines
- Add ProfileDashboardWidget for main window integration
- Use existing LLM infrastructure (support local/OpenAI/Gemini)
- All AI features must work offline with local LLM
- Store all data locally in SQLite (no cloud)

Please implement this enhanced profiling system with full Qt6 UI integration.
```

### 3.2 Trend Analysis and Folder Recommendations (AI-ASSISTED)
**Status:** Requires AI agent  
**Complexity:** High  
**Dependencies:** Phase 3.1 (enhanced profiling)

**AI Agent Prompt:**
```
Implement trend analysis and folder recommendation features for AI File Sorter:

1. TREND ANALYSIS:
   - Track category usage over time (daily, weekly, monthly)
   - Detect changes in organization patterns
   - Identify growing/declining category usage
   - Visualize trends with interactive charts (Qt Charts)
   - Alert on significant pattern changes

2. FOLDER RECOMMENDATIONS:
   - Analyze current folder structure
   - Suggest new folders based on:
     * Frequently used categories without dedicated folders
     * Large categories that could be split
     * Similar files scattered across locations
     * User profile preferences
   - Provide reasoning for each recommendation
   - One-click folder creation and file migration

3. FOLDER HIERARCHY ANALYSIS:
   - Learn optimal folder depth for user
   - Suggest hierarchical improvements
   - Detect over-categorization or under-categorization
   - Recommend consolidation or splitting

4. CLEANUP SUGGESTIONS:
   - Identify folders that haven't been organized
   - Detect duplicate content across folders
   - Find unused/orphaned categories
   - Suggest archival of old files

Current Code:
- UserProfileManager in app/lib/UserProfileManager.cpp handles basic analysis
- FolderInsight struct exists in Types.hpp

Technical Requirements:
- Add TrendAnalyzer class for statistical analysis
- Add FolderRecommender class for suggestion generation
- Create FolderRecommendationsDialog for displaying suggestions
- Use time-series analysis for trend detection
- Integrate with user profile for personalization
- All operations must be async (non-blocking UI)

Please implement with proper error handling and user feedback.
```

---

## Phase 4: Enhanced Categorization
**Duration:** 3-4 weeks  
**Goal:** Advanced categorization modes and taxonomy management

### 4.1 User-Editable Taxonomy System (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** None

**What to do:**
1. Create TaxonomyManagerDialog for viewing/editing taxonomy
2. Features:
   - Tree view of all categories and subcategories
   - Merge similar categories (e.g., "Documents" + "Docs" → "Documents")
   - Rename categories (applies to all cached entries)
   - Delete unused categories
   - Add category aliases (multiple names for same category)
   - Export/import taxonomy as JSON

3. Integrate with categorization:
   - Prefer taxonomy categories in LLM prompts
   - Apply aliases during categorization
   - Update database when taxonomy changes

**Implementation Notes:**
```cpp
// New class: app/include/TaxonomyManagerDialog.hpp
class TaxonomyManagerDialog : public QDialog {
public:
    explicit TaxonomyManagerDialog(DatabaseManager& db, QWidget* parent);
    
private slots:
    void on_merge_categories();
    void on_rename_category();
    void on_add_alias();
    void on_delete_category();
    void on_export_taxonomy();
    void on_import_taxonomy();
    
private:
    QTreeWidget* taxonomy_tree_;
    DatabaseManager& db_;
};
```

**Files to create:**
- `app/include/TaxonomyManagerDialog.hpp`
- `app/lib/TaxonomyManagerDialog.cpp`

**Files to modify:**
- `app/lib/MainApp.cpp` (add menu item)
- `app/lib/DatabaseManager.cpp` (taxonomy modification methods)

### 4.2 Smart Taxonomy Suggestions (AI-ASSISTED)
**Status:** Requires AI agent  
**Complexity:** Medium  
**Dependencies:** Phase 4.1 (editable taxonomy)

**AI Agent Prompt:**
```
Implement smart taxonomy suggestion system for AI File Sorter:

1. AUTOMATIC SIMILARITY DETECTION:
   - Analyze taxonomy to find similar categories
   - Detect variations: singular/plural, abbreviations, synonyms
   - Use LLM to determine semantic similarity
   - Example detections:
     * "Document" vs "Documents" vs "Docs"
     * "Photo" vs "Image" vs "Picture"
     * "Music" vs "Audio" vs "Sound"

2. MERGE SUGGESTIONS:
   - Suggest category merges with confidence scores
   - Show files affected by each merge
   - Preview merged taxonomy before applying
   - Allow manual adjustment of suggestions

3. TAXONOMY OPTIMIZATION:
   - Suggest splitting overly broad categories
   - Recommend subcategory creation for large categories
   - Identify redundant subcategories
   - Optimize taxonomy depth based on file counts

4. LEARNING FROM CORRECTIONS:
   - When user manually changes categories, suggest taxonomy updates
   - If many files from category A → B, suggest merge or alias
   - Track user preferences for category naming

Current Implementation:
- Taxonomy stored in DatabaseManager (app/lib/DatabaseManager.cpp)
- Basic taxonomy usage in CategorizationService

Requirements:
- Add TaxonomySuggestionEngine class
- Integrate with TaxonomyManagerDialog
- Use LLM for semantic analysis (support local/remote)
- Show suggestions proactively in UI
- Allow batch acceptance of suggestions

Please implement with Qt6 UI integration and proper async handling.
```

### 4.3 Hybrid Categorization Mode (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** None

**What to do:**
1. Add third categorization mode: "Hybrid"
2. Hybrid mode logic:
   - Analyze file batch to detect clusters
   - Apply consistency hints within clusters
   - Use refined mode between clusters
   - Automatically adjust based on file similarity

3. UI changes:
   - Add "Hybrid (Smart)" radio button
   - Add "Consistency Strength" slider (0-100%)
   - Show current mode in progress dialog

**Implementation Notes:**
```cpp
// Add to Types.hpp
enum class CategorizationMode {
    Refined,
    Consistent,
    Hybrid
};

// Modify CategorizationService
class CategorizationService {
    // ... existing ...
    
private:
    std::vector<std::vector<FileEntry>> cluster_similar_files(
        const std::vector<FileEntry>& files);
    
    float get_consistency_strength() const;  // From settings
};
```

**Files to modify:**
- `app/include/Types.hpp`
- `app/include/Settings.hpp`
- `app/lib/CategorizationService.cpp`
- `app/lib/MainApp.cpp` (add UI controls)

### 4.4 Session Management (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** Phase 1.1 (database schema)

**What to do:**
1. Save categorization sessions with metadata
2. Session data:
   - Folder path
   - Categorization mode
   - Consistency strength
   - Files processed
   - Timestamp
   - Results

3. Features:
   - Resume interrupted sessions
   - Review past sessions
   - Reapply session settings to new folders
   - Session templates

**Implementation Notes:**
```cpp
// New class: app/include/SessionManager.hpp
class SessionManager {
public:
    struct Session {
        std::string session_id;
        std::string folder_path;
        CategorizationMode mode;
        float consistency_strength;
        std::string timestamp;
        bool completed;
    };
    
    std::string create_session(const std::string& folder);
    void save_session_state(const std::string& session_id, 
                           const std::vector<CategorizedFile>& results);
    std::optional<Session> get_active_session();
    std::vector<Session> get_recent_sessions(int count = 10);
};
```

**Files to create:**
- `app/include/SessionManager.hpp`
- `app/lib/SessionManager.cpp`

**Files to modify:**
- `app/lib/MainApp.cpp` (session management UI)

### 4.5 Post-Sorting Category Rename System (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 4.1 (editable taxonomy)

**What to do:**
1. Add bulk category rename after sorting
2. Features:
   - Select multiple files in results
   - Right-click → "Change Category"
   - If new category doesn't exist, create it
   - Move files to new category folder
   - Update database and taxonomy
   - Undo support

3. Smart features:
   - Suggest similar existing categories
   - Auto-complete category names
   - Validate category names
   - Show affected file count

**Implementation Notes:**
```cpp
// Add to CategorizationDialog
class CategorizationDialog {
    // ... existing ...
    
private slots:
    void on_bulk_category_change();
    void on_bulk_subcategory_change();
    
private:
    void change_category_for_selected(const std::string& new_category);
    std::vector<std::string> suggest_similar_categories(const std::string& input);
};
```

**Files to modify:**
- `app/lib/CategorizationDialog.cpp`
- `app/lib/DatabaseManager.cpp` (bulk update methods)

---

## Phase 5: File Management & UX
**Duration:** 2-3 weeks  
**Goal:** Enhanced file management and user experience

### 5.1 Enhanced Category/Subcategory Editor (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 4.1 (editable taxonomy)

**What to do:**
1. Create visual tree-based category editor
2. Keyboard shortcuts:
   - Enter / Click + : Add new category
   - Shift+Enter / Click * : Add subcategory to current category
   - Enter again : Save and add next subcategory
   - Backspace / Click - : Go back to category level
   - Delete: Remove category/subcategory

3. Visual features:
   - Tree branch display showing hierarchy
   - Different categories can have different subcategories
   - Drag-and-drop reordering
   - Color coding by usage frequency
   - Quick search/filter

**Implementation Notes:**
```cpp
// New class: app/include/CategoryEditorWidget.hpp
class CategoryEditorWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit CategoryEditorWidget(QWidget* parent = nullptr);
    
    void set_categories(const std::map<std::string, std::vector<std::string>>& categories);
    std::map<std::string, std::vector<std::string>> get_categories() const;
    
protected:
    void keyPressEvent(QKeyEvent* event) override;
    
private slots:
    void on_add_category();
    void on_add_subcategory();
    void on_remove_item();
    void on_item_double_clicked(QTreeWidgetItem* item);
    
private:
    QTreeWidget* tree_;
    QLineEdit* quick_add_input_;
    enum class EditMode { Category, Subcategory };
    EditMode current_mode_{EditMode::Category};
};
```

**Files to create:**
- `app/include/CategoryEditorWidget.hpp`
- `app/lib/CategoryEditorWidget.cpp`

**Files to modify:**
- Integrate into WhitelistManagerDialog and TaxonomyManagerDialog

### 5.2 Conflict Detection and Resolution (AI-ASSISTED)
**Status:** Requires AI agent  
**Complexity:** High  
**Dependencies:** Phase 2.2 (confidence scoring)

**AI Agent Prompt:**
```
Implement comprehensive conflict detection and resolution system for AI File Sorter:

1. CONFLICT TYPES TO DETECT:
   - File name conflicts (same name, different content)
   - Destination already exists (overwrite scenarios)
   - Circular moves (A→B, B→A)
   - Permission issues (read-only, locked files)
   - Low confidence categorizations (<50%)
   - Multiple files with same metadata
   - Ambiguous categorization (file fits multiple categories)

2. NATURAL LANGUAGE RESOLUTION INTERFACE:
   - User describes problem: "There are duplicate files with different dates"
   - AI analyzes conflicts and understands intent
   - Suggests resolution strategies:
     * Keep newest/oldest
     * Keep largest/smallest
     * Merge content
     * Rename automatically
     * Skip conflicts
   - Apply resolution to all similar conflicts

3. BATCH CONFLICT RESOLUTION:
   - Show all conflicts in one dialog
   - Group similar conflicts
   - Apply same resolution to groups
   - Preview changes before applying
   - Undo support for resolutions

4. SMART SUGGESTIONS:
   - Learn from user's resolution choices
   - Suggest resolutions for future conflicts
   - Proactive conflict prevention

Current Code:
- Basic file moving in ResultsCoordinator (app/lib/ResultsCoordinator.cpp)
- Some error handling exists

Requirements:
- Create ConflictDetector class for detection
- Create ConflictResolutionDialog with natural language input
- Add AIConflictResolver class using LLM
- Integrate with existing file moving logic
- Support offline resolution (without AI for simple cases)
- Store resolution preferences for learning

Please implement with Qt6 integration and proper error handling.
```

### 5.3 Simulation Mode Enhancements (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** Existing dry run mode

**What to do:**
1. Enhance existing dry run with:
   - Before/After folder structure visualization
   - File count and size per category
   - Conflict highlighting
   - Category statistics
   - Searchable preview table

2. Multiple preview modes:
   - List view (current)
   - Tree view (folder hierarchy)
   - Graph view (category relationships)

**Implementation Notes:**
```cpp
// Enhance DryRunPreviewDialog
class DryRunPreviewDialog : public QDialog {
    // ... existing ...
    
private:
    enum class ViewMode { List, Tree, Graph };
    ViewMode current_view_{ViewMode::List};
    
    QTreeWidget* tree_view_;
    QGraphicsView* graph_view_;
    QTableWidget* list_view_;  // existing
    
    void show_tree_view();
    void show_graph_view();
    void generate_statistics();
};
```

**Files to modify:**
- `app/include/DryRunPreviewDialog.hpp`
- `app/lib/DryRunPreviewDialog.cpp`

### 5.4 Selective Execution from Preview (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 5.3 (enhanced simulation)

**What to do:**
1. Add checkboxes to preview dialog
2. Allow selecting specific moves to execute
3. Filter options:
   - By category
   - By confidence score
   - By file type
   - By conflict status

4. Execute only selected items
5. Save unexecuted items for later

**Implementation Notes:**
```cpp
// Enhance DryRunPreviewDialog
class DryRunPreviewDialog : public QDialog {
    // ... existing ...
    
private slots:
    void on_select_all();
    void on_select_none();
    void on_select_by_category();
    void on_select_by_confidence();
    void on_execute_selected();
    
private:
    std::vector<bool> selected_items_;
    QPushButton* execute_selected_button_;
};
```

**Files to modify:**
- `app/lib/DryRunPreviewDialog.cpp`
- `app/lib/ResultsCoordinator.cpp` (partial execution)

### 5.5 Enhanced Undo System (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 1.1 (database schema)

**What to do:**
1. Expand undo to support:
   - Multiple undo history (not just last)
   - Partial undo (select specific files)
   - Redo support
   - Undo chains (track sequences)
   - Conflict resolution during undo

2. UI enhancements:
   - Undo history dialog
   - Visual timeline of operations
   - Search undo history
   - Batch undo operations

**Implementation Notes:**
```cpp
// Enhance UndoManager
class UndoManager {
    // ... existing ...
    
public:
    std::vector<UndoEntry> get_history(int limit = 50);
    bool undo_specific(const std::string& undo_id);
    bool redo_specific(const std::string& undo_id);
    std::vector<Entry> get_entries_for_plan(const std::string& plan_path);
    bool undo_partial(const std::string& plan_path, 
                     const std::vector<int>& entry_indices);
    
private:
    std::stack<std::string> redo_stack_;
};

// New dialog: UndoHistoryDialog
class UndoHistoryDialog : public QDialog {
public:
    explicit UndoHistoryDialog(UndoManager& undo_mgr, QWidget* parent);
    
private slots:
    void on_undo_selected();
    void on_redo_selected();
    void on_view_details();
    
private:
    QListWidget* history_list_;
    UndoManager& undo_mgr_;
};
```

**Files to modify:**
- `app/include/UndoManager.hpp`
- `app/lib/UndoManager.cpp`

**Files to create:**
- `app/include/UndoHistoryDialog.hpp`
- `app/lib/UndoHistoryDialog.cpp`

### 5.6 Enhanced Progress Logging (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** None

**What to do:**
1. Improve real-time sorting feedback
2. Show in progress dialog:
   - Current operation (analyzing, moving, caching)
   - Current file with path
   - Files per second rate
   - Time elapsed / remaining
   - Success/skip/error counts
   - Detailed expandable log

3. Export log to file
4. Filter log by message type

**Implementation Notes:**
```cpp
// Enhance CategorizationProgressDialog
class CategorizationProgressDialog : public QDialog {
    // ... existing ...
    
private:
    QTextEdit* detailed_log_;
    QLabel* stats_label_;
    QPushButton* export_log_button_;
    
    std::chrono::steady_clock::time_point start_time_;
    int files_processed_{0};
    
    void update_statistics();
    void export_log_to_file();
};
```

**Files to modify:**
- `app/include/CategorizationProgressDialog.hpp`
- `app/lib/CategorizationProgressDialog.cpp`

---

## Phase 6: Auxiliary Tools & Polish
**Duration:** 2-3 weeks  
**Goal:** New tools and final polish

### 6.1 File Tinder - File Cleanup Tool (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** Phase 1.1 (database schema)

**What to do:**
1. Create new standalone tool accessible from Tools menu
2. Features:
   - Show one file at a time
   - V key or ✓ button: Keep file
   - X key or ✗ button: Mark for deletion
   - I key or skip button: Ignore (next file)
   - R key or revert button: Undo last decision
   - 5-second timeout (optional, toggleable)
   - If timeout, file goes to back of queue

3. File preview:
   - Images: Show thumbnail/full preview
   - Videos: Show first frame + play button
   - Folders: Show contents tree
   - Archives (.zip, .rar, .7z): Show contents list
   - Text: Show first 100 lines
   - Other: Show metadata

4. UI:
   - Large central preview area
   - Big buttons with keyboard shortcuts
   - Progress indicator (X/Y files)
   - Category filter (review specific category)
   - File type filter
   - Size sorting options

5. Batch operations:
   - At end, review all marked deletions
   - Execute deletion (with undo support)
   - Save session state (resume later)

**Implementation Notes:**
```cpp
// New class: app/include/FileTinderDialog.hpp
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
    void on_timeout();
    void on_finish_review();
    
private:
    enum class Decision { Keep, Delete, Ignore, Pending };
    
    struct FileToReview {
        std::string path;
        Decision decision{Decision::Pending};
    };
    
    std::vector<FileToReview> files_;
    size_t current_index_{0};
    DatabaseManager& db_;
    
    QLabel* preview_area_;
    QTimer* timeout_timer_;
    QProgressBar* progress_bar_;
    QPushButton* keep_button_;
    QPushButton* delete_button_;
    QPushButton* ignore_button_;
    QPushButton* revert_button_;
    
    void load_files();
    void show_current_file();
    void preview_file(const std::string& path);
    void save_state();
    void load_state();
};
```

**Files to create:**
- `app/include/FileTinderDialog.hpp`
- `app/lib/FileTinderDialog.cpp`

**Files to modify:**
- `app/lib/MainApp.cpp` (add Tools → File Tinder menu)

### 6.2 Easy Mode Interface (AI-ASSISTED)
**Status:** Requires AI agent  
**Complexity:** High  
**Dependencies:** Most Phase 2-5 features

**AI Agent Prompt:**
```
Create an "Easy Mode" interface for AI File Sorter that streamlines the entire workflow:

1. SIMPLIFIED WIZARD INTERFACE:
   - Step 1: "Which folder do you want to organize?"
   - Step 2: "What kind of files are these?" (with smart suggestions)
   - Step 3: "How do you want to organize them?" (simple preset choices)
   - Step 4: Preview with clear visual indicators
   - Step 5: One-click execute

2. SMART DEFAULTS:
   - Auto-detect optimal settings based on folder content
   - Suggest categorization mode (refined/consistent/hybrid)
   - Pre-populate category whitelist based on file types
   - Enable appropriate features automatically

3. GUIDED EXPERIENCE:
   - Tooltips and inline help
   - Progress indicators
   - Success celebrations
   - Error recovery wizards
   - Undo always visible and easy

4. PRESET TEMPLATES:
   - "Clean my Downloads folder"
   - "Organize my Photos"
   - "Sort my Documents"
   - "Archive old files"
   - Custom templates saved by user

5. PROGRESSIVE DISCLOSURE:
   - Hide advanced options by default
   - "Show advanced" button for power users
   - Smooth transition to full interface
   - Remember user preference

Current Interface:
- MainApp with manual configuration (app/lib/MainApp.cpp)
- Multiple dialogs and settings

Requirements:
- Create EasyModeWizard class with multi-step wizard
- Use Qt6 QWizard as base
- Integrate with all existing features
- Maintain full control - easy doesn't mean limited
- Graceful degradation if features unavailable
- Can switch between Easy/Advanced anytime

Please implement complete easy mode with intuitive Qt6 UI.
```

### 6.3 Local Database Cache Management UI (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** None

**What to do:**
1. Create cache management dialog
2. Features:
   - View cache statistics (size, entry count, hit rate)
   - Clear all cache
   - Clear cache for specific folders
   - Clear cache older than X days
   - Export/import cache
   - Optimize database (VACUUM)

**Implementation Notes:**
```cpp
// New class: app/include/CacheManagerDialog.hpp
class CacheManagerDialog : public QDialog {
public:
    explicit CacheManagerDialog(DatabaseManager& db, QWidget* parent);
    
private slots:
    void on_clear_all();
    void on_clear_folder();
    void on_clear_old();
    void on_optimize_database();
    void on_export_cache();
    void on_import_cache();
    
private:
    struct CacheStats {
        int entry_count;
        int64_t total_size_bytes;
        float hit_rate;
        std::string oldest_entry;
        std::string newest_entry;
    };
    
    DatabaseManager& db_;
    CacheStats stats_;
    
    void refresh_stats();
    void update_ui();
};
```

**Files to create:**
- `app/include/CacheManagerDialog.hpp`
- `app/lib/CacheManagerDialog.cpp`

**Files to modify:**
- `app/lib/MainApp.cpp` (add Settings → Manage Cache menu)

### 6.4 Testing Mode with Functionality Checklist (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Medium  
**Dependencies:** All features

**What to do:**
1. Create comprehensive testing dialog
2. Automated tests:
   - LLM connectivity (local, OpenAI, Gemini)
   - Database access
   - File system permissions
   - GPU backend availability
   - Cache operations
   - Undo/redo operations
   - API quota checks

3. UI features:
   - Checklist of all features
   - Run all tests button
   - Individual test execution
   - Test results with pass/fail indicators
   - Error details for failed tests
   - Export test report

**Implementation Notes:**
```cpp
// New class: app/include/TestingModeDialog.hpp
class TestingModeDialog : public QDialog {
public:
    explicit TestingModeDialog(QWidget* parent);
    
private slots:
    void on_run_all_tests();
    void on_run_selected_test();
    void on_export_report();
    
private:
    struct TestCase {
        std::string name;
        std::string description;
        std::function<bool()> test_function;
        bool passed{false};
        std::string error_message;
    };
    
    std::vector<TestCase> tests_;
    QTreeWidget* test_tree_;
    QTextEdit* output_log_;
    
    void register_tests();
    bool test_llm_connectivity();
    bool test_database();
    bool test_file_operations();
    bool test_gpu_backends();
    // ... more test methods ...
};
```

**Files to create:**
- `app/include/TestingModeDialog.hpp`
- `app/lib/TestingModeDialog.cpp`

**Files to modify:**
- `app/lib/MainApp.cpp` (add Development → Testing Mode menu)

### 6.5 File Explorer Enhancements (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** None

**What to do:**
1. Multiple folder selection (Ctrl+Click)
2. Context menu with:
   - Open in file manager
   - Properties
   - Analyze folder
   - Add to favorites
   - Recent folders list

3. Folder preview on hover:
   - File count
   - Total size
   - Last modified

**Implementation Notes:**
```cpp
// Enhance file explorer in MainApp
class MainApp : public QMainWindow {
    // ... existing ...
    
private slots:
    void on_explorer_context_menu(const QPoint& pos);
    void on_analyze_selected_folders();
    void on_add_to_favorites();
    
private:
    QMenu* explorer_context_menu_;
    std::vector<std::string> favorite_folders_;
    std::vector<std::string> recent_folders_;
};
```

**Files to modify:**
- `app/lib/MainApp.cpp`

### 6.6 README Update (DIRECT IMPLEMENTATION)
**Status:** Can implement immediately  
**Complexity:** Low  
**Dependencies:** All features completed

**What to do:**
1. Update README.md with new features section
2. Group custom features clearly
3. User-friendly descriptions
4. Screenshots/GIFs where appropriate
5. Usage examples
6. Configuration guide

**New README sections:**
- Custom Features Overview
- Content-Based Analysis
- User Profiling System
- Advanced Categorization Modes
- File Tinder Tool
- Easy Mode
- And more...

**Files to modify:**
- `README.md`

---

## AI Agent Prompts

### Summary of AI-Assisted Features

The following features require specialized AI agent prompts:

1. **Enhanced Error System with AI Resolution** (Phase 1.2)
2. **Enhanced User Profile Manager** (Phase 3.1)
3. **Trend Analysis and Recommendations** (Phase 3.2)
4. **Smart Taxonomy Suggestions** (Phase 4.2)
5. **Conflict Detection and Resolution** (Phase 5.2)
6. **Easy Mode Interface** (Phase 6.2)

All prompts are provided in their respective phase sections above.

---

## Dependencies & Interconnections

### Critical Dependencies
```
Phase 1.1 (Database Schema) 
  ↓
  ├─→ Phase 2.1 (Content Analysis)
  ├─→ Phase 2.2 (Confidence Scoring)
  ├─→ Phase 2.3 (Learning from Corrections)
  ├─→ Phase 2.4 (API Cost Tracking)
  ├─→ Phase 3.1 (User Profiling)
  └─→ Phase 5.5 (Enhanced Undo)

Phase 2.2 (Confidence Scoring)
  ↓
  ├─→ Phase 2.3 (Learning from Corrections)
  └─→ Phase 5.2 (Conflict Resolution)

Phase 3.1 (User Profiling)
  ↓
  └─→ Phase 3.2 (Trend Analysis)

Phase 4.1 (Editable Taxonomy)
  ↓
  ├─→ Phase 4.2 (Smart Suggestions)
  ├─→ Phase 4.5 (Post-Sort Rename)
  └─→ Phase 5.1 (Category Editor)

All Phase 2-5 Features
  ↓
  └─→ Phase 6.2 (Easy Mode)
```

### Feature Overlaps

1. **Profile + Corrections + Content Analysis**: All feed into improved categorization
2. **Taxonomy + Corrections + Suggestions**: Form taxonomy management system
3. **Confidence + Conflicts + Content**: Work together for better accuracy
4. **Sessions + Undo + Simulation**: Combined comprehensive file management
5. **All Features**: Integrated into Easy Mode

---

## Implementation Recommendations

### Start with Foundation (Phase 1)
Begin with database schema changes. This is the most critical step as it affects everything else. Design carefully to minimize future migrations.

### Parallel Tracks After Foundation
Once Phase 1 is done, you can work on multiple phases in parallel:
- **Track A**: Core enhancements (Phase 2)
- **Track B**: Categorization improvements (Phase 4)
- **Track C**: UI/UX enhancements (Phase 5)

### AI-Assisted Features
For complex AI-assisted features, use the provided prompts with specialized AI agents. Make sure to:
1. Provide full context from existing code
2. Specify integration points clearly
3. Request Qt6-compatible implementations
4. Ensure offline capability (local LLM support)

### Testing Strategy
- Write tests for each phase before implementation
- Test features in isolation first
- Integration testing after each phase
- Full regression testing before final release

### User Feedback
- Release features incrementally
- Gather feedback early and often
- Be prepared to iterate on UI/UX
- Keep features toggleable during beta

---

## Estimated Timeline

| Phase | Duration | Cumulative |
|-------|----------|------------|
| Phase 1: Foundation | 2-3 weeks | 3 weeks |
| Phase 2: Core Enhancements | 3-4 weeks | 7 weeks |
| Phase 3: User Profiling | 4-5 weeks | 12 weeks |
| Phase 4: Categorization | 3-4 weeks | 16 weeks |
| Phase 5: File Management | 2-3 weeks | 19 weeks |
| Phase 6: Tools & Polish | 2-3 weeks | 22 weeks |

**Total: 22 weeks (5.5 months)**

With parallel development and proper planning, this could be reduced to **12-16 weeks (3-4 months)**.

---

## Next Steps

1. **Review and Approve**: Review this implementation plan
2. **Prioritize**: Decide which phases are most important
3. **Start Phase 1**: Begin with database schema changes
4. **Set Up Development**: Prepare development environment
5. **Create Branch**: Create feature branch for implementation
6. **Iterative Development**: Complete one phase at a time
7. **User Testing**: Beta test each phase before moving on

---

## Notes

- All features maintain backward compatibility
- Existing functionality continues to work
- Features are additive, not replacing
- User control is maintained throughout
- Privacy-first approach (all data local)
- Offline operation supported
- Cross-platform compatibility maintained

This plan provides a comprehensive roadmap for implementing all requested features in a structured, manageable way.
