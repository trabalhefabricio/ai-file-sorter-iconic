# GitHub Codespaces Implementation Guide

## Purpose
This guide provides pre-implementation instructions for AI agents working on this fork via GitHub Codespaces to implement the changes documented in ALL_CHANGES_PLAIN_TEXT.txt.

## Repository Context

**Repository:** trabalhefabricio/ai-file-sorter-iconic  
**Original Fork:** hyperfield/ai-file-sorter  
**Language:** C++ (Qt6)  
**Build System:** CMake + Make  
**Platform:** Cross-platform (Windows, macOS, Linux)

---

## Pre-Implementation Setup Instructions

### 1. UNDERSTAND THE CODEBASE ARCHITECTURE

**Before making ANY changes, familiarize yourself with:**

**Core Architecture Pattern:**
- MVC-like structure with Qt6
- **Managers** (13 classes): Business logic, state management (e.g., UserProfileManager, UndoManager, DatabaseManager)
- **Dialogs** (18 classes): UI components (e.g., WhitelistManagerDialog, CacheManagerDialog, FileTinderDialog)
- **Services** (4 classes): Core operations (e.g., CategorizationService, ConsistencyPassService)

**Key Directories:**
```
app/
├── include/           # Header files (.hpp)
├── lib/              # Implementation files (.cpp)
├── resources/        # UI resources, icons
└── CMakeLists.txt    # Build configuration
```

**Critical Files to Review:**
- app/include/DatabaseManager.hpp - Database schema and operations
- app/include/Settings.hpp - Application settings and configuration
- app/lib/CategorizationService.cpp - Core categorization logic
- app/include/Types.hpp - Data structures used throughout
- app/lib/MainApp.cpp - Main application window and UI coordination

### 2. BUILD SYSTEM UNDERSTANDING

**This is a Qt6 C++ application using CMake:**

**Dependencies:**
- Qt6 (Core, Widgets, Network, Sql)
- SQLite3
- libcurl (for HTTP requests to Gemini/OpenAI)
- jsoncpp (for JSON parsing)
- spdlog (for logging)
- Optional: exiv2, poppler, libarchive (for content analysis features)

**Build Commands:**
```bash
# Standard CMake build
cd app
mkdir -p build
cd build
cmake ..
make -j$(nproc)

# Or use the Makefile wrapper
cd app
make
```

**IMPORTANT:** Always test builds after changes before committing.

### 3. DATABASE SCHEMA AWARENESS

**The application uses SQLite with multiple tables.**

**Existing Tables (already implemented):**
- file_categorization - Cached categorization results
- category_taxonomy - Category hierarchy and frequency
- user_profiles - User profile data
- profile_characteristics - User traits and preferences
- folder_insights - Per-folder analysis data
- folder_learning_settings - Learning level per folder
- api_usage_tracking - API request tracking
- file_tinder_state - File Tinder session state
- undo_history - Undo operation tracking

**Critical:** When adding database features:
1. Add table creation in DatabaseManager::initialize()
2. Add indexes for performance (see optimization recommendations)
3. Use transactions for batch operations
4. Test with existing database files (backward compatibility)

### 4. SETTINGS AND CONFIGURATION

**Configuration Files:**
- config.ini - Main application settings
- whitelists.ini - Whitelist definitions (categories/subcategories)
- Database: app_data/ai_file_sorter.db

**Location:** Platform-specific user data directory
- Linux: ~/.local/share/ai-file-sorter/
- Windows: %APPDATA%/ai-file-sorter/
- macOS: ~/Library/Application Support/ai-file-sorter/

**When adding new settings:**
1. Add getter/setter methods in Settings class
2. Add persistence in save()/load() methods
3. Add UI controls in appropriate dialog
4. Test settings persistence across restarts

### 5. CODING STANDARDS AND PATTERNS

**Follow Existing Patterns:**

**Naming Conventions:**
- Classes: PascalCase (e.g., UserProfileManager)
- Methods: snake_case (e.g., analyze_and_update_from_folder)
- Member variables: snake_case with trailing underscore (e.g., db_manager_)
- Constants: kPascalCase (e.g., kMinHobbyConfidence)

**Qt Patterns:**
- Use Qt signals/slots for async communication
- Inherit from QObject for classes needing signals/slots
- Use Q_OBJECT macro when using signals/slots
- Prefer Qt containers (QString, QVector) for Qt-related code
- Use std containers (std::string, std::vector) for internal logic

**Memory Management:**
- Use smart pointers (std::shared_ptr, std::unique_ptr) for ownership
- Qt parent-child relationship for widgets (automatic cleanup)
- RAII pattern for resource management

**Error Handling:**
- Use exceptions (AppException) for serious errors
- Return std::optional for nullable results
- Log errors using spdlog logger
- Show user-friendly error dialogs via DialogUtils

### 6. IMPLEMENTATION ORDER (CRITICAL)

**Follow this sequence when implementing from ALL_CHANGES_PLAIN_TEXT.txt:**

**Phase 1: Section 1 - Changed/Enhanced Features**
Start here. These modify existing code:
1. Whitelist hierarchical mode + flatten_to_legacy()
2. Whitelist separator change (comma → semicolon)
3. Enhanced context building in CategorizationService
4. (Gemini integration - already done)
5. (Dry run preview - already done)
6. (Persistent undo - already done)

**Phase 2: Section 3 - Optimization Recommendations**
Apply these BEFORE adding new planned features:
1. HIGH PRIORITY:
   - Add database indexes (5 min, test queries before/after)
   - Enable WAL mode (1 line in DatabaseManager::initialize())
   - Batch API usage writes (2-3 hours, test with 100+ files)

2. MEDIUM PRIORITY:
   - Async profile analysis (4-6 hours, careful with threading)
   - Reduce Gemini file I/O (2-3 hours, test state persistence)

**Phase 3: Section 4 - Planned Features**
Only after Phases 1 & 2 are complete and tested.

### 7. TESTING REQUIREMENTS

**Before Committing ANY Code:**

**Functional Testing:**
1. Build successfully with no warnings
2. Run the application and verify UI loads
3. Test the specific feature you changed
4. Test related features (regression testing)
5. Test with existing database files (backward compatibility)

**Integration Testing:**
- Test with actual files (create test folder with 20-50 files)
- Test categorization end-to-end
- Test settings persistence
- Test undo functionality if you modified file operations

**Performance Testing (for optimizations):**
- Measure before/after with realistic data
- Test with 1000+ files for batch operations
- Monitor memory usage with large datasets

### 8. COMMON PITFALLS TO AVOID

**DO NOT:**
- ❌ Modify database schema without migration strategy
- ❌ Add synchronous long-running operations on UI thread
- ❌ Use raw pointers for Qt widgets with no parent
- ❌ Forget to update CMakeLists.txt when adding new files
- ❌ Commit without building and testing
- ❌ Break existing functionality
- ❌ Add features in wrong dependency order

**DO:**
- ✅ Read existing code for similar functionality first
- ✅ Reuse existing patterns and helper functions
- ✅ Add logging for debugging (use spdlog)
- ✅ Handle errors gracefully with user feedback
- ✅ Test on multiple platforms if possible
- ✅ Keep commits small and focused
- ✅ Document complex logic with comments

### 9. FILE MODIFICATION WORKFLOW

**When modifying existing files:**
1. Read the entire file first to understand context
2. Look for similar patterns elsewhere in codebase
3. Make minimal changes (surgical edits)
4. Preserve existing code style and formatting
5. Update related header files if needed
6. Add/update documentation comments

**When adding new files:**
1. Follow existing file structure and naming
2. Add to app/include/ for headers
3. Add to app/lib/ for implementations
4. Update app/CMakeLists.txt to include new files
5. Use include guards in headers: #ifndef FILENAME_HPP / #define FILENAME_HPP
6. Follow existing copyright header format

### 10. SPECIFIC IMPLEMENTATION NOTES

**For Whitelist Hierarchical Mode (Section 1.1):**
- Existing WhitelistStore class handles persistence
- INI format already supports key-value pairs
- flatten_to_legacy() needed for AI prompt generation
- Test with both hierarchical and shared modes
- Verify conversion between modes works correctly

**For Database Optimizations (Section 3):**
- Add indexes in DatabaseManager::initialize()
- Test query performance with EXPLAIN QUERY PLAN
- Enable WAL mode: db.exec("PRAGMA journal_mode = WAL;")
- Verify no conflicts with existing queries

**For User Profiling (Already Implemented, but for reference):**
- UserProfileManager handles all profile operations
- Database tables already created
- Integration point: CategorizationService calls after sorting
- UI: UserProfileDialog, FolderLearningDialog

### 11. DEBUGGING TOOLS

**Available in the Application:**
- Logging: Check ~/.local/share/ai-file-sorter/logs/ (Linux) or equivalent
- Database: Use sqlite3 CLI to inspect database state
- Error Reporter: Check app/lib/ErrorReporter.cpp for error tracking
- Testing Mode: Future feature for automated testing

**Qt Creator (recommended IDE):**
- Open app/CMakeLists.txt as project
- Built-in debugger with breakpoints
- Qt Designer for UI editing
- Integrated build system

### 12. RESOURCE CONSTRAINTS

**Keep in mind:**
- Target users have varied hardware (laptops to workstations)
- Some users work with 10,000+ files
- Database operations must scale
- UI must remain responsive
- Memory usage should be reasonable (<500MB typical)

### 13. FINAL PRE-IMPLEMENTATION CHECKLIST

Before starting implementation:

- [ ] Read ALL_CHANGES_PLAIN_TEXT.txt completely
- [ ] Understand the specific feature you're implementing
- [ ] Review relevant existing code files
- [ ] Identify dependencies and integration points
- [ ] Understand the build system and how to test
- [ ] Set up your development environment
- [ ] Create a test plan for your changes
- [ ] Understand backward compatibility requirements
- [ ] Know which optimization recommendations to apply first
- [ ] Have a rollback plan if something breaks

---

## Quick Command Reference

```bash
# Build the application
cd app && make

# Clean build
cd app && make clean && make

# Run from build directory
cd app/build && ./ai-file-sorter

# View logs (Linux)
tail -f ~/.local/share/ai-file-sorter/logs/latest.log

# Inspect database
sqlite3 ~/.local/share/ai-file-sorter/ai_file_sorter.db
> .tables
> .schema table_name
> SELECT * FROM table_name LIMIT 10;

# Check for TODO/FIXME comments
grep -r "TODO\|FIXME" app/lib/ app/include/
```

---

## Key Success Factors

1. **Read before writing**: Understand existing code patterns
2. **Test incrementally**: Build and test after each change
3. **Follow the order**: Implement in dependency order
4. **Optimize first**: Apply Section 3 optimizations before adding features
5. **Preserve quality**: Don't break existing functionality
6. **Document decisions**: Add comments for complex logic
7. **Ask when stuck**: Better to clarify than implement incorrectly

---

## Emergency Rollback

If you break something:
```bash
# See recent changes
git log --oneline -10

# Revert last commit
git revert HEAD

# Discard all uncommitted changes
git reset --hard HEAD

# Check what files changed
git diff HEAD~1
```

---

This guide ensures you implement changes correctly, maintain code quality, and avoid common mistakes. Refer to it before starting ANY implementation work.

