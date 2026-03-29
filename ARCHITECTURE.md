# AI File Sorter - Application Architecture

## Table of Contents
- [Overview](#overview)
- [Architecture Layers](#architecture-layers)
- [Core Components](#core-components)
- [Data Flow](#data-flow)
- [LLM Integration](#llm-integration)
- [Database Architecture](#database-architecture)
- [User Profile & Learning System](#user-profile--learning-system)
- [Error Handling System](#error-handling-system)
- [Internationalization](#internationalization)
- [Component Interaction Patterns](#component-interaction-patterns)

---

## Overview

AI File Sorter is a cross-platform Qt6 desktop application that automates file organization using AI-powered categorization. The application follows a layered architecture with clear separation of concerns between UI, business logic, and data access layers.

### Key Design Principles
- **Separation of Concerns**: Clear boundaries between UI, business logic, and data layers
- **Extensibility**: Plugin-like architecture for LLM providers
- **Offline-First**: Local LLM support with optional remote API usage
- **User Control**: Profile learning and categorization preferences are configurable
- **Type Safety**: Modern C++20 with strong typing throughout

### High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         PRESENTATION LAYER                          │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐   │
│  │   MainApp    │  │   Dialogs    │  │   Progress/Results     │   │
│  │   (Qt6 UI)   │  │   (Review)   │  │     (Feedback)         │   │
│  └──────┬───────┘  └──────┬───────┘  └───────────┬────────────┘   │
└─────────┼──────────────────┼──────────────────────┼────────────────┘
          │                  │                      │
          ▼                  ▼                      ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       BUSINESS LOGIC LAYER                          │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │ Categorization   │  │  Consistency     │  │  Results        │  │
│  │    Service       │──│     Pass         │──│  Coordinator    │  │
│  └────────┬─────────┘  └──────────────────┘  └─────────────────┘  │
│           │            ┌──────────────────┐  ┌─────────────────┐  │
│           │            │  File Scanner    │  │  User Profile   │  │
│           └────────────│  (Discovery)     │  │   Manager       │  │
│                        └──────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
          │                                           │
          ▼                                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          LLM ABSTRACTION                            │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────────┐  │
│  │   ILLMClient │──│ LocalLLM     │  │  ChatGPT / Gemini       │  │
│  │  (Interface) │  │ (llama.cpp)  │  │  (Remote APIs)          │  │
│  └──────────────┘  └──────────────┘  └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
          │                                           │
          ▼                                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        DATA ACCESS LAYER                            │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │  Database        │  │  Whitelist       │  │   Settings      │  │
│  │  Manager         │  │  Store           │  │   (IniConfig)   │  │
│  │  (SQLite Cache)  │  │  (Categories)    │  │   (Persist)     │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
          │
          ▼
    ┌──────────┐
    │  SQLite  │
    │ Database │
    └──────────┘
```

---

## Architecture Layers

### 1. Presentation Layer (UI)
**Location**: `app/lib/*Dialog.cpp`, `app/lib/MainApp*.cpp`

The presentation layer is built on Qt6 widgets and handles all user interactions.

#### Components:
- **MainApp** (`MainApp.hpp/.cpp`)
  - Primary application window
  - Coordinates all UI components
  - Manages application lifecycle
  - Handles menu actions and toolbar interactions
  
- **MainAppUiBuilder** (`MainAppUiBuilder.cpp`)
  - Separates UI construction from business logic
  - Builds all widgets and layouts
  - Configures visual appearance

- **Dialog Classes**:
  - `CategorizationDialog`: Review and edit file categorizations
  - `CategorizationProgressDialog`: Shows analysis progress
  - `LLMSelectionDialog`: Choose and configure LLM provider
  - `UserCategorizationDialog`: Manual category assignment
  - `WhitelistManagerDialog`: Manage category whitelists
  - `UserProfileDialog`: View user profile and learning data
  - `DryRunPreviewDialog`: Preview file moves before execution
  - `FolderLearningDialog`: Configure per-folder learning settings

#### UI Flow:
1. User selects directory
2. Configures options (subcategories, whitelists, scan options)
3. Clicks "Analyze"
4. Progress dialog shows real-time updates
5. Review dialog displays results
6. User confirms or edits categorizations
7. Files are moved or dry-run preview is shown

---

### 2. Business Logic Layer
**Location**: `app/lib/*Service.cpp`, `app/lib/FileScanner.cpp`

The business logic layer implements core application functionality independent of UI.

#### Core Services:

##### CategorizationService (`CategorizationService.hpp/.cpp`)
**Purpose**: Orchestrates the file categorization process

**Responsibilities**:
- Coordinates between LLM clients and database
- Manages categorization cache
- Implements consistency hints for uniform categorization
- Handles batch processing with progress callbacks
- Manages timeout and retry logic

**Key Methods**:
```cpp
// Load previously categorized files from cache
std::vector<CategorizedFile> load_cached_entries(const std::string& directory_path);

// Categorize new files using LLM
std::vector<CategorizedFile> categorize_entries(
    const std::vector<FileEntry>& files,
    bool is_local_llm,
    std::atomic<bool>& stop_flag,
    const ProgressCallback& progress_callback,
    const QueueCallback& queue_callback,
    const RecategorizationCallback& recategorization_callback,
    std::function<std::unique_ptr<ILLMClient>()> llm_factory);
```

**Categorization Strategy**:
1. Check cache for existing categorization
2. If not cached, send to LLM with context
3. Apply consistency hints (if enabled)
4. Store result in database
5. Return categorized file

##### ConsistencyPassService (`ConsistencyPassService.hpp/.cpp`)
**Purpose**: Ensures consistent categorization across similar files

**How It Works**:
- Analyzes extension and file name patterns
- Builds consistency hints from recent assignments
- Provides context to LLM: "Similar files were categorized as X"
- Helps maintain uniform categories within a folder

##### FileScanner (`FileScanner.hpp/.cpp`)
**Purpose**: Discovers and classifies files/directories

**Responsibilities**:
- Traverses directory structures
- Identifies file types (regular files, bundles, directories)
- Filters hidden and junk files
- Respects user scan options (files, directories, both)

**File Classification**:
```cpp
enum class FileType {
    RegularFile,    // Standard file
    Directory,      // Folder
    Bundle          // macOS .app bundles, etc.
};
```

##### ResultsCoordinator (`ResultsCoordinator.hpp/.cpp`)
**Purpose**: Manages categorization results and file operations

**Responsibilities**:
- Validates categorization results
- Prepares file move operations
- Handles dry-run vs actual move logic
- Coordinates with UndoManager

##### UndoManager (`UndoManager.hpp/.cpp`)
**Purpose**: Enables reverting file sorting operations

**Features**:
- Persists undo plans to disk
- Validates undo safety (checks for conflicts)
- Best-effort restoration of original state

---

### 3. Data Access Layer
**Location**: `app/lib/DatabaseManager.cpp`, `app/lib/WhitelistStore.cpp`

#### DatabaseManager (`DatabaseManager.hpp/.cpp`)
**Purpose**: All database operations using SQLite

**Database Schema**:
- **taxonomy**: Category/subcategory definitions with frequency tracking
- **categorization**: File categorization cache
- **user_profiles**: User profile data
- **user_characteristics**: Learned user traits (hobbies, work patterns)
- **folder_insights**: Per-folder analysis data
- **folder_learning_settings**: Per-folder learning preferences
- **organizational_templates**: Reusable organization patterns

**Key Methods**:
```cpp
// Category resolution with taxonomy management
ResolvedCategory resolve_category(const std::string& category, 
                                   const std::string& subcategory);

// Categorization cache operations
bool insert_or_update_file_with_categorization(...);
std::vector<CategorizedFile> get_categorized_files(const std::string& directory);

// User profile persistence
bool save_user_profile(const UserProfile& profile);
UserProfile load_user_profile(const std::string& user_id);

// Folder learning preferences
std::string get_folder_inclusion_level(const std::string& folder_path);
void set_folder_inclusion_level(const std::string& folder_path, const std::string& level);
```

**Caching Strategy**:
- Cache categorizations per file name + type
- Cache hits avoid expensive LLM calls
- Prune empty/invalid cache entries
- Frequency tracking for popular categories

#### WhitelistStore (`WhitelistStore.hpp/.cpp`)
**Purpose**: Manages category whitelists

**Features**:
- Store named whitelists (e.g., "Work", "Photos", "Downloads")
- Load/save whitelists from database
- Provide whitelist context to LLM prompts
- Support multiple whitelists per user

---

## Core Components

### Settings Management

#### Settings (`Settings.hpp/.cpp`)
**Purpose**: Application configuration persistence

**Stored Settings**:
- LLM choice (local, ChatGPT, Gemini)
- API keys (encrypted)
- Last selected directory
- UI preferences (language, theme)
- Categorization options (subcategories, consistency mode)
- File scan options
- Profile learning preferences

#### IniConfig (`IniConfig.hpp/.cpp`)
**Purpose**: INI file parsing and writing

**File Location**:
- Linux: `~/.config/aifilesorter/config.ini`
- macOS: `~/Library/Application Support/aifilesorter/config.ini`
- Windows: `%APPDATA%\aifilesorter\config.ini`

### Logging System

#### Logger (`Logger.hpp/.cpp`)
**Purpose**: Centralized logging using spdlog

**Log Categories**:
- `core_logger`: Application logic and errors
- `ui_logger`: UI events and user actions
- `llm_logger`: LLM requests and responses (optional)

**Log Levels**:
```cpp
trace() // Detailed debugging
debug() // Development information
info()  // General information
warn()  // Warnings
error() // Errors
critical() // Critical failures
```

### Translation System

#### UiTranslator (`UiTranslator.hpp/.cpp`)
**Purpose**: Runtime UI language switching

**Supported Languages**:
- English (default)
- French
- German
- Italian
- Spanish
- Turkish

#### TranslationManager (`TranslationManager.hpp/.cpp`)
**Purpose**: Manages Qt translation files and gettext integration

---

## Data Flow

### File Categorization Flow

```
1. USER ACTION
   ↓
   MainApp::on_analyze_clicked()
   ↓
2. FILE SCANNING
   ↓
   FileScanner::get_directory_entries()
   ├─ Traverse directory
   ├─ Filter hidden/junk files
   └─ Return FileEntry vector
   ↓
3. CACHE CHECK
   ↓
   CategorizationService::load_cached_entries()
   ├─ Query DatabaseManager
   └─ Return cached categorizations
   ↓
4. LLM CATEGORIZATION (for uncached files)
   ↓
   CategorizationService::categorize_entries()
   ├─ Create LLM client (Local/ChatGPT/Gemini)
   ├─ Build prompt with context
   │  ├─ File name/path
   │  ├─ Whitelist constraints
   │  ├─ Consistency hints
   │  ├─ User profile context
   │  └─ Category language preference
   ├─ Send to LLM
   ├─ Parse response
   ├─ Validate category
   ├─ Store in database cache
   └─ Return CategorizedFile
   ↓
5. CONSISTENCY PASS (if enabled)
   ↓
   ConsistencyPassService::run()
   ├─ Analyze patterns
   └─ Refine categorizations
   ↓
6. RESULTS DISPLAY
   ↓
   CategorizationDialog::show()
   ├─ Display in table
   ├─ Allow editing
   └─ User confirms
   ↓
7. FILE OPERATIONS
   ↓
   ResultsCoordinator::execute_moves()
   ├─ Create destination folders
   ├─ Move files
   ├─ Save undo plan
   └─ Report success
```

### User Profile Learning Flow

```
1. ANALYSIS COMPLETION
   ↓
   UserProfileManager::analyze_folder()
   ├─ Extract categories
   ├─ Detect patterns
   ├─ Infer characteristics
   │  ├─ Hobbies (music, photography, etc.)
   │  ├─ Work patterns (programming, design, etc.)
   │  └─ Organization style (minimalist, detailed, etc.)
   ├─ Update confidence scores
   └─ Store in database
   ↓
2. PROFILE USAGE
   ↓
   CategorizationService::categorize_entries()
   ├─ Load user profile
   ├─ Build profile context
   ├─ Include in LLM prompt
   └─ Get personalized suggestions
   ↓
3. PROFILE VIEWING
   ↓
   UserProfileDialog::show()
   ├─ Display characteristics
   ├─ Show folder insights
   └─ Present confidence levels
```

---

## LLM Integration

### Abstract Interface

#### ILLMClient (`ILLMClient.hpp`)
**Purpose**: Common interface for all LLM providers

```cpp
class ILLMClient {
public:
    virtual ~ILLMClient() = default;
    
    virtual std::string categorize_file(
        const std::string& file_name,
        const std::string& file_path,
        FileType file_type,
        const std::string& consistency_context) = 0;
    
    virtual std::string complete_prompt(
        const std::string& prompt,
        int max_tokens) = 0;
    
    virtual void set_prompt_logging_enabled(bool enabled) = 0;
};
```

### Implementations

#### LocalLLMClient (`LocalLLMClient.hpp/.cpp`)
**Purpose**: Run GGUF models locally using llama.cpp

**Features**:
- GPU acceleration (CUDA, Vulkan, Metal)
- CPU fallback with OpenBLAS
- Model caching and management
- Thread-safe inference

**Model Loading**:
1. Check model cache (`~/.local/share/aifilesorter/llms`)
2. Load model with llama.cpp
3. Configure context size and parameters
4. Keep model in memory for session

**GPU Backend Priority**:
```
Vulkan → CUDA → Metal → CPU/OpenBLAS
```

#### LLMClient (`LLMClient.hpp/.cpp`)
**Purpose**: OpenAI ChatGPT API integration

**Features**:
- API key management
- Model selection (gpt-4o-mini, gpt-4, etc.)
- Rate limiting and retry logic
- Response parsing

**Request Format**:
```json
{
  "model": "gpt-4o-mini",
  "messages": [
    {"role": "system", "content": "You are a file categorization assistant..."},
    {"role": "user", "content": "Categorize: vacation_photo.jpg"}
  ],
  "temperature": 0.3,
  "max_tokens": 50
}
```

#### GeminiClient (`GeminiClient.hpp/.cpp`)
**Purpose**: Google Gemini API integration

**Features**:
- Free tier optimization (15 RPM rate limiting)
- Adaptive timeout handling (20-240s)
- Exponential backoff retry logic
- Persistent state tracking

**Rate Limiting Strategy**:
- Track request timestamps
- Enforce 15 requests per minute for free tier
- Adaptive delays between requests
- Automatic retry on quota errors

---

## Database Architecture

### Schema Overview

#### Categorization Cache
```sql
CREATE TABLE categorization (
    id INTEGER PRIMARY KEY,
    file_name TEXT NOT NULL,
    file_type TEXT NOT NULL,
    dir_path TEXT NOT NULL,
    taxonomy_id INTEGER,
    used_consistency_hints BOOLEAN,
    user_provided BOOLEAN,
    created_at TIMESTAMP,
    FOREIGN KEY (taxonomy_id) REFERENCES taxonomy(id)
);
```

#### Taxonomy System
```sql
CREATE TABLE taxonomy (
    id INTEGER PRIMARY KEY,
    category TEXT NOT NULL,
    subcategory TEXT,
    normalized_category TEXT,
    frequency INTEGER DEFAULT 0,
    UNIQUE(normalized_category)
);
```

**Normalization**: Categories are normalized (lowercased, trimmed) to avoid duplicates like "Documents" vs "documents".

#### User Profiles
```sql
CREATE TABLE user_profiles (
    user_id TEXT PRIMARY KEY,
    created_at TIMESTAMP,
    last_updated TIMESTAMP
);

CREATE TABLE user_characteristics (
    id INTEGER PRIMARY KEY,
    user_id TEXT,
    category TEXT,  -- 'hobby', 'work_pattern', 'organization_style'
    name TEXT,
    confidence REAL,
    evidence TEXT,
    FOREIGN KEY (user_id) REFERENCES user_profiles(user_id)
);

CREATE TABLE folder_insights (
    id INTEGER PRIMARY KEY,
    user_id TEXT,
    folder_path TEXT,
    dominant_categories TEXT,  -- JSON array
    file_count INTEGER,
    analyzed_at TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user_profiles(user_id)
);
```

#### Folder Learning Settings
```sql
CREATE TABLE folder_learning_settings (
    folder_path TEXT PRIMARY KEY,
    inclusion_level TEXT  -- 'full', 'partial', 'none'
);
```

### Database Operations

**Transaction Management**:
- Batch inserts use transactions
- Automatic rollback on errors
- Connection pooling for concurrent access

**Query Optimization**:
- Indexes on frequently queried columns
- Prepared statements for repeated queries
- Lazy loading of large result sets

---

## User Profile & Learning System

### UserProfileManager (`UserProfileManager.hpp/.cpp`)

**Purpose**: Learns from user's file organization patterns over time

### Learning Process

1. **Folder Analysis**
   ```cpp
   void analyze_folder(const std::string& folder_path,
                       const std::vector<CategorizedFile>& files);
   ```
   - Extracts category frequencies
   - Identifies patterns in file types
   - Detects organizational style

2. **Characteristic Inference**
   - **Hobbies**: Music files → music lover, Photos → photography enthusiast
   - **Work Patterns**: Code files → programmer, Design files → designer
   - **Organization Style**: File counts and categories → minimalist/power user

3. **Confidence Scoring**
   - Initial: Low confidence (0.3)
   - With more data: Confidence increases (up to 1.0)
   - Multiple evidences strengthen characteristics

4. **Profile Context for LLM**
   ```
   User Profile:
   - Hobbies: photography (confidence: 0.8), music (confidence: 0.6)
   - Work: software development (confidence: 0.9)
   - Style: detailed organization (confidence: 0.7)
   
   Use this context to better understand the user's needs.
   ```

### Learning Levels

**Per-Folder Configuration**:
- **Full Learning**: Use profile for categorization AND update profile
- **Partial Learning**: Don't use profile but STILL update it
- **No Learning**: Don't use profile AND don't update it

**Use Cases**:
- Full: Trusted personal folders
- Partial: Gathering data without affecting results
- None: Sensitive or temporary folders

---

## Error Handling System

### Comprehensive Error Codes

**See**: `ERROR_SYSTEM_SUMMARY.md`, `ERROR_CODES.md`

### AppException (`AppException.hpp`)

**Purpose**: Type-safe exception handling with error codes

```cpp
try {
    categorize_files();
}
catch (const ErrorCodes::AppException& ex) {
    // Error code: 1401
    int code = ex.get_error_code_int();
    
    // User message: "Failed to load the LLM model"
    std::string message = ex.get_user_message();
    
    // Resolution steps
    std::string resolution = ex.get_resolution();
    
    // Technical details
    std::string details = ex.get_context();
    
    // Show dialog with copy button
    DialogUtils::show_error_dialog(this, ex);
}
```

### Error Categories

| Range | Category | Examples |
|-------|----------|----------|
| 1000-1099 | Network | Connection, timeout, DNS, SSL |
| 1100-1199 | API | Authentication, rate limits, invalid response |
| 1200-1299 | File System | Not found, permissions, disk full |
| 1300-1399 | Database | Connection, query, corruption |
| 1400-1499 | LLM | Model loading, inference, OOM |
| 1500-1599 | Configuration | Invalid, missing, save failed |
| 1600-1699 | Validation | Invalid input, format errors |
| 1700-1799 | System | OOM, unsupported platform |
| 1800-1899 | Categorization | No files, failed, timeout |
| 1900-1999 | Download | CURL errors, network, write |

---

## Internationalization

### Two-Level System

1. **UI Translations** (Qt .ts files)
   - Location: `app/resources/i18n/`
   - Format: XML-based Qt Linguist files
   - Covers: Buttons, labels, dialogs

2. **Error Messages** (gettext)
   - Location: `app/locale/`
   - Format: .po/.mo files
   - Covers: Error messages, help text

### Translation Workflow

```cpp
// UI Translation (Qt)
QLabel* label = new QLabel(tr("Select Directory"));

// Error Message Translation (gettext)
std::string msg = _("Failed to load model");

// Category Language
settings.set_category_language(CategoryLanguage::French);
// LLM will now suggest categories in French
```

### Category Languages

**Supported**:
- English, Dutch, French, German, Italian, Polish, Portuguese, Spanish, Turkish

**Implementation**:
- System prompt includes: "Respond with categories in French"
- LLM generates localized category names
- UI remains in selected UI language

---

## Component Interaction Patterns

### Observer Pattern (Signals/Slots)

Qt's signal/slot mechanism connects components:

```cpp
// MainApp connects to dialog signals
connect(categorization_dialog, &CategorizationDialog::files_confirmed,
        this, &MainApp::on_files_confirmed);

// Progress updates
connect(service, &Service::progress_update,
        progress_dialog, &ProgressDialog::update_progress);
```

### Factory Pattern (LLM Clients)

```cpp
std::unique_ptr<ILLMClient> MainApp::make_llm_client() {
    switch (settings.get_llm_choice()) {
        case LLMChoice::Local:
            return std::make_unique<LocalLLMClient>(...);
        case LLMChoice::ChatGPT:
            return std::make_unique<LLMClient>(...);
        case LLMChoice::Gemini:
            return std::make_unique<GeminiClient>(...);
    }
}
```

### Strategy Pattern (Categorization Modes)

```cpp
if (use_consistency_mode) {
    // Build consistency hints from session history
    hints = collect_consistency_hints(...);
} else {
    // Refined mode: no hints
    hints = "";
}
```

### Callback Pattern (Progress Reporting)

```cpp
auto progress_callback = [this](const std::string& msg) {
    emit progress_update(msg);
};

categorization_service.categorize_entries(
    files, 
    is_local,
    stop_flag,
    progress_callback,  // Called during processing
    queue_callback,
    recategorization_callback,
    llm_factory);
```

---

## Threading Model

### UI Thread
- All Qt widgets and events
- User interactions
- Dialog displays

### Worker Thread
- File scanning
- LLM categorization
- Database operations (bulk)

### Thread Safety
- `std::atomic<bool> stop_flag` for cancellation
- Callbacks execute on UI thread via Qt signals
- Database: one connection per thread

---

## Build System

### CMake Structure
- Main: `app/CMakeLists.txt`
- Tests: `tests/unit/CMakeLists.txt`
- Dependencies: vcpkg for Windows, pkg-config for Unix

### Launcher Scripts
- **Windows**: `StartAiFileSorter.exe` detects GPU backend
- **Linux**: `run_aifilesorter.sh` sets LD_LIBRARY_PATH
- **macOS**: `.app` bundle with Metal support

---

## Summary

AI File Sorter is a well-architected application with:

1. **Clean Layering**: UI → Business Logic → Data Access
2. **Extensible Design**: Plugin-like LLM providers
3. **Offline-First**: Local models with cloud options
4. **Smart Caching**: Reduces LLM costs and latency
5. **User Control**: Configurable learning and categorization
6. **Robust Errors**: Comprehensive error handling with user guidance
7. **International**: Multi-language UI and category support
8. **Cross-Platform**: Qt6 on Windows, macOS, Linux

Each component has a single, well-defined responsibility, making the codebase maintainable and testable.
