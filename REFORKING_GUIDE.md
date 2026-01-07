# Complete Reforking Guide

## Overview

This document contains everything you need to re-fork the original `hyperfield/ai-file-sorter` repository while preserving all custom features from this fork. The guide is organized as sequential implementation prompts that you can feed to an AI coding agent.

---

## Implementation Prompts

### Prompt 0: Enhanced Categorization Prompt (MOST CRITICAL)

**Priority:** CRITICAL - Implement this FIRST before any other features

I want to enhance the core categorization prompt to make file sorting much more intelligent than the original fork.

**Current Situation (Original Fork):**
- The original fork uses a simple prompt that just says "Categorize this file: [filename]"
- It doesn't consider file extensions or file types
- It has no confidence threshold
- Results in many incorrect categorizations

**What I Want:**

I want the categorization logic to be completely redesigned so that:

1. **File Type Awareness**: The AI should analyze the file extension (like .cpp, .json, .pdf) to understand what type of file it is and what it's typically used for

2. **Semantic Analysis**: The AI should understand the semantic meaning of both the filename AND the extension together, not just the raw text

3. **Path Context**: If the file path is available, include it in the analysis to provide additional context about where the file is located

4. **Confidence Threshold**: The AI should only categorize files when it's at least 70% confident. If confidence is below 70%, it should return "UNCERTAIN : [filename]" instead of guessing

5. **Structured Analysis Framework**: The AI should be instructed to analyze three specific aspects:
   - What this file type (with its extension) is typically used for
   - The semantic meaning of the filename itself
   - Common purposes and applications for this file format

6. **Response Format**: The response should be either:
   - "Category : Subcategory" (if confident)
   - "UNCERTAIN : [filename]" (if not confident)
   - No explanations, no additional text

**Where This Happens:**
- This logic needs to be in both the OpenAI client and the Gemini client (since we'll add Gemini support later)
- The system prompt and user prompt construction for categorization requests

**Why This Matters:**
- This is THE most important difference between the original fork and our custom fork
- It dramatically improves categorization accuracy
- It prevents bad guesses and allows manual categorization of uncertain files
- It makes the AI context-aware instead of just pattern-matching filenames

**Success Criteria:**
- When categorizing a file like "config.json", the AI should understand it's a JSON configuration file
- When categorizing "data.bin" with no context, it should return UNCERTAIN
- When categorizing "report_2024.pdf", the AI should understand it's a PDF document and consider the date in the filename
- The confidence threshold prevents random guessing

---

### Prompt 1: Project Setup and Database Foundation

I want to set up a fresh fork of the original repository with the proper project structure and database enhancements.

**What I Want:**

1. **Fork Setup**: 
   - Fork the original hyperfield/ai-file-sorter repository
   - Clone the fork locally
   - Verify the project builds successfully
   - Test the basic categorization workflow

2. **Database Manager Enhancements**:
   - I want helper methods added to the DatabaseManager class to make it easier to work with custom features
   - I want methods for serializing and deserializing JSON data for complex objects
   - I want methods for batch operations on database records
   - I want better error handling for database operations

**Why This Matters:**
- Clean starting point ensures we can easily sync with upstream updates
- Enhanced database utilities will be used by all the custom features we'll add
- Proper foundation prevents technical debt

**Success Criteria:**
- Fresh fork exists and builds successfully
- Database helper methods are available for use by other features
- Can perform basic file categorization to verify the fork works

---

### Prompt 2: Complete Database Schema (16 Tables)

I want to create a comprehensive database schema that supports all current and future features.

**What I Want:**

I need 16 database tables created to support various features. The tables should be organized into two groups:

**Active Feature Tables (8 tables):**

1. **api_usage_log**: Track every API call with timestamp, provider (OpenAI/Gemini), tokens used, cost estimation
2. **undo_plans**: Store undo plans with JSON serialization so undo works across application restarts
3. **user_profiles**: Store user preferences and learned patterns per user
4. **folder_insights**: Track categorization patterns for specific folders
5. **pattern_learning**: Record frequently used categorization patterns
6. **cache_metadata**: Store cache statistics and optimization data
7. **categorization_history**: Log of all categorization operations for analysis
8. **error_logs**: Structured error logging for debugging and reporting

**Future-Ready Tables (8 tables):**

9. **confidence_scores**: Foundation for tracking AI confidence levels per categorization
10. **content_analysis_cache**: Prepare for caching file content analysis results
11. **categorization_sessions**: Support for save/resume categorization workflows
12. **user_corrections**: Learn from manual corrections to improve future suggestions
13. **enhanced_undo_history**: Multi-level undo with branching support
14. **file_relationships**: Track related files for intelligent grouping
15. **custom_rules**: User-defined categorization rules
16. **performance_metrics**: Application performance tracking

**Requirements:**
- All tables should have proper primary keys
- Use foreign keys where relationships exist
- Add indices on frequently queried columns
- Include created_at timestamps
- Use appropriate data types (INTEGER, TEXT, REAL, BLOB for JSON)

**Why This Matters:**
- Creates the data foundation for all features
- Future-ready tables mean we can implement new features without schema migrations
- Proper indexing ensures performance at scale

**Success Criteria:**
- All 16 tables exist in the database
- Tables have proper constraints and relationships
- Can insert and query test data in each table

---

### Prompt 3: Enhanced Persistent Undo System

I want the undo system to save undo plans to the database so users can undo file moves even after closing and reopening the application.

**Current Situation:**
- The original fork has basic undo functionality
- Undo history is lost when the application closes
- Users can't undo operations from previous sessions

**What I Want:**

1. **Persistent Undo Plans**:
   - When files are categorized and moved, I want the undo plan saved to the undo_plans database table
   - The undo plan should include: original paths, new paths, timestamp, operation type
   - Plans should be serialized as JSON for easy storage and retrieval

2. **Cross-Session Undo**:
   - When the application starts, I want it to load any existing undo plans from the database
   - Users should see undo options from previous sessions
   - Old undo plans (>30 days) should be automatically cleaned up

3. **Undo UI Integration**:
   - I want an "Undo Last Operation" menu item or button
   - It should show what will be undone (like "Undo: Moved 15 files on 2024-01-05")
   - It should be disabled if there's nothing to undo

**Why This Matters:**
- Users often realize they need to undo something after they've closed the app
- Persistent undo provides safety and confidence
- It's a significant usability improvement over the original fork

**Success Criteria:**
- Categorize some files, close the app, reopen it, and successfully undo the operation
- Multiple undo operations can be stacked and undone in reverse order
- The UI accurately reflects whether undo is available

---

### Prompt 4: Dry Run Preview Mode

I want users to be able to preview file moves before they actually happen.

**What I Want:**

1. **Dry Run Checkbox**:
   - I want a checkbox in the categorization dialog labeled "Preview mode (don't actually move files)"
   - When checked, the categorization should proceed but NOT actually move files
   - Instead, it should show what WOULD happen

2. **Preview Dialog**:
   - After dry run categorization completes, I want a dialog that shows:
     - List of files that would be moved
     - Original location → New location for each file
     - Category assignments
   - Users should be able to:
     - Review the preview
     - Export the preview to a text file
     - Decide to proceed with the real categorization or cancel

3. **Clear Indication**:
   - The preview dialog should clearly state "PREVIEW MODE - No files were actually moved"
   - Different color scheme or icon to distinguish from real operations

**Why This Matters:**
- Users want to verify categorization results before making changes
- Prevents mistakes and gives confidence
- Allows experimentation with different settings
- Users can review large categorization jobs before committing

**Success Criteria:**
- Dry run categorizes files but doesn't actually move them
- Preview dialog shows all planned moves accurately
- Can proceed with real categorization after reviewing preview

---

### Prompt 5: Cache Manager Dialog

I want a dialog where users can view cache statistics and manage the categorization cache.

**What I Want:**

1. **Cache Statistics Display**:
   - I want a dialog that shows:
     - Total cache size
     - Number of cached categorizations
     - Hit rate percentage
     - Last cache optimization date
     - Oldest and newest cache entries

2. **Cache Management Operations**:
   - Button to clear all cache
   - Button to clear old cache entries (>30 days)
   - Button to optimize database (VACUUM command)
   - Each operation should ask for confirmation

3. **Real-time Updates**:
   - Statistics should update after any operation
   - Show progress during optimization
   - Display success/error messages

4. **Menu Integration**:
   - Add "Cache Manager" to the Tools menu
   - Should be easily accessible

**Why This Matters:**
- Cache can grow large over time
- Users want control over data storage
- Provides transparency about what the app is storing
- Optimization can improve performance

**Success Criteria:**
- Cache statistics are accurate
- Clear cache operations work correctly
- Database optimization completes successfully
- Dialog accessible from Tools menu

---

### Prompt 6: Google Gemini API Client (NEW FEATURE)

I want to add Google Gemini as an alternative LLM provider so users aren't limited to OpenAI.

**Current Situation:**
- The original fork ONLY supports OpenAI API
- There is no Gemini integration
- This is a completely new feature

**What I Want:**

1. **Gemini API Client**:
   - I want a new client that implements the same interface as the OpenAI client
   - It should support the Gemini REST API
   - It should use Gemini's specific API format and endpoints

2. **Rate Limiting and Retry Logic**:
   - Gemini has different rate limits than OpenAI
   - I want automatic retry with exponential backoff for rate limit errors
   - I want respect for Gemini's free tier quotas

3. **Configuration**:
   - Add Gemini to the LLM provider selection (dropdown with "OpenAI" and "Gemini")
   - Store Gemini API key separately in settings
   - Allow switching between providers

4. **Feature Parity**:
   - Gemini client should support:
     - File categorization requests (same as OpenAI)
     - The enhanced categorization prompt from Prompt 0
     - Error handling and logging
     - API usage tracking

**Why This Matters:**
- Gemini has a generous free tier (saves money)
- Provides fallback if one service has issues
- Users can choose their preferred LLM provider
- Diversifies dependencies

**Success Criteria:**
- Can select Gemini as LLM provider in settings
- Categorization works identically with Gemini as with OpenAI
- API key configuration works for Gemini
- Rate limiting prevents quota errors

---

### Prompt 7: API Usage Tracking System

I want to track all API calls to monitor token consumption and estimate costs.

**What I Want:**

1. **Automatic Tracking**:
   - Every API call (OpenAI or Gemini) should be automatically logged to the api_usage_log table
   - Track: timestamp, provider, model, tokens used, estimated cost, operation type

2. **Token Counting**:
   - Count tokens in requests and responses
   - Use approximate counts if exact counting isn't available
   - Store token counts for later analysis

3. **Cost Estimation**:
   - Calculate estimated cost based on current pricing:
     - OpenAI GPT-4: ~$0.03 per 1K tokens
     - Gemini: free tier, then ~$0.001 per 1K tokens
   - Accumulate costs over time

4. **Integration**:
   - Tracking should be transparent (doesn't affect normal operation)
   - Should work with both OpenAI and Gemini clients
   - Should handle errors gracefully (tracking failure shouldn't break categorization)

**Why This Matters:**
- Users want to know how much they're spending on API calls
- Helps identify expensive operations
- Allows monitoring usage against quotas
- Provides data for optimization

**Success Criteria:**
- Every API call is logged correctly
- Token counts are reasonably accurate
- Cost estimates make sense
- Can query usage history from database

---

### Prompt 8: Usage Statistics Dialog

I want a dialog that displays API usage statistics and costs.

**What I Want:**

1. **Statistics Display**:
   - I want a dialog showing:
     - Total API calls (by provider)
     - Total tokens used
     - Estimated total cost
     - Usage breakdown by date/week/month
     - Most expensive operations
     - Average tokens per request

2. **Filtering and Grouping**:
   - Filter by date range
   - Filter by provider (OpenAI vs Gemini)
   - Group by day, week, or month

3. **Visual Representation**:
   - Simple charts or tables showing trends
   - Color-code by provider
   - Highlight high-cost operations

4. **Export**:
   - Ability to export statistics to CSV
   - Useful for expense tracking

5. **Menu Integration**:
   - Add "API Usage Statistics" to the Tools menu

**Why This Matters:**
- Users want visibility into costs
- Helps justify API expenses
- Can identify optimization opportunities
- Transparency builds trust

**Success Criteria:**
- Statistics accurately reflect api_usage_log data
- Filtering works correctly
- Export produces valid CSV
- Dialog accessible from Tools menu

---

### Prompt 9: User Profiling Core Engine

I want a system that learns from user behavior to provide better categorization suggestions.

**What I Want:**

1. **Pattern Learning**:
   - I want the system to track which categories users choose for different file types
   - Track folder-specific patterns (e.g., "files in ~/Documents/Work often go to Work/Projects")
   - Track extension-based patterns (e.g., "user always puts .sql files in Development/Database")
   - Store patterns in user_profiles and folder_insights tables

2. **Profile Analysis**:
   - Analyze categorization history to identify consistent patterns
   - Calculate confidence scores for patterns (how often is this pattern followed?)
   - Update patterns automatically as more data is collected

3. **Intelligent Suggestions**:
   - When categorizing a file, check if it matches learned patterns
   - Suggest categories based on:
     - File extension + user history
     - Source folder + user history
     - Filename patterns + user history
   - Only suggest if confidence is high (>80%)

4. **Privacy Controls**:
   - Learning should be opt-in
   - Ability to clear learned patterns
   - Ability to disable learning for specific folders

**Why This Matters:**
- System becomes personalized over time
- Reduces manual categorization effort
- Learns user's organizational style
- Becomes more accurate with use

**Success Criteria:**
- After categorizing 20+ files, system identifies patterns
- Suggestions match user's actual preferences
- Patterns are stored and retrieved correctly
- Learning can be disabled

---

### Prompt 10: User Profile Manager

I want a manager class that handles profile creation, updates, and queries.

**What I Want:**

1. **Profile Management**:
   - Create new user profiles (one per user/computer)
   - Update profiles as new categorization data comes in
   - Query profiles for suggestions

2. **Folder Insights**:
   - Track insights per folder (which folders have strong patterns)
   - Calculate "learning confidence" for each folder
   - Identify folders where manual categorization is needed vs automated

3. **Pattern Storage**:
   - Store patterns as structured data (not just text)
   - Support querying patterns by extension, folder, or filename pattern
   - Efficient lookups during categorization

4. **Data Management**:
   - Clean up old patterns that are no longer relevant
   - Merge similar patterns
   - Handle pattern conflicts (when user changes behavior)

**Why This Matters:**
- Centralizes profile logic
- Makes pattern lookup efficient
- Handles complexity of learning system
- Enables future profile features

**Success Criteria:**
- Profiles are created automatically
- Pattern lookups are fast (<10ms)
- Old data is cleaned up appropriately
- Manager handles edge cases gracefully

---

### Prompt 11: User Profile Viewer Dialog

I want a UI where users can see what the system has learned about their preferences.

**What I Want:**

1. **Profile Overview**:
   - Display profile statistics:
     - Total categorizations
     - Number of patterns learned
     - Folders with strong patterns
     - Most common categories used

2. **Pattern List**:
   - Show learned patterns in a readable format:
     - "Files with .cpp extension → Development/Source"
     - "Files from ~/Downloads → Needs Review"
     - "Files matching pattern *report*.pdf → Documents/Reports"

3. **Pattern Management**:
   - Ability to delete individual patterns
   - Ability to edit pattern confidence
   - Ability to mark patterns as "always use" or "never use"

4. **Visual Feedback**:
   - Color-code patterns by confidence level
   - Show how many times each pattern has been applied
   - Show pattern accuracy (% of times it matches user choice)

5. **Menu Integration**:
   - Add "View User Profile" to the Help menu (or Tools menu)

**Why This Matters:**
- Transparency about what system has learned
- User control over automated suggestions
- Builds trust in the learning system
- Allows correction of bad patterns

**Success Criteria:**
- Profile displays accurate information
- Can see and understand all learned patterns
- Pattern management operations work
- Dialog accessible from menu

---

### Prompt 12: Folder Learning Settings Dialog

I want users to have fine-grained control over what the system learns.

**What I Want:**

1. **Global Learning Toggle**:
   - Master switch to enable/disable all learning
   - Clear explanation of what learning does
   - Shows current status (enabled/disabled)

2. **Folder-Specific Controls**:
   - List of folders that have been analyzed
   - Toggle learning per folder
   - Add folders to "never learn" list
   - Add folders to "always learn" list

3. **Data Management**:
   - Button to clear all learned data
   - Button to clear data for specific folders
   - Button to export learned patterns (backup)
   - Button to import patterns (restore)
   - All operations should ask for confirmation

4. **Privacy Information**:
   - Clear explanation that all learning is local (no cloud)
   - Explanation of what data is stored
   - Where data is stored (database file location)

5. **Menu Integration**:
   - Add "Learning Settings" to the Tools menu or Settings

**Why This Matters:**
- Privacy is important to users
- Gives users control over automation
- Allows disabling learning for sensitive folders
- Builds trust through transparency

**Success Criteria:**
- Can toggle learning on/off globally
- Can control learning per folder
- Data clearing operations work correctly
- Privacy information is clear and accurate

---

### Prompt 13: User Profiling Integration

I want user profiling integrated into the main categorization workflow.

**What I Want:**

1. **Automatic Pattern Application**:
   - During categorization, check if file matches learned patterns
   - If high-confidence pattern exists, suggest it to user
   - If user accepts suggestion, increment pattern confidence
   - If user rejects suggestion, decrement pattern confidence

2. **UI Integration**:
   - Show suggested category with confidence level
   - Visual indicator that suggestion comes from learned patterns (e.g., brain icon)
   - Option to use suggestion or ignore it
   - Option to always use pattern for similar files

3. **Learning During Categorization**:
   - As user categorizes files, update patterns in background
   - Don't interrupt categorization flow
   - Show brief notification when new pattern is learned

4. **Smart Defaults**:
   - Pre-fill category field with high-confidence suggestions
   - Allow manual override
   - Remember when user overrides (learn new patterns)

**Why This Matters:**
- Makes learning system actually useful
- Speeds up categorization workflow
- Adapts to user over time
- Seamless integration (doesn't feel like separate feature)

**Success Criteria:**
- Suggestions appear during categorization
- Accepting suggestions speeds up workflow
- Learning happens automatically in background
- Manual overrides update patterns correctly

---

### Prompt 14: Menu Integration for All Features

I want all new features properly integrated into the application menu structure.

**What I Want:**

1. **Tools Menu Additions**:
   - "Cache Manager" → Opens cache management dialog
   - "API Usage Statistics" → Opens usage statistics dialog
   - "Learning Settings" → Opens folder learning settings dialog
   - "File Tinder" → Opens file tinder tool (we'll implement this next)

2. **Help Menu Additions**:
   - "View User Profile" → Opens user profile viewer dialog
   - Keep existing help items

3. **Settings/Configuration**:
   - LLM Provider selection (OpenAI / Gemini)
   - API key configuration for both providers
   - Other existing settings preserved

4. **Menu Organization**:
   - Logical grouping of related items
   - Keyboard shortcuts for frequently used items
   - Separators between groups
   - Proper ordering (most used items more accessible)

**Why This Matters:**
- Features need to be discoverable
- Good menu structure improves usability
- Keyboard shortcuts improve workflow
- Professional appearance

**Success Criteria:**
- All menu items are correctly labeled
- All menu items open correct dialogs
- Menu organization is logical
- Keyboard shortcuts work

---

### Prompt 15: File Tinder Tool

I want a swipe-style interface for quickly deciding which files to keep or delete.

**What I Want:**

1. **Swipe UI**:
   - Show files one at a time (like Tinder)
   - Display: filename, path, size, preview (if possible)
   - Large buttons or swipe gestures:
     - "Keep" (right swipe or green button)
     - "Delete" (left swipe or red button)
     - "Skip" (up swipe or yellow button)

2. **File Queue**:
   - Load all files from selected folder
   - Show progress (e.g., "File 5 of 120")
   - Allow jumping back to previous file
   - Allow skipping ahead

3. **Delete Confirmation**:
   - Files marked for deletion go to "pending deletion" list
   - Show summary before actual deletion
   - User must confirm before any files are deleted
   - Option to move to trash instead of permanent delete

4. **Filtering**:
   - Filter by file type
   - Filter by size (e.g., only files >10MB)
   - Filter by date (e.g., older than 1 year)

5. **Statistics**:
   - Show how much space would be freed
   - Show how many files kept vs deleted
   - Show time taken

**Why This Matters:**
- Makes file cleanup fast and even fun
- Reduces decision paralysis
- Safe (confirmation before deletion)
- Useful for cleaning up downloads folder

**Success Criteria:**
- Can quickly review files
- Keep/Delete/Skip operations work correctly
- Deletion requires confirmation
- Shows accurate statistics

---

### Prompt 16: Error Reporting System

I want an automated system for generating error reports when something goes wrong.

**What I Want:**

1. **Automatic Error Detection**:
   - Catch and log all exceptions
   - Log error details: type, message, stack trace, timestamp
   - Store in error_logs database table

2. **Error Report Generation**:
   - When error occurs, offer to generate error report
   - Report should include:
     - Error details
     - System information (OS, Qt version, app version)
     - Recent logs
     - Database state summary
     - Configuration settings (sanitized, no API keys)

3. **User Control**:
   - User can review report before saving
   - User can add notes describing what they were doing
   - Can save report to file
   - Optional: can submit report (if we add that feature later)

4. **Privacy**:
   - No personally identifiable information in reports
   - No file paths that might reveal personal info
   - No API keys or passwords
   - Clear about what's included

**Why This Matters:**
- Makes bug reporting easier
- Helps with debugging
- Users can share reports for support
- Improves software quality

**Success Criteria:**
- Errors are caught and logged
- Reports contain useful debugging information
- Privacy is respected
- Users can easily save reports

---

### Prompt 17: UI Polish and Small Enhancements

I want various small UI improvements that make the application feel more polished.

**What I Want:**

1. **Improved Tooltips**:
   - Add helpful tooltips to all buttons and controls
   - Explain what each feature does
   - Include keyboard shortcuts in tooltips

2. **Better Error Messages**:
   - User-friendly error messages (not just technical details)
   - Suggestions for how to fix common errors
   - Clear next steps

3. **Progress Indicators**:
   - Show progress during long operations
   - Estimated time remaining
   - Ability to cancel long operations

4. **Consistent Styling**:
   - Consistent button sizes and spacing
   - Consistent dialog sizes
   - Consistent color scheme
   - Proper alignment

5. **Keyboard Navigation**:
   - Tab order makes sense
   - Enter/Escape work as expected
   - Keyboard shortcuts for common actions

6. **Status Messages**:
   - Brief success messages after operations
   - Clear indication when operations fail
   - Non-intrusive notifications

**Why This Matters:**
- Professional appearance
- Better user experience
- Reduces user confusion
- Makes app feel complete

**Success Criteria:**
- UI feels polished and professional
- Tooltips are helpful
- Error messages are clear
- Keyboard navigation works well

---

### Prompt 18: Future Feature Foundations

I want to set up the foundation for future features using the unused database tables.

**What I Want:**

1. **Confidence Scoring System**:
   - Use confidence_scores table to track AI confidence for each categorization
   - Store: file_id, category, confidence percentage, timestamp
   - Query interface for finding low-confidence categorizations
   - Useful for improving categorization quality

2. **Session Management**:
   - Use categorization_sessions table to enable save/resume
   - Store: session_id, files to process, progress, settings
   - Allow users to pause large categorization jobs
   - Resume from where they left off

3. **User Corrections Learning**:
   - Use user_corrections table to learn from manual corrections
   - When user manually recategorizes a file, store the correction
   - Analyze corrections to improve future suggestions
   - Feed corrections back into learning system

4. **Enhanced Undo History**:
   - Use enhanced_undo_history for multi-level undo with branching
   - Store undo tree structure
   - Allow undo/redo with branching support
   - Visual representation of undo history

**What to Implement Now:**
- Just the basic table structures and query functions
- Simple examples of how to use each table
- Documentation of the intended use cases
- Placeholder functions that can be filled in later

**Why This Matters:**
- Makes future development easier
- Tables are ready when we need them
- Shows roadmap for future features
- Database schema is complete

**Success Criteria:**
- Can insert sample data into future tables
- Query functions exist but may return empty results
- Documentation explains intended use
- No errors when tables are accessed

---

## Summary

These 19 prompts cover everything needed to replicate all custom features from this fork:

**Critical Foundation:**
- Prompt 0: Enhanced Categorization Prompt ⚠️ MOST IMPORTANT

**Core Infrastructure:**
- Prompt 1: Project Setup and Database Foundation
- Prompt 2: Complete Database Schema (16 Tables)

**Modified Features:**
- Prompt 3: Enhanced Persistent Undo System
- Prompt 4: Dry Run Preview Mode

**Simple New Features:**
- Prompt 5: Cache Manager Dialog
- Prompt 6: Google Gemini API Client (NEW)
- Prompt 7: API Usage Tracking System
- Prompt 8: Usage Statistics Dialog

**Complex New Features:**
- Prompt 9: User Profiling Core Engine
- Prompt 10: User Profile Manager
- Prompt 11: User Profile Viewer Dialog
- Prompt 12: Folder Learning Settings Dialog
- Prompt 13: User Profiling Integration

**Integration and Polish:**
- Prompt 14: Menu Integration for All Features
- Prompt 15: File Tinder Tool
- Prompt 16: Error Reporting System
- Prompt 17: UI Polish and Small Enhancements
- Prompt 18: Future Feature Foundations

## Implementation Timeline

**Week 1**: Prompts 0-2 (Foundation)
**Week 2**: Prompts 3-5 (Modified Features)
**Week 3**: Prompts 6-8 (Simple New Features)
**Week 4-5**: Prompts 9-13 (User Profiling)
**Week 6**: Prompts 14-15 (Integration & File Tinder)
**Week 7**: Prompts 16-18 (Error Reporting & Polish)

Total: ~7 weeks with AI assistance

## Usage Instructions

1. Fork hyperfield/ai-file-sorter
2. Clone your fork
3. Copy Prompt 0 to your AI coding agent
4. Implement, test, and commit
5. Move to Prompt 1
6. Repeat for all prompts in order

Each prompt is designed to be independent and complete. Feed them to your AI agent one at a time, review the generated code, test it, and commit before moving to the next prompt.
