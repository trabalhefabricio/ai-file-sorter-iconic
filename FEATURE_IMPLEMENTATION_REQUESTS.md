# Feature Implementation Requests
## Project Manager Style Prompts for AI File Sorter Re-fork

These are feature requests written as you would ask a programmer on your team. Each describes WHAT to build and WHY, with clear requirements and acceptance criteria.

---

## MODIFIED FEATURES (Enhancements to Existing Code)

### Feature Request 1: Enhanced Persistent Undo System

**Background:**
Currently, AI File Sorter has a basic undo feature that only works during the current session. If users close the application after sorting files, they lose the ability to undo those moves. This is frustrating for users who realize they made a mistake after closing the app.

**What We Need:**
Enhance the existing UndoManager to save undo plans persistently to the database. Users should be able to undo file moves even after closing and reopening the application.

**Why This Matters:**
- Users often realize they want to undo a sort operation hours or days later
- Provides peace of mind that changes can always be reversed
- Reduces user anxiety about using the automatic sorting feature
- Professional applications always preserve undo history

**Requirements:**

1. **Persistence Layer:**
   - When an undo plan is created, automatically save it to the database
   - Include all file moves (source and destination paths)
   - Store timestamp and session identifier
   - Mark whether the undo has been executed

2. **Load on Demand:**
   - When user clicks "Undo Last Run", first check for in-memory undo
   - If no in-memory undo exists, load the most recent unexecuted undo from database
   - Deserialize the file moves and execute them

3. **Data Format:**
   - Store file moves as JSON: list of objects with "from" and "to" paths
   - Properly escape special characters in paths
   - Handle edge cases like deleted or moved files

4. **Integration:**
   - Modify UndoManager constructor to accept DatabaseManager reference
   - Update MainApp to pass database manager to UndoManager
   - No changes to the UI menu structure

**Acceptance Criteria:**
- User sorts files → closes app → reopens app → clicks "Undo Last Run" → files are restored
- Multiple undo operations are preserved (show most recent first)
- System handles cases where destination folders have been deleted
- Proper error messages if undo cannot be executed

**Technical Notes:**
- Use existing undo_history database table (created in foundation)
- Serialize moves to JSON for storage
- Add helper methods: save_plan_to_database(), load_plan_from_database()

---

### Feature Request 2: Dry Run Preview Mode

**Background:**
Users are often hesitant to click "Confirm & Sort" because they're not sure exactly where their files will end up. They want to see a preview of file movements before executing them.

**What We Need:**
Add a "Dry run (preview only)" checkbox to the categorization dialog. When checked, show a preview of file moves without actually moving any files.

**Why This Matters:**
- Reduces user anxiety about automated file operations
- Allows users to verify categorization before committing
- Helps catch categorization errors before they happen
- Standard feature in similar tools (like git dry-run)

**Requirements:**

1. **UI Enhancement:**
   - Add checkbox labeled "Dry run (preview only, do not move files)" to categorization dialog
   - Place it prominently near the "Confirm & Sort" button
   - Make the label clear that no files will be moved

2. **Preview Dialog:**
   - Create new dialog that displays a table with three columns:
     - File Name
     - Current Location (from)
     - Destination (to)
   - Show the complete destination path for each file
   - Calculate: base_path / category / [subcategory] / filename
   - Add visual warning at top: "PREVIEW ONLY - No files will be moved"
   - Use orange/yellow color scheme to indicate preview mode

3. **Behavior:**
   - When dry run checkbox is checked and user clicks "Confirm & Sort":
     - Show preview dialog instead of moving files
     - Do NOT execute any file moves
     - Do NOT create undo plan
     - Do NOT update database cache
   - When checkbox is unchecked:
     - Normal behavior (move files as usual)

4. **User Experience:**
   - Close button on preview dialog returns to categorization dialog
   - User can uncheck dry run and re-submit to actually move files
   - Preview is instantaneous (no waiting)

**Acceptance Criteria:**
- Check dry run → click Confirm → see preview → no files moved → close preview
- Uncheck dry run → click Confirm → files actually move
- Preview shows correct destination paths for all files
- Warning message is clearly visible
- No performance impact when dry run is disabled

**Technical Notes:**
- Create DryRunPreviewDialog class
- Path calculation must match actual move logic exactly
- Consider filesystem path separators (cross-platform)

---

## NEW FEATURES (Simple - Standalone Dialogs)

### Feature Request 3: Cache Management Dialog

**Background:**
AI File Sorter caches file categorizations to avoid redundant AI API calls. Over time, this cache can grow large and may contain stale data. Users need a way to view and manage this cache.

**What We Need:**
Create a cache management dialog accessible from the Settings menu that shows statistics and provides operations to clear/optimize the cache.

**Why This Matters:**
- Users want to know how much data is being stored
- Cache can become outdated when user's organization style changes
- Database can fragment over time and benefit from optimization
- Transparency builds user trust

**Requirements:**

1. **Statistics Display:**
   - Total number of cached entries
   - Database file size (in KB/MB/GB with appropriate units)
   - Date range: oldest and newest cached entries
   - Display in a clear, easy-to-read format

2. **Cache Operations:**
   - **Clear All Cache**: Button to delete all cached categorizations
     - Show confirmation dialog: "Are you sure? This cannot be undone"
     - Red background to indicate destructive action
   - **Clear Old Cache**: Option to clear cache older than X days
     - Provide spinner input for number of days (1-365)
     - Default to 30 days
     - Show confirmation before clearing
   - **Optimize Database**: Run VACUUM command to reclaim space
     - Explain: "This will compact the database and reclaim unused space"
     - Show success message with space saved

3. **Real-time Updates:**
   - "Refresh Statistics" button to update displayed values
   - Automatically refresh stats after any operation
   - Show loading indicator during operations

4. **User Experience:**
   - Modal dialog
   - Clean, organized layout with groupboxes for Statistics and Operations
   - Appropriate button styling (red for destructive, normal for safe)
   - Close button to dismiss dialog

**Acceptance Criteria:**
- Settings → Manage Cache opens dialog
- Statistics accurately reflect database state
- Clear All works and requires confirmation
- Clear Old respects the days parameter
- Optimize reclaims space and completes successfully
- All operations update statistics display
- Error handling for database failures

**Technical Notes:**
- Use DatabaseManager methods: get_cache_entry_count(), get_database_size(), clear_all_cache(), clear_cache_older_than(), optimize_database()
- Format bytes with appropriate units (B, KB, MB, GB)
- Handle empty cache gracefully

---

### Feature Request 4: Google Gemini API Integration

**Background:**
Currently, AI File Sorter only supports OpenAI API and local LLM. Google Gemini offers a generous free tier (15 requests per minute) which would benefit users who don't want to pay for OpenAI.

**What We Need:**
Implement a Google Gemini API client as an alternative LLM provider, with intelligent free-tier handling including rate limiting and timeout management.

**Why This Matters:**
- Gemini has a generous free tier (users save money)
- Diversifies LLM provider options (reduces vendor lock-in)
- Some users prefer Google's ecosystem
- Gemini 1.5 Flash is fast and capable for categorization

**Requirements:**

1. **API Client Implementation:**
   - Implement ILLMClient interface for consistency
   - Use Gemini REST API (not SDK for simplicity)
   - Endpoint: `https://generativelanguage.googleapis.com/v1/models/{model}:generateContent`
   - Support models: gemini-1.5-flash (default), gemini-1.5-pro
   - Pass API key as URL parameter

2. **Smart Rate Limiting for Free Tier:**
   - Free tier limit: 15 requests per minute (RPM)
   - Implement token bucket algorithm:
     - Start with 15 tokens available
     - Consume 1 token per request
     - Refill 1 token every 4 seconds (15 per minute)
   - Wait if no tokens available (don't fail the request)
   - Per-model state tracking (different models can have different states)

3. **Adaptive Timeout Handling:**
   - Gemini free tier can be slow during high load
   - Start with 20-second timeout
   - If timeout occurs, double the timeout for next request
   - Progression: 20s → 40s → 80s → 160s → 240s (max)
   - Reset to 20s after successful request
   - Log timeout increases for debugging

4. **Retry Logic:**
   - Retry up to 3 times on timeout
   - Use exponential backoff between retries: 1s, 2s, 4s
   - Only retry on timeout errors, not auth/quota errors
   - Reset timeout on success

5. **Settings Integration:**
   - Add "Google Gemini" option to LLM selection dialog
   - Add API key input field (get from: https://aistudio.google.com/app/apikey)
   - Add model selection (dropdown with gemini-1.5-flash, gemini-1.5-pro)
   - Save to settings file

**Acceptance Criteria:**
- User can select Gemini as LLM provider
- Rate limiting prevents quota errors
- Timeout adaptation prevents failures during slow responses
- Retry logic handles transient failures
- Works with free tier API key
- Categorization quality matches OpenAI
- No hanging requests (all have timeouts)

**Technical Notes:**
- Use CURL for HTTP requests
- Parse JSON responses to extract generated text
- Request format: `{"contents":[{"parts":[{"text":"prompt"}]}]}`
- Response format: `candidates[0].content.parts[0].text`
- Static map for per-model rate limit state

---

### Feature Request 5: API Usage Tracking and Statistics

**Background:**
Users paying for OpenAI or other LLM APIs want to monitor their usage and costs. Currently, there's no visibility into how many tokens are being used or how much it's costing them.

**What We Need:**
Implement a system to track all API usage (tokens, requests, costs) and display statistics in a user-friendly dialog.

**Why This Matters:**
- Users need to monitor their spending on AI APIs
- Helps users stay within budget limits
- Shows value proposition (how much automation vs. cost)
- Warns users before they hit expensive quota limits
- Professional applications always provide usage tracking

**Requirements:**

1. **Tracking Infrastructure:**
   - Record every API call made to OpenAI or Gemini
   - Capture: provider, date, tokens used, number of requests, estimated cost, model used
   - Store in database (api_usage_tracking table)
   - Track both successful and failed requests

2. **Cost Estimation:**
   - Maintain current pricing for common models:
     - gpt-4o-mini: $0.15 per 1M input tokens, $0.60 per 1M output tokens
     - gpt-4o: $2.50 per 1M input tokens, $10.00 per 1M output tokens
     - gemini-1.5-flash: FREE (with rate limits)
     - gemini-1.5-pro: $0.35 per 1M input tokens, $1.05 per 1M output tokens
   - Calculate daily and monthly cost estimates
   - Show costs in USD

3. **Statistics Dialog:**
   - Accessible from Tools menu: "API Usage Statistics"
   - Tabs for each provider (OpenAI, Gemini)
   - Display for each:
     - Today's token usage
     - Today's request count
     - Today's estimated cost
     - Monthly estimated cost (based on today's rate)
     - For Gemini free tier: remaining quota (15 RPM, 1500 RPD)
   - Visual indicators:
     - Green: normal usage
     - Yellow: approaching limits (>80% of quota)
     - Red: near limit (>95% of quota)

4. **Integration:**
   - Hook into LLMClient and GeminiClient
   - After each successful API call, record usage
   - After each failed call, record attempt
   - Pass token count from API response

5. **User Experience:**
   - Clean, dashboard-style layout
   - Real-time updates
   - Exportable data (CSV) for expense reports
   - Refresh button to update statistics

**Acceptance Criteria:**
- Every API call is tracked in database
- Statistics accurately reflect usage
- Cost estimates match current API pricing
- Gemini free tier quota warnings work
- Dialog is clear and easy to understand
- No performance impact on categorization
- Handles edge cases (network failures, etc.)

**Technical Notes:**
- APIUsageTracker class with record_request() and get_stats() methods
- UsageStatsDialog with tabbed interface
- Update pricing periodically (comment in code with date)
- Consider approximate token counting for rate limiting

---

## NEW FEATURES (Complex - Multi-Component)

[Content continues in next part due to length...]

### Feature Request 6: User Profiling and Adaptive Learning System

**Background:**
AI File Sorter currently treats every user and every folder the same way. But users have different interests, work patterns, and organizational preferences. The system should learn from how users organize their files and use that knowledge to provide better categorization suggestions.

**What We Need:**
A comprehensive user profiling system that analyzes folders over time, infers user characteristics (hobbies, work patterns, organizational style), and uses this context to improve AI categorization.

**Why This Matters:**
- Personalization dramatically improves categorization accuracy
- Users get better results with less manual correction
- System becomes smarter over time (continuous improvement)
- Differentiating feature that competitors don't have
- Builds user loyalty through personalized experience

**High-Level Architecture:**
1. **Profile Manager** (backend logic)
2. **Profile Viewer Dialog** (UI to see learned profile)
3. **Folder Learning Settings** (UI to configure per-folder privacy)
4. **Integration** (connect to categorization flow)

---

#### Sub-Feature 6A: User Profiling Core Engine

**What We Need:**
Backend system that analyzes categorized files and builds a user profile with confidence-scored characteristics.

**Requirements:**

1. **Profile Data Model:**
   - User ID (default: "default")
   - List of characteristics (trait name, value, confidence score, evidence)
   - Folder insights (analyzed folders with their patterns)
   - Organizational templates (learned category structures)
   - Timestamps for creation and last update

2. **Analysis When Folder is Categorized:**
   - Count files by category and subcategory
   - Calculate category distribution
   - Infer user characteristics:
     - **Hobbies**: Map categories to interests
       - Many Photography files → "Photography hobby" (confidence based on %)
       - Many Music files → "Music hobby"
       - Many Gaming files → "Gaming hobby"
     - **Work Patterns**: Detect professional use
       - Documents + Presentations + Spreadsheets → "Professional user"
       - Code + Projects → "Software developer"
       - Research + Academic → "Student/researcher"
     - **Organization Style**: Analyze structure
       - Many subcategories → "Detail-oriented organizer"
       - Mostly flat structure → "Minimalist organizer"
       - Consistent patterns across folders → "Systematic user"

3. **Confidence Scoring:**
   - Calculate confidence as: (category_count / total_files) * consistency_factor
   - Only store characteristics with confidence > 0.1
   - Update confidence when analyzing new folders
   - Increase confidence when patterns are consistent
   - Decrease confidence when patterns change

4. **Folder Insights:**
   - Store per-folder analysis:
     - Dominant categories
     - File count
     - Last analyzed date
     - Usage pattern (archive, active work, media library, etc.)
   - Track which folders have been analyzed
   - Allow excluding folders from learning

5. **Organizational Templates:**
   - Learn common category patterns from folders
   - Extract: template name, suggested categories, suggested subcategories
   - Calculate confidence based on usage count
   - Merge similar templates automatically

6. **Context Generation for LLM:**
   - Method to generate user context string
   - Format: "User profile: [hobby1], [hobby2], prefers [style] organization, [work pattern]"
   - Include top 10 characteristics by confidence
   - Inject this context into AI prompts
   - Result: AI provides better, personalized suggestions

**Acceptance Criteria:**
- Analyzing photography-heavy folder → infers "Photography hobby"
- Analyzing multiple folders → confidence scores increase
- Generated context is concise and relevant
- No PII (personally identifiable information) in profile
- Profile persists across app sessions
- Can be reset/cleared by user

---

#### Sub-Feature 6B: User Profile Viewer Dialog

**What We Need:**
UI to display the learned user profile in a friendly, understandable format.

**Requirements:**

1. **Dialog Layout:**
   - Tabbed interface with three tabs:
     - **Overview**: High-level summary
     - **Characteristics**: Detailed trait list
     - **Folder Insights**: Analyzed folders

2. **Overview Tab:**
   - Welcome message: "Here's what AI File Sorter has learned about you"
   - Top 5 characteristics with confidence levels (visual bars)
   - Total folders analyzed
   - Profile created date and last updated date
   - Personality summary paragraph (friendly, natural language)

3. **Characteristics Tab:**
   - Table with columns:
     - Trait Type (Hobby, Work Pattern, Style)
     - Value (e.g., "Photography", "Professional user")
     - Confidence (0-100% with visual indicator)
     - Evidence (e.g., "Based on 450 photography files")
   - Sort by confidence (highest first)
   - Color coding: green (>70%), yellow (40-70%), gray (<40%)

4. **Folder Insights Tab:**
   - List of analyzed folders
   - For each folder show:
     - Path
     - Dominant categories (top 3)
     - File count
     - Last analyzed date
     - Usage pattern (determined by system)
   - Option to remove folder from learning (privacy)

5. **User Experience:**
   - Read-only (no editing of profile)
   - "Reset Profile" button (with strong confirmation)
   - Help text explaining what the profile is used for
   - Links to privacy settings

**Acceptance Criteria:**
- Help → View User Profile opens dialog
- All three tabs display correctly
- Data accurately reflects analyzed folders
- Confidence scores make intuitive sense
- Reset Profile clears all data

---

#### Sub-Feature 6C: Folder Learning Settings Dialog

**What We Need:**
UI to configure per-folder privacy settings for the learning system.

**Requirements:**

1. **Privacy Levels:**
   - **Full Learning** (default):
     - Use profile to suggest categories
     - Store learnings from this folder
     - Update user profile based on this folder
   - **Partial Learning**:
     - DON'T use profile for suggestions (treat folder neutrally)
     - DO store learnings from this folder
     - DO update user profile
     - Use case: folder that doesn't represent user's interests (e.g., organizing someone else's files)
   - **No Learning**:
     - DON'T use profile
     - DON'T store learnings
     - DON'T update profile
     - Use case: private/sensitive folders

2. **UI Design:**
   - Simple dialog with radio buttons for three options
   - Clear explanation of each level below radio button
   - Visual icons to indicate privacy level
   - "Learn More" link explaining the feature
   - OK/Cancel buttons

3. **Access:**
   - Settings button (⚙️ icon) next to folder path selector in main window
   - Shows current learning level for selected folder
   - Persists choice in database

**Acceptance Criteria:**
- Settings button opens dialog
- Current level is pre-selected
- Changing level and clicking OK saves to database
- Learning level is respected during categorization
- Clear explanation helps users understand options

---

#### Sub-Feature 6D: Integration with Main Application

**What We Need:**
Connect the user profiling system to the categorization workflow and main UI.

**Requirements:**

1. **Main Window UI:**
   - Add checkbox: "Learn from my organization patterns" (checked by default)
   - Add settings button (⚙️) next to folder path selector
   - Both prominently placed so users notice the feature

2. **Categorization Flow:**
   - If learning is enabled AND folder learning level allows:
     - Get user context: `profile_manager.generate_user_context_for_llm()`
     - Inject context into AI prompt: "User context: [characteristics]"
     - AI receives personalized context for better suggestions
   - After categorization completes:
     - If folder learning level allows storing:
       - `profile_manager.analyze_and_update_from_folder(folder_path, categorized_files)`
       - Update user profile with new insights
       - Store folder insights

3. **Menu Integration:**
   - Help menu → "View User Profile" action
   - Opens UserProfileDialog

4. **First-Time Experience:**
   - Tooltip on learning checkbox explaining the feature
   - Or brief intro dialog on first use: "AI File Sorter can learn..."

**Acceptance Criteria:**
- Learning checkbox visible and works
- Settings button opens folder learning dialog
- Profile context is included in AI prompts when enabled
- Profile updates after each categorization (when allowed)
- Menu item opens profile viewer
- Feature is discoverable by new users

**Technical Notes:**
- Initialize UserProfileManager on app startup
- Pass to CategorizationService
- Handle case where profile doesn't exist yet (first use)

---

## NEW FEATURES (Utilities)

### Feature Request 7: File Tinder - Swipe-Style Cleanup Tool

**Background:**
Users often have folders full of files they're not sure about - some they want to keep, some they want to delete, some they're undecided on. Going through files one-by-one in a file manager is tedious. We need a fun, efficient way to triage files.

**What We Need:**
A "File Tinder" tool that presents files one at a time, allowing users to quickly decide: keep, delete, or skip. Think Tinder for files.

**Why This Matters:**
- Makes cleanup fun instead of tedious
- Much faster than traditional file management
- Reduces cognitive load (one decision at a time)
- Memorable feature that users will tell friends about
- Safe: shows review screen before actually deleting

**Requirements:**

1. **Core Functionality:**
   - Scan a folder and load all files
   - Present files one at a time (full screen experience)
   - Show file preview (image thumbnail, text preview, or metadata)
   - Display file name, size, type, and location

2. **Decision Mechanisms:**
   - **Keyboard controls** (primary):
     - → (Right arrow): Keep this file
     - ← (Left arrow): Mark for deletion
     - ↓ (Down arrow): Skip (ignore this file)
     - ↑ (Up arrow): Revert last decision (undo)
     - R key: Revert last decision (alternative)
   - **Mouse controls** (secondary):
     - ✓ button: Keep
     - ✗ button: Mark for deletion
     - Skip button: Skip
     - Back button: Revert last decision

3. **Progress Tracking:**
   - Progress bar showing current position (e.g., "23 / 150 files")
   - Running count: "Keep: 45 | Delete: 12 | Ignored: 3"
   - Update in real-time as decisions are made

4. **File Preview:**
   - Images: Show thumbnail (max 800x600)
   - Text files: Show first 20 lines
   - Videos: Show first frame or generate thumbnail
   - Other files: Show icon and metadata (size, date modified, file type)

5. **Session Persistence:**
   - Save current state to database (file_tinder_state table)
   - Track: folder path, current index, all decisions
   - Resume if user quits mid-session
   - Clear state after completion

6. **Review Before Deletion:**
   - After going through all files, show summary:
     - "Marked 12 files for deletion (345 MB total)"
     - List all files to be deleted with file names and sizes
   - Require explicit confirmation: "Delete these files permanently?"
   - Option to go back and revise decisions
   - Option to cancel (keep everything)

7. **Safety Features:**
   - Files are not deleted until final confirmation
   - Clear visual distinction for delete-marked files (red background)
   - Show total size being deleted
   - Cannot accidentally delete (requires multiple confirmations)
   - No delete of system files (safety check)

**Acceptance Criteria:**
- Tools → File Tinder opens dialog
- User can navigate through files with arrow keys
- Keep/Delete/Skip decisions are tracked
- Back button works (undo last decision)
- Progress bar updates correctly
- Review screen shows all marked files
- Confirmation required before deletion
- Session saves and resumes correctly
- Deleted files go to recycle bin (not permanent delete)

**User Experience Notes:**
- Fast and responsive (no lag between files)
- Smooth transitions
- Visual feedback for each action
- Keyboard shortcuts displayed prominently
- Fun, game-like feel

**Technical Notes:**
- Use QLabel for preview area
- Handle large folders efficiently (lazy loading)
- Serialize decisions as JSON
- Platform-specific recycle bin (trash on macOS/Linux, recycle bin on Windows)

---

### Feature Request 8: Enhanced Error Reporting System (Optional)

**Background:**
When errors occur in AI File Sorter, users often don't know what went wrong or how to report it. Developers need detailed error information to debug issues. We need a system that makes error reporting easy for users and useful for developers.

**What We Need:**
Automated error reporting system that generates detailed, developer-friendly error reports as markdown files.

**Why This Matters:**
- Users can easily report bugs (just attach the generated file)
- Developers get all the context they need to reproduce issues
- Reduces back-and-forth ("what were you doing when it crashed?")
- Professional applications have good error handling
- Builds user confidence that issues will be fixed

**Requirements:**

1. **Error Detection:**
   - Wrap critical operations in try-catch blocks
   - Define error codes for common failures:
     - DATABASE_INIT_FAILED
     - LLM_CONNECTION_FAILED
     - FILE_MOVE_FAILED
     - CACHE_CLEAR_FAILED
     - API_QUOTA_EXCEEDED
     - etc.

2. **Error Report Generation:**
   - Create markdown file: `COPILOT_ERROR_{timestamp}.md`
   - Include:
     - Error code and human-readable message
     - Timestamp (ISO 8601 format)
     - Operating system and version
     - Application version
     - Context: what the user was trying to do
     - Stack trace (if available)
     - Recent log entries (last 50 lines)
     - System information (Qt version, database size, etc.)
     - Steps to reproduce (if known)

3. **User-Friendly Error Messages:**
   - Show dialog with friendly explanation
   - Example: "Oops! Something went wrong"
   - Explain what happened in simple terms
   - Show path to generated error report
   - Button to "Copy error report path to clipboard"
   - Link to GitHub issues page

4. **Automatic Reporting (Optional):**
   - Option to send error reports automatically (with user consent)
   - Privacy: no personal data, no file paths
   - Only if user opts in during first run

**Acceptance Criteria:**
- Errors generate markdown files automatically
- Error reports contain all necessary debugging information
- Users can easily find and attach error reports
- No personal/sensitive data in reports
- Error dialog is clear and not scary
- Developers can reproduce issues from reports

**Technical Notes:**
- ErrorReporter class with static methods
- ErrorCode enum with all error types
- Integration points in all critical operations
- Consider privacy (sanitize paths)

---

## YET-TO-BE-IMPLEMENTED FEATURES (Future - Foundations Only)

These are features with database tables created but no functionality yet. Implementing the foundation now makes future development easier.

### Feature Request 9: Confidence Scoring System (Foundation)

**Background:**
Not all AI categorizations are equally confident. Some files have clear categories, others are ambiguous. Users should know which categorizations to review manually.

**What We Need (Foundation Only):**
Basic infrastructure to calculate and store confidence scores for categorizations. Full UI and filtering to be added later.

**Requirements:**

1. **Confidence Calculation:**
   - For each categorized file, calculate confidence score (0.0 to 1.0)
   - Factors that increase confidence:
     - Category found in cache (0.9)
     - Clear file name pattern (0.8)
     - Consistent with similar files (0.7)
   - Factors that decrease confidence:
     - Category from LLM with vague reasoning (0.5)
     - Unusual file type (0.4)
     - Conflicting signals (0.3)

2. **Storage:**
   - Save to confidence_scores table after each categorization
   - Include: file path, category confidence, subcategory confidence, factors (as JSON)

3. **Future Use:**
   - Low confidence files can be flagged for manual review
   - Filter UI: show only files below X% confidence
   - Visual indicators in categorization preview

**For Now:**
Just implement the scoring calculation and storage. Don't build UI yet.

---

### Feature Request 10: Session Management System (Foundation)

**Background:**
Large folders can take a long time to categorize. Users should be able to save their progress and resume later.

**What We Need (Foundation Only):**
Basic infrastructure to save and load categorization sessions. Full resume/replay UI to be added later.

**Requirements:**

1. **Session State:**
   - Save current categorization state to database
   - Include: folder path, selected settings, progress (files categorized vs. total)
   - Store as JSON in categorization_sessions table

2. **Session List:**
   - Method to list all saved sessions
   - Show folder path and last updated timestamp

3. **Future Use:**
   - "Resume Session" button in main window
   - "Session History" dialog showing all sessions
   - One-click resume from where you left off

**For Now:**
Just implement session save/load methods. Don't build UI yet.

---

### Feature Request 11: User Corrections Learning System (Foundation)

**Background:**
When users manually change a category in the review dialog, that's valuable feedback. The system should learn from these corrections.

**What We Need (Foundation Only):**
Infrastructure to record user corrections. Analysis and profile updates to be added later.

**Requirements:**

1. **Correction Tracking:**
   - When user changes category in review dialog, record it
   - Save: file path, original category, corrected category, timestamp
   - Store in user_corrections table

2. **Future Use:**
   - Analyze patterns in corrections
   - If user often changes "Documents" to "Work Documents", learn that
   - Adjust user profile based on corrections
   - Show correction statistics in profile viewer

**For Now:**
Just record the corrections. Don't analyze them yet.

---

## IMPLEMENTATION NOTES

**Order of Implementation:**
1. Start with Modified Features (1-2) - they enhance existing code
2. Then Simple New Features (3-5) - standalone dialogs
3. Then Complex Features (6) - multi-component system
4. Then Utilities (7-8) - nice-to-have tools
5. Finally Foundations (9-11) - future preparation

**Each Request Should:**
- Be clear about what to build
- Explain why it matters
- List specific requirements
- Define acceptance criteria
- Note technical considerations

**These are exactly how you'd ask a developer to implement each feature in a real project.**

