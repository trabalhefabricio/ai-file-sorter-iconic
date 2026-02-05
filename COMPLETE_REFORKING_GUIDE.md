# Complete Reforking Guide - All Custom Features

This comprehensive guide contains all 26 implementation prompts needed to re-fork `hyperfield/ai-file-sorter` while preserving every custom feature from `trabalhefabricio/ai-file-sorter-iconic`.

## Overview

**Purpose:** Systematically implement all custom features in a fresh fork of the original repository.

**Timeline:** ~8 weeks with AI assistance

**Approach:** Sequential implementation using "I want" style prompts that describe desired features without implementation code.

**Total Prompts:** 26 (Prompt 0-25)

---

## Usage Instructions

1. Fork `hyperfield/ai-file-sorter` on GitHub
2. Clone your new fork locally
3. Copy each prompt below sequentially
4. Feed prompt to your AI coding agent (GitHub Copilot, Cursor, Claude, etc.)
5. Review and test the generated code
6. Commit the changes
7. Move to the next prompt

**Important:** Follow prompts in order - each builds on previous implementations.

---

## Prompt 0: Enhanced Categorization Prompt ⚠️ MOST CRITICAL

**Priority:** CRITICAL - Implement this first before anything else

**What I Want Built:**

I want to enhance the core AI categorization prompt to make it significantly more intelligent than the original fork.

**How It Should Behave:**

The categorization system should:
- Analyze file extensions to understand what type of content the file represents (e.g., `.cpp` files are C++ source code, `.json` are configuration/data files)
- Consider the full file path when available, not just the filename
- Include semantic analysis instructions that ask the AI to think about the PURPOSE of the file, not just its name
- Implement a confidence threshold of 70% - if the AI is less than 70% confident, it should return "UNCERTAIN" rather than guessing
- Use a structured three-point analysis framework: (1) what this file extension typically represents, (2) the meaning of the filename, (3) common uses for this file type

**Integration Requirements:**

This change affects the core categorization logic that runs every time a file is processed. It should work with both OpenAI and Gemini API clients (which will be added later).

**What Success Looks Like:**

- When categorizing a `config.json` file, the AI should recognize it's a configuration file and consider JSON semantics
- Files with ambiguous names should return "UNCERTAIN" instead of making poor guesses
- The categorization accuracy should noticeably improve compared to the original fork's simple "categorize this file: [filename]" approach
- Path context should help disambiguate files (e.g., `test.cpp` in a `/tests/` folder vs in `/src/`)

---

## Prompt 1: Project Setup and Database Foundation

**What I Want Built:**

I want to set up the project with proper database infrastructure and helper methods that will support all the custom features.

**How It Should Behave:**

The system should:
- Have a DatabaseManager class with connection pooling and transaction support
- Include helper methods for common database operations (insert, update, delete, query)
- Support prepared statements for SQL injection prevention
- Have proper error handling for database operations
- Support migrations for future schema changes

**Integration Requirements:**

This is foundational infrastructure that all features will use. It should be initialized when the application starts and cleaned up on shutdown.

**What Success Looks Like:**

- Database connections are stable and don't leak
- All SQL operations use prepared statements
- Error messages for database issues are clear and actionable
- The application can recover gracefully from database errors

---

## Prompt 2: Complete Database Schema (16 Tables)

**What I Want Built:**

I want a comprehensive database schema that supports both current features and provides foundations for future features.

**How It Should Behave:**

The schema should include tables for:
1. **Active Features (8 tables):**
   - `undo_history` - Persistent undo operations across sessions
   - `api_usage` - Track API token consumption and costs
   - `user_profile` - User behavior patterns and preferences
   - `folder_insights` - Learned patterns per folder
   - `categorization_cache` - Cache AI responses to save API calls
   - `file_tinder_history` - Track File Tinder decisions
   - `error_reports` - Store generated error reports
   - `whitelist_entries` - Files/folders excluded from processing

2. **Future-Ready Tables (8 tables):**
   - `confidence_scores` - For future confidence tracking
   - `content_analysis_cache` - For future content-based analysis
   - `categorization_sessions` - For future session management
   - `user_corrections` - For future correction learning
   - `translation_strings` - For future multi-language support
   - `custom_llm_configs` - For future custom LLM endpoints
   - `category_templates` - For future category suggestions
   - `scheduled_tasks` - For future automation

**Integration Requirements:**

All tables should have appropriate indices for performance, foreign key constraints for data integrity, and timestamps for tracking.

**What Success Looks Like:**

- Schema creates successfully on first run
- All relationships between tables are properly defined
- Queries on large datasets remain performant due to proper indexing
- Future features can be enabled by simply using the existing tables

---

## Prompt 3: Enhanced Persistent Undo System

**What I Want Built:**

I want an undo system that remembers all file operations and can undo them even after the application restarts.

**How It Should Behave:**

The system should:
- Save every file move operation to the database before executing it
- Store the original path, new path, and timestamp
- Allow users to undo operations from previous sessions
- Support undoing multiple operations in reverse chronological order
- Clean up old undo history after a configurable time period (e.g., 30 days)

**Integration Requirements:**

This should integrate into the existing file moving logic. Every time a file is moved, the undo information should be persisted first.

**What Success Looks Like:**

- Users can undo yesterday's file moves
- The undo list shows when each operation was performed
- Undo operations fail gracefully if files have been manually moved/deleted
- Users can see what would be undone before confirming

---

## Prompt 4: Dry Run Preview Mode

**What I Want Built:**

I want users to be able to preview what would happen to their files before actually moving them.

**How It Should Behave:**

The system should:
- Have a "Dry Run" checkbox in the categorization dialog
- When enabled, show all planned file moves in a preview dialog
- Display: original path → new path for each file
- Allow users to review the list before executing
- Provide "Execute All" and "Cancel" buttons
- Not actually move any files until the user confirms

**Integration Requirements:**

This should hook into the categorization process before any file operations occur. The dry run should use the same logic as real categorization to ensure accuracy.

**What Success Looks Like:**

- Users can see exactly where their files will go
- The preview list shows the full paths clearly
- Users can review and decide whether to proceed
- If they cancel, no files are moved

---

## Prompt 5: Cache Manager Dialog

**What I Want Built:**

I want a dialog that lets users view cache statistics and clear cached AI responses when needed.

**How It Should Behave:**

The dialog should:
- Display the number of cached categorization results
- Show the size of the cache in MB
- Display how much the cache has saved (estimated API calls avoided)
- Provide a "Clear Cache" button to remove all cached data
- Include an "Optimize Database" button to compact the database
- Show before/after statistics when actions are performed

**Integration Requirements:**

This should be accessible from the Tools menu. It should interact with the `categorization_cache` table and use database optimization commands.

**What Success Looks Like:**

- Users can see how much the cache is helping them
- Clearing the cache frees up disk space
- Database optimization improves query performance
- All statistics are accurate and update in real-time

---

## Prompt 6: API Usage Tracking System

**What I Want Built:**

I want the system to track every API call made to OpenAI or Gemini, including token usage and estimated costs.

**How It Should Behave:**

The system should:
- Log every API call to the `api_usage` table
- Record: timestamp, LLM provider, model used, tokens consumed, estimated cost
- Calculate costs based on current pricing for each provider
- Track both successful and failed API calls
- Aggregate statistics: total calls today, this week, this month, all time

**Integration Requirements:**

This should wrap all API calls to OpenAI and Gemini clients. Every time an API request is made, usage should be logged.

**What Success Looks Like:**

- Every API call is tracked accurately
- Token counts match what the API returns
- Cost estimates help users understand their spending
- Historical data shows usage patterns over time

---

## Prompt 7: Usage Statistics Dialog

**What I Want Built:**

I want a dialog that displays API usage statistics in a user-friendly way.

**How It Should Behave:**

The dialog should:
- Show total API calls and tokens used
- Display estimated costs (with disclaimer that these are estimates)
- Break down usage by: today, this week, this month, all time
- Show usage per LLM provider (OpenAI vs Gemini)
- Display a usage trend graph or simple statistics
- Include an "Export" button to save statistics to CSV

**Integration Requirements:**

This should be accessible from the Tools menu and query the `api_usage` table for all statistics.

**What Success Looks Like:**

- Users can see their API usage at a glance
- Cost estimates help them budget for API usage
- The breakdown helps identify patterns (e.g., high usage on certain days)
- Exported data can be analyzed in spreadsheets

---

## Prompt 8: Google Gemini API Client (NEW FEATURE)

**What I Want Built:**

I want to add Google Gemini as an alternative LLM provider. The original fork only supports OpenAI.

**How It Should Behave:**

The Gemini client should:
- Implement the same interface as the OpenAI client for consistency
- Support Gemini's free tier and paid tiers
- Include rate limiting to respect Gemini's quotas
- Implement retry logic with exponential backoff for failed requests
- Support the latest Gemini models (Gemini Pro, Gemini Pro Vision)
- Parse Gemini's response format and convert to the standard format

**Integration Requirements:**

This should be a new client that implements the ILLMClient interface. Users should be able to select Gemini as their LLM provider in settings.

**What Success Looks Like:**

- Users can switch between OpenAI and Gemini seamlessly
- Gemini's free tier works correctly
- Rate limiting prevents quota errors
- Categorization quality is comparable to OpenAI
- All features (dry run, cache, undo) work with Gemini

---

## Prompt 9: LLM Selection Dialog (NEW FEATURE)

**What I Want Built:**

I want a user-friendly dialog for selecting and configuring LLM providers.

**How It Should Behave:**

The dialog should:
- Show available LLM providers: OpenAI, Gemini, Local LLM, Custom
- For each provider, show: pricing info, features, recommended use cases
- Allow users to enter their API key with a "Test Connection" button
- Display the currently selected provider
- Save the selection and API key securely
- Show helpful error messages if API keys are invalid

**Integration Requirements:**

This should be accessible from the Settings menu and update the application's LLM configuration.

**What Success Looks Like:**

- Users can easily switch between providers
- The "Test Connection" button verifies API keys before saving
- Helpful messages guide users through setup
- Settings are saved and persist across restarts

---

## Prompt 10: Local LLM Client Support (NEW FEATURE)

**What I Want Built:**

I want support for locally hosted LLM models (like LM Studio, Ollama) for users who want offline AI or privacy.

**How It Should Behave:**

The Local LLM client should:
- Support OpenAI-compatible endpoints (LM Studio, Ollama, etc.)
- Allow users to specify the endpoint URL
- Not require API keys for local models
- Work offline if the local server is running
- Support common local models (Llama, Mistral, etc.)
- Handle connection errors gracefully (e.g., if local server is down)

**Integration Requirements:**

This should implement the ILLMClient interface and be selectable as a provider in the LLM Selection Dialog.

**What Success Looks Like:**

- Users can categorize files without internet connection
- Local model responses are processed correctly
- The system gracefully handles when the local server is offline
- All features work with local LLMs (except API usage tracking, which doesn't apply)

---

## Prompt 11: Custom LLM Configuration Dialog (NEW FEATURE)

**What I Want Built:**

I want users to be able to configure any custom LLM endpoint that they want to use.

**How It Should Behave:**

The dialog should:
- Allow users to enter a custom API endpoint URL
- Support custom headers and authentication methods
- Provide fields for: model name, temperature, max tokens
- Include a "Test Custom Endpoint" button
- Save multiple custom configurations
- Show which custom config is currently active

**Integration Requirements:**

This should save configurations to the `custom_llm_configs` table and make them available in the LLM Selection Dialog.

**What Success Looks Like:**

- Users can configure any LLM service they have access to
- Test functionality verifies the endpoint works before saving
- Multiple configurations can be saved and switched between
- Authentication (Bearer tokens, API keys) works correctly

---

## Prompt 12: User Profiling Core Engine

**What I Want Built:**

I want a system that learns from user behavior to provide better categorization suggestions over time.

**How It Should Behave:**

The profiling engine should:
- Observe which categories users choose for different file types
- Track patterns per folder (e.g., "files in ~/Downloads/ usually go to category X")
- Identify frequently used categories for specific file extensions
- Build a confidence score for each learned pattern
- Decay old patterns over time so it adapts to changing behavior
- Store all insights in the `user_profile` and `folder_insights` tables

**Integration Requirements:**

This should run in the background and observe all categorization decisions (both AI and manual).

**What Success Looks Like:**

- After categorizing 20-30 files, the system starts making accurate suggestions
- Patterns are specific enough to be useful but not overfitted
- The system adapts when user behavior changes
- Learned patterns improve categorization accuracy by 20-30%

---

## Prompt 13: User Profile Manager

**What I Want Built:**

I want a management system for user profile data that handles creation, updates, and queries.

**How It Should Behave:**

The Profile Manager should:
- Create user profiles automatically on first run
- Update profiles as new patterns are learned
- Query profiles to get categorization suggestions
- Provide APIs for other components to access profile data
- Handle profile resets and exports
- Maintain profile statistics (patterns learned, confidence scores, etc.)

**Integration Requirements:**

This should be used by the profiling engine and provide data to the categorization integration layer.

**What Success Looks Like:**

- Profile data is always consistent and up-to-date
- Queries for suggestions are fast (< 50ms)
- Profile can be reset without affecting other features
- Profile statistics help users understand what's been learned

---

## Prompt 14: User Profile Viewer Dialog

**What I Want Built:**

I want a dialog where users can see what the system has learned about their behavior.

**How It Should Behave:**

The dialog should:
- Display top learned patterns (e.g., ".pdf files → Documents/PDFs 87% confidence")
- Show folder-specific insights (e.g., "~/Downloads → Documents 95% confidence")
- Display recently learned patterns
- Show patterns with low confidence that might need more data
- Include a "Reset Profile" button to clear all learned data
- Provide an "Export Profile" button for backup

**Integration Requirements:**

This should be accessible from the Help menu and query the Profile Manager for all display data.

**What Success Looks Like:**

- Users can see and understand what the system has learned
- Patterns are displayed clearly with confidence scores
- Users can reset their profile if they want to start over
- The insights help users trust the adaptive system

---

## Prompt 15: Folder Learning Settings Dialog

**What I Want Built:**

I want privacy controls for the folder learning system.

**How It Should Behave:**

The dialog should:
- Allow users to enable/disable folder learning globally
- Provide a list of folders to exclude from learning (e.g., private folders)
- Show which folders have active learning enabled
- Display how much data has been collected per folder
- Include "Clear Folder Data" buttons to remove specific folder insights
- Respect privacy: excluded folders should never be analyzed

**Integration Requirements:**

This should be accessible from the Settings menu and control what data the profiling engine collects.

**What Success Looks Like:**

- Users can protect private folders from analysis
- Learning can be completely disabled if desired
- Excluded folders are never included in patterns
- Users feel in control of their data privacy

---

## Prompt 16: User Profiling Integration

**What I Want Built:**

I want the user profiling system to be integrated into the categorization workflow.

**How It Should Behave:**

The integration should:
- Check for learned patterns before calling the AI
- If a high-confidence pattern exists (>90%), suggest it to the user
- Allow users to accept the suggestion or use AI categorization
- Learn from both AI-suggested and manually-chosen categories
- Provide a "Why this suggestion?" tooltip showing the pattern
- Track which suggestions are accepted to improve confidence scores

**Integration Requirements:**

This should hook into the categorization dialog and work alongside AI categorization without interfering with it.

**What Success Looks Like:**

- Frequent file types get instant suggestions
- Users can still override suggestions when needed
- The system learns from both acceptances and rejections
- Categorization becomes faster as more patterns are learned

---

## Prompt 17: File Tinder Tool

**What I Want Built:**

I want a swipe-style interface for quickly cleaning up folders with keep/delete decisions.

**How It Should Behave:**

The File Tinder interface should:
- Show one file at a time with a preview
- Provide "Keep" (left swipe), "Delete" (right swipe), and "Skip" (up swipe) actions
- Support keyboard shortcuts (arrow keys or K/D/S)
- Show file name, size, type, and modification date
- Queue up files from a selected folder
- Batch the deletions for safety (confirmation before permanent delete)
- Remember decisions in `file_tinder_history` table

**Integration Requirements:**

This should be accessible from the Tools menu and work on any folder the user selects.

**What Success Looks Like:**

- Users can quickly review 100+ files in minutes
- The swipe interface feels smooth and responsive
- Deleted files can be reviewed before permanent deletion
- The tool helps users clean up messy downloads folders

---

## Prompt 18: Menu Integration for All Features

**What I Want Built:**

I want all custom features to be properly integrated into the application's menu system.

**How It Should Behave:**

The menu structure should include:

**Tools Menu:**
- Cache Manager
- Usage Statistics
- File Tinder
- Category Suggestion Wizard
- Whitelist Manager

**Settings Menu:**
- LLM Selection
- Custom LLM Configuration
- Folder Learning Settings

**Help Menu:**
- User Profile Viewer
- Error Reporter
- About

**Integration Requirements:**

All menu items should open their respective dialogs or trigger their actions appropriately.

**What Success Looks Like:**

- Users can find all features easily in logical menu locations
- Menu items are enabled/disabled based on context appropriately
- Keyboard shortcuts work for frequently used items
- The menu feels organized and professional

---

## Prompt 19: Error Reporting System

**What I Want Built:**

I want an automated system that generates detailed error reports when issues occur.

**How It Should Behave:**

The error reporting system should:
- Catch all unhandled exceptions
- Collect system information (OS, app version, database version)
- Include relevant log files
- Sanitize sensitive information (API keys, file paths)
- Generate a formatted error report
- Save reports to the `error_reports` table
- Offer to save the report to a file for sharing
- Include instructions for reporting bugs

**Integration Requirements:**

This should be a global exception handler that catches errors throughout the application.

**What Success Looks Like:**

- When errors occur, users get helpful error reports
- Reports include enough information for debugging
- Sensitive data is never included in reports
- Users can easily share reports with developers

---

## Prompt 20: Category Suggestion Wizard (NEW FEATURE)

**What I Want Built:**

I want a wizard that helps new users set up their category structure by suggesting common patterns.

**How It Should Behave:**

The wizard should:
- Analyze the user's existing file structure
- Suggest common category hierarchies (Documents, Media, Code, etc.)
- Show examples of what goes in each category
- Allow users to customize suggestions
- Import category templates from presets (Home User, Developer, Creative, etc.)
- Export the final category structure for backup
- Run on first launch or be manually accessible

**Integration Requirements:**

This should be accessible from the Tools menu and integrate with the category configuration system.

**What Success Looks Like:**

- New users get started quickly with sensible defaults
- The wizard adapts suggestions based on files it finds
- Users can customize the suggested structure
- Templates provide starting points for different use cases

---

## Prompt 21: Whitelist Manager (NEW FEATURE)

**What I Want Built:**

I want users to be able to exclude specific files or folders from being categorized.

**How It Should Behave:**

The Whitelist Manager should:
- Show a list of current exclusions
- Allow adding files or folders to the whitelist
- Support pattern matching (e.g., "*.tmp", "~/.config/*")
- Provide regex support for advanced patterns
- Mark exclusions as temporary or permanent
- Include import/export functionality for exclusion lists
- Show how many files are currently being excluded

**Integration Requirements:**

This should be checked before processing any file and store exclusions in the `whitelist_entries` table.

**What Success Looks Like:**

- System files and temp files can be excluded automatically
- Users can protect specific folders from being touched
- Pattern matching makes it easy to exclude file types
- Exclusions are respected across all categorization operations

---

## Prompt 22: Translation Manager (NEW FEATURE)

**What I Want Built:**

I want a system for managing UI translations to support multiple languages.

**How It Should Behave:**

The Translation Manager should:
- Load translation strings from the `translation_strings` table
- Support multiple languages (at minimum: English, Spanish, French, German, Portuguese)
- Provide a fallback to English if a translation is missing
- Allow community contributions of new translations
- Include a translation editor dialog for managing strings
- Export/import translation files in JSON format
- Auto-detect system language on first run

**Integration Requirements:**

This should be integrated throughout the UI, replacing all hardcoded strings with translatable strings.

**What Success Looks Like:**

- The entire UI can be displayed in different languages
- Missing translations fall back to English gracefully
- Community members can contribute translations
- Language switching takes effect immediately

---

## Prompt 23: UI Translator Integration (NEW FEATURE)

**What I Want Built:**

I want all UI text to be routed through the translation system.

**How It Should Behave:**

The integration should:
- Replace all hardcoded UI strings with translation keys
- Provide a simple API: `tr("key")` or similar
- Update all dialogs to use translated strings
- Handle pluralization correctly (e.g., "1 file" vs "2 files")
- Support parameter substitution (e.g., "Deleted {count} files")
- Include a language selection dropdown in settings
- Restart the UI when language changes (or reload dynamically)

**Integration Requirements:**

This touches every dialog and UI element in the application.

**What Success Looks Like:**

- All UI text is translatable
- No hardcoded English text remains
- The app feels native in each supported language
- Translations are contextually appropriate

---

## Prompt 24: Error Handling System (NEW FEATURE)

**What I Want Built:**

I want a structured error handling system with error codes and user-friendly messages.

**How It Should Behave:**

The error handling system should:
- Define error codes for all error categories (E001-E999)
- Map error codes to user-friendly messages
- Provide recovery suggestions for each error type
- Log errors with full stack traces
- Show simplified error messages to users
- Include a "Details" button to see technical information
- Integrate with the error reporting system

**Integration Requirements:**

This should replace generic error handling throughout the application.

**What Success Looks Like:**

- Users see clear, helpful error messages
- Error codes help developers identify issues quickly
- Recovery suggestions help users fix problems themselves
- Technical details are available when needed but hidden by default

---

## Prompt 25: UI Polish and Small Enhancements

**What I Want Built:**

I want various small improvements that enhance the overall user experience.

**How It Should Behave:**

The enhancements should include:
- Improved tooltips with helpful descriptions
- Consistent icon usage throughout the app
- Better dialog sizing and layout
- Progress indicators for long operations
- Confirmation dialogs for destructive actions
- Keyboard shortcuts for common actions
- Status bar messages for user feedback
- Improved error dialog styling

**Integration Requirements:**

These are small changes throughout the application that improve polish and consistency.

**What Success Looks Like:**

- The application feels professional and polished
- Users can work efficiently with keyboard shortcuts
- Feedback is always clear and immediate
- The UI is consistent in style and behavior

---

## Prompt 26: Future Feature Foundations

**What I Want Built:**

I want to ensure the database tables for future features are properly set up even if the features aren't fully implemented yet.

**How It Should Behave:**

The foundations should:
- Include example usage patterns for each future table
- Document the intended purpose of each table
- Provide basic CRUD operations for future features
- Include migration notes for when features are activated
- Add placeholder menu items (disabled) for future features
- Document the implementation plan for each feature

**Integration Requirements:**

This is primarily documentation and basic infrastructure that will make future development easier.

**What Success Looks Like:**

- Database tables for future features exist and are documented
- Clear path exists for implementing each future feature
- No technical debt prevents future feature additions
- Code comments explain the vision for incomplete features

---

## Implementation Timeline

**Week 1: Foundation & Core (Prompts 0-2)**
- Days 1-2: Enhanced Categorization Prompt (MOST CRITICAL)
- Days 3-4: Project Setup and Database
- Day 5: Complete Database Schema

**Week 2: Core Features (Prompts 3-7)**
- Day 1: Enhanced Persistent Undo
- Day 2: Dry Run Preview Mode
- Day 3: Cache Manager Dialog
- Days 4-5: API Usage Tracking + Statistics Dialog

**Week 3: AI/LLM Features (Prompts 8-11)**
- Days 1-2: Google Gemini API Client
- Day 3: LLM Selection Dialog
- Day 4: Local LLM Client Support
- Day 5: Custom LLM Configuration

**Week 4-5: User Learning (Prompts 12-16)**
- Days 1-3: User Profiling Core Engine
- Days 4-5: User Profile Manager
- Days 6-7: User Profile Viewer Dialog
- Day 8: Folder Learning Settings
- Days 9-10: User Profiling Integration

**Week 6: Utility Tools (Prompts 17-21)**
- Days 1-2: File Tinder Tool
- Day 3: Menu Integration
- Day 4: Error Reporting System
- Day 5: Category Suggestion Wizard + Whitelist Manager

**Week 7-8: Polish & Quality (Prompts 22-26)**
- Days 1-2: Translation Manager + UI Integration
- Day 3: Error Handling System
- Day 4: UI Polish and Enhancements
- Day 5: Future Feature Foundations
- Days 6-7: Testing and bug fixes
- Days 8-10: Documentation and release preparation

---

## Final Notes

**Testing:** Test each feature thoroughly before moving to the next prompt. Use the acceptance criteria in each prompt to verify success.

**Code Review:** After completing all prompts, review the entire codebase for consistency, security issues, and code quality.

**Documentation:** Update README and user documentation to reflect all new features.

**Release:** Create release v1.6.0-custom with changelog documenting all custom features.

**Backup:** Before starting, backup your current custom fork in case you need to reference the original implementation.

Good luck with your reforking! This comprehensive guide covers every custom feature in the trabalhefabricio fork. 🚀
