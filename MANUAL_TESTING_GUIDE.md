# AI File Sorter - Comprehensive Manual Testing Guide

## Overview

This guide provides step-by-step instructions for manually testing all features of AI File Sorter. Use this guide to validate the application before releases, after major changes, or when investigating issues.

**Last Updated:** 2026-01-13  
**Version Scope:** v1.5.0+

---

## Table of Contents

1. [Pre-Testing Setup](#pre-testing-setup)
2. [Core Features Testing](#core-features-testing)
3. [AI & LLM Integration Testing](#ai--llm-integration-testing)
4. [User Profiling & Learning Testing](#user-profiling--learning-testing)
5. [File Management Features Testing](#file-management-features-testing)
6. [UI/UX Features Testing](#uiux-features-testing)
7. [Platform & Performance Testing](#platform--performance-testing)
8. [Error Handling Testing](#error-handling-testing)
9. [Post-Testing Validation](#post-testing-validation)

---

## Pre-Testing Setup

### 1. Prepare Test Environment

**Required:**
- [ ] Clean installation of AI File Sorter
- [ ] Test data directory with varied file types (20-50 files)
- [ ] Internet connection (for API tests)
- [ ] OpenAI API key (optional, for API tests)
- [ ] Gemini API key (optional, for API tests)

**Test Data Structure:**
```
test_files/
├── documents/
│   ├── report.pdf
│   ├── invoice.xlsx
│   └── notes.txt
├── images/
│   ├── photo1.jpg
│   ├── screenshot.png
│   └── diagram.svg
├── music/
│   ├── song.mp3
│   └── album.flac
└── mixed/
    ├── project.zip
    ├── readme.md
    └── data.csv
```

### 2. Run Diagnostic Tool

Before starting manual tests, run the diagnostic tool:

```bash
# Linux/macOS
python3 diagnostic_tool.py --verbose --output pre_test_diagnostic.json

# Windows
python diagnostic_tool.py --verbose --output pre_test_diagnostic.json
```

**Expected Result:**
- ✓ All critical checks should pass (OK status)
- ⚠ Warnings are acceptable for optional components
- ✗ No critical failures

**Document:**
- [ ] Overall health status
- [ ] Any warnings or failures
- [ ] System configuration details

---

## Core Features Testing

### Test 1: Application Startup

**Objective:** Verify the application launches correctly and UI loads properly.

**Steps:**
1. Launch AI File Sorter
   - Windows: Run `StartAiFileSorter.exe`
   - Linux: Run `./app/bin/run_aifilesorter.sh` or installed launcher
   - macOS: Open AI File Sorter.app

**Expected Results:**
- [ ] Application window opens without errors
- [ ] Main UI elements are visible and properly rendered
- [ ] Menu bar is accessible (File, Edit, Settings, Tools, Help)
- [ ] Status bar shows "Ready" or similar message
- [ ] No error dialogs appear

**If Failed:**
- Check startup logs (`startup_log.txt`)
- Verify all DLLs/libraries are present
- Check Qt version compatibility
- Review diagnostic tool output

---

### Test 2: Directory Selection

**Objective:** Test directory browsing and selection functionality.

**Steps:**
1. Click "Browse" button or folder icon
2. Navigate to test data directory
3. Select directory containing test files
4. Verify path appears in main window

**Expected Results:**
- [ ] File browser dialog opens correctly
- [ ] Can navigate file system
- [ ] Selected path displays in text field
- [ ] Path is valid and readable

**Variations:**
- [ ] Test with empty directory
- [ ] Test with directory containing subdirectories
- [ ] Test with network drive (if available)
- [ ] Test with very long path names

---

### Test 3: File Scanning

**Objective:** Verify file scanning and enumeration works correctly.

**Steps:**
1. Select test directory
2. Configure scan options:
   - [ ] Enable "Include subdirectories"
   - [ ] Enable "Assign subcategories"
3. Click "Analyze" button
4. Observe progress dialog

**Expected Results:**
- [ ] Progress dialog appears
- [ ] File count updates in real-time
- [ ] Current file being processed is shown
- [ ] Can cancel operation with "Stop" button
- [ ] Scan completes without errors

**Document:**
- Number of files scanned: ___________
- Scan duration: ___________
- Any files skipped or failed: ___________

---

## AI & LLM Integration Testing

### Test 4: Local LLM Categorization

**Objective:** Test categorization using local LLM models.

**Steps:**
1. Go to Settings → Select LLM
2. Choose a local LLM (e.g., "LLaMa 3.2 3B")
3. Click OK
4. Select test directory and click "Analyze"
5. Wait for categorization to complete

**Expected Results:**
- [ ] LLM loads successfully (check progress dialog)
- [ ] Files are categorized with reasonable categories
- [ ] Categories are in the selected language
- [ ] Subcategories are assigned (if enabled)
- [ ] Results appear in categorization review dialog

**Backend Tests:**
- [ ] CPU backend: Works without GPU
- [ ] CUDA backend: Works with NVIDIA GPU (if available)
- [ ] Vulkan backend: Works with compatible GPU (if available)
- [ ] Metal backend: Works on macOS Apple Silicon (if available)

**Performance Metrics:**
- Average time per file: ___________
- Total categorization time: ___________
- Memory usage: ___________

**If Failed:**
- Check LLM backend availability (diagnostic tool)
- Verify model files are downloaded
- Check available system resources (RAM, GPU)

---

### Test 5: OpenAI API Categorization

**Objective:** Test categorization using OpenAI ChatGPT API.

**Prerequisites:**
- Valid OpenAI API key
- Internet connection

**Steps:**
1. Go to Settings → Select LLM
2. Choose "ChatGPT (OpenAI API key)"
3. Enter API key
4. Select model (e.g., "gpt-4o-mini")
5. Click OK
6. Run categorization on test directory

**Expected Results:**
- [ ] API key is accepted and stored
- [ ] Connection to OpenAI API succeeds
- [ ] Files are categorized accurately
- [ ] Rate limiting is handled gracefully
- [ ] API errors are reported clearly

**Test Cases:**
- [ ] Valid API key: Categorization works
- [ ] Invalid API key: Clear error message
- [ ] Network issues: Retry logic activates
- [ ] API rate limit: Proper handling and delays

**Cost Tracking:**
- Estimated tokens used: ___________
- Approximate cost: ___________

---

### Test 6: Google Gemini API Categorization

**Objective:** Test categorization using Google Gemini API with free tier optimizations.

**Prerequisites:**
- Valid Gemini API key
- Internet connection

**Steps:**
1. Go to Settings → Select LLM
2. Choose "Google Gemini (Gemini API key)"
3. Enter API key
4. Select model (e.g., "gemini-1.5-flash")
5. Click OK
6. Run categorization on larger test set (30+ files)

**Expected Results:**
- [ ] API key is accepted and stored
- [ ] Connection to Gemini API succeeds
- [ ] Rate limiting (15 RPM) is applied correctly
- [ ] Timeout handling works (20-240s adaptive)
- [ ] Circuit breaker activates after failures
- [ ] Files are categorized successfully

**Free Tier Tests:**
- [ ] Rate limiting prevents quota exhaustion
- [ ] No timeout errors even with slow responses
- [ ] Automatic retry with backoff on transient failures
- [ ] Clear progress indication during waits

**Performance Metrics:**
- Requests per minute: ___________
- Average response time: ___________
- Retry attempts: ___________

---

### Test 7: Categorization Modes

**Objective:** Test "More Refined" vs "More Consistent" categorization modes.

**Test A: More Refined Mode**
1. Select "More Refined" categorization type
2. Categorize test directory with mixed file types
3. Review results

**Expected Results:**
- [ ] Different file types get diverse, specific categories
- [ ] Categories are highly descriptive
- [ ] Similar files may have different categories
- [ ] Subcategories are varied and specific

**Test B: More Consistent Mode**
1. Clear cache (Settings → Manage Cache → Clear All)
2. Select "More Consistent" categorization type
3. Categorize same test directory
4. Review results

**Expected Results:**
- [ ] Similar files get same categories
- [ ] Categories are more uniform
- [ ] Consistency hints visible in prompt logs (if enabled)
- [ ] Fewer unique categories than Refined mode

**Comparison:**
- Refined mode categories: ___________
- Consistent mode categories: ___________
- Difference observed: ___________

---

### Test 8: Category Whitelists

**Objective:** Test whitelist creation, management, and application.

**Steps:**
1. Go to Settings → Manage category whitelists
2. Create new whitelist:
   - Name: "Test Whitelist"
   - Add categories: Documents, Images, Music, Videos
   - Add subcategories: Reports, Photos, Songs, Movies
3. Save whitelist
4. Enable "Use a whitelist" on main window
5. Select "Test Whitelist" from dropdown
6. Run categorization

**Expected Results:**
- [ ] Whitelist is saved successfully
- [ ] Whitelist appears in dropdown
- [ ] All categorized files use only whitelisted categories
- [ ] No categories outside whitelist are used
- [ ] Whitelist persists after app restart

**Management Tests:**
- [ ] Edit existing whitelist: Changes are saved
- [ ] Delete whitelist: Removed from list
- [ ] Multiple whitelists: Can switch between them
- [ ] Disable whitelist: All categories allowed again

---

### Test 9: Multilingual Categorization

**Objective:** Test categorization in different languages.

**Steps:**
1. Go to Settings → Category Language
2. Select non-English language (e.g., Spanish, French, German)
3. Run categorization on test directory
4. Review category names

**Expected Results:**
- [ ] Categories are assigned in selected language
- [ ] Category names are correct and natural
- [ ] Subcategories also in selected language
- [ ] UI language can differ from category language

**Test Each Language:**
- [ ] Dutch: Categories in Dutch
- [ ] French: Categories in French
- [ ] German: Categories in German
- [ ] Italian: Categories in Italian
- [ ] Polish: Categories in Polish
- [ ] Portuguese: Categories in Portuguese
- [ ] Spanish: Categories in Spanish
- [ ] Turkish: Categories in Turkish

---

## User Profiling & Learning Testing

### Test 10: User Profile Building

**Objective:** Test automatic user profile learning from categorizations.

**Steps:**
1. Enable "Learn from my organization patterns" checkbox
2. Categorize multiple directories with distinct content:
   - Photography folder (images)
   - Music library (audio files)
   - Work documents (PDFs, spreadsheets)
3. Go to Help → View User Profile
4. Review learned characteristics

**Expected Results:**
- [ ] Profile dialog opens successfully
- [ ] Overview tab shows summary statistics
- [ ] Characteristics tab shows learned traits
- [ ] Confidence levels increase with more data
- [ ] Evidence is provided for each characteristic

**Characteristics to Verify:**
- [ ] Hobbies detected (e.g., "photography", "music")
- [ ] Work patterns identified (e.g., "document_heavy")
- [ ] Organization style classified (minimalist/balanced/detailed/power)
- [ ] Confidence levels are reasonable (0.0-1.0)

**Profile Evolution:**
1. Note initial characteristics
2. Categorize 2-3 more folders
3. Check profile again
4. Verify confidence levels increased

---

### Test 11: Folder Insights

**Objective:** Test per-folder analysis and insights tracking.

**Steps:**
1. Categorize several different directories
2. Go to Help → View User Profile
3. Switch to "Folder Insights" tab
4. Review folder-specific information

**Expected Results:**
- [ ] All analyzed folders are listed
- [ ] Each folder shows:
  - [ ] Folder path
  - [ ] Description
  - [ ] Dominant categories
  - [ ] File count
  - [ ] Last analyzed timestamp
  - [ ] Usage pattern (work/personal/archive)

**Verify Insights Accuracy:**
- Select a known folder and verify:
  - [ ] Dominant categories match actual content
  - [ ] File count is accurate
  - [ ] Usage pattern is reasonable

---

### Test 12: Per-Folder Learning Control

**Objective:** Test granular learning control per folder.

**Steps:**
1. Select a folder path in main window
2. Click settings/gear icon (⚙️) next to path
3. Folder Learning Dialog should open

**Test Each Learning Level:**

**A. Full Learning (default)**
1. Select "Full Learning"
2. Categorize folder
3. Check profile: Folder should appear in insights
4. Next categorization: Profile should influence suggestions

**B. Partial Learning**
1. Select "Partial Learning"
2. Categorize folder
3. Check profile: Folder appears in insights
4. Next categorization: Profile NOT used for suggestions

**C. No Learning**
1. Select "No Learning"
2. Categorize folder
3. Check profile: Folder does NOT appear in insights
4. Profile completely bypassed

**Expected Results:**
- [ ] Dialog opens and closes properly
- [ ] Selection is saved per folder path
- [ ] Full learning: Both stores and uses profile
- [ ] Partial learning: Stores but doesn't use profile
- [ ] No learning: Neither stores nor uses profile

---

## File Management Features Testing

### Test 13: Dry Run / Preview Mode

**Objective:** Test preview functionality without moving files.

**Steps:**
1. Categorize test directory
2. In results dialog, enable "Dry run (preview only)"
3. Click "Confirm & Sort!"
4. Review preview dialog

**Expected Results:**
- [ ] Dry run preview dialog opens
- [ ] Shows From → To mapping for all files
- [ ] File paths are accurate
- [ ] Can scroll through all planned moves
- [ ] No files are actually moved
- [ ] Can close preview and try again

**Verify Safety:**
1. Note file locations before dry run
2. Run dry run
3. Close preview
4. Verify files are still in original locations

---

### Test 14: Persistent Undo

**Objective:** Test undo functionality across sessions.

**Steps:**
1. Categorize and sort files (with dry run DISABLED)
2. Verify files were moved to new locations
3. Close application
4. Reopen application
5. Go to Edit → "Undo last run"
6. Confirm undo operation

**Expected Results:**
- [ ] Undo menu item is enabled
- [ ] Undo dialog shows files to be restored
- [ ] Files are moved back to original locations
- [ ] Undo report shows success/skip counts
- [ ] Modified files are skipped (not overwritten)

**Edge Cases:**
- [ ] Undo with missing destination files: Skips gracefully
- [ ] Undo with modified files: Skips to prevent data loss
- [ ] Multiple undo attempts: Only most recent available
- [ ] Undo after manual file operations: Handles conflicts

**Document:**
- Files sorted: ___________
- Files restored: ___________
- Files skipped: ___________

---

### Test 15: Sortable Review Table

**Objective:** Test result table sorting and navigation.

**Steps:**
1. Categorize test directory
2. In categorization review dialog:
   - Click "File Name" column header
   - Click "Category" column header
   - Click "Subcategory" column header (if enabled)

**Expected Results:**
- [ ] Table sorts by clicked column
- [ ] Sort order toggles (ascending/descending)
- [ ] Visual indicator shows sort column and direction
- [ ] Sorting is fast and responsive
- [ ] Can sort by multiple columns in sequence

**Usability Tests:**
- [ ] Find specific file: Use name sorting
- [ ] Review category groups: Use category sorting
- [ ] Check subcategory distribution: Use subcategory sorting

---

### Test 16: Database Cache Management

**Objective:** Test cache manager functionality.

**Steps:**
1. Categorize several directories (build up cache)
2. Go to Settings → Manage Cache
3. Review cache statistics

**Test Features:**

**A. View Statistics**
- [ ] Cache entry count displayed
- [ ] Database size shown (KB/MB)
- [ ] Date ranges visible (oldest, newest)
- [ ] Statistics refresh button works

**B. Clear All Cache**
1. Click "Clear All Cache"
2. Confirm dialog appears
3. Confirm deletion
4. Verify cache is empty

**Expected:**
- [ ] Confirmation dialog requires explicit consent
- [ ] All cache entries deleted
- [ ] Statistics update to show 0 entries
- [ ] Next categorization repopulates cache

**C. Clear Old Cache**
1. Set days threshold (e.g., 30 days)
2. Click "Clear Cache Older Than X Days"
3. Confirm operation

**Expected:**
- [ ] Only old entries are deleted
- [ ] Recent entries remain
- [ ] Statistics reflect partial deletion

**D. Optimize Database**
1. Click "Optimize (VACUUM)"
2. Wait for completion
3. Check database size

**Expected:**
- [ ] VACUUM operation completes
- [ ] Database size may reduce
- [ ] No data loss
- [ ] Performance may improve

---

## UI/UX Features Testing

### Test 17: File Tinder Tool

**Objective:** Test swipe-style file cleanup feature.

**Steps:**
1. Go to Tools → File Tinder
2. Select directory with files to review
3. Use arrow keys to triage files:
   - → Right: Keep file
   - ← Left: Delete file
   - ↓ Down: Skip file
   - ↑ Up: Go back

**Expected Results:**
- [ ] File Tinder dialog opens
- [ ] Current file preview shows (image/text/metadata)
- [ ] Arrow key navigation works smoothly
- [ ] Progress bar shows completion percentage
- [ ] Counters update (keep/delete/skip)
- [ ] Can save and resume session

**File Preview Tests:**
- [ ] Images: Thumbnail displayed
- [ ] Text files: Content preview shown
- [ ] Other files: Metadata displayed (size, date, type)

**Session Tests:**
- [ ] Save session: State persists
- [ ] Resume session: Continues from last position
- [ ] Cancel session: Can exit without changes

**Safety Tests:**
1. Mark several files for deletion
2. Review deletion list before confirming
3. Verify marked files are correct
4. Confirm deletion
5. Verify files are deleted (move to trash if possible)

---

### Test 18: File Explorer Dock

**Objective:** Test integrated file system browser.

**Steps:**
1. Go to View → Toggle File Explorer
2. File explorer dock should appear/hide
3. Navigate file system in explorer
4. Click on folder

**Expected Results:**
- [ ] Explorer dock toggles visibility
- [ ] Can navigate entire file system
- [ ] Clicking folder sets it as target in main window
- [ ] Dock is resizable
- [ ] Dock position persists across sessions
- [ ] Shows folders and drives properly

**Navigation Tests:**
- [ ] Expand/collapse folder trees
- [ ] Navigate to home directory
- [ ] Navigate to root/drives
- [ ] Scroll long folder lists

---

### Test 19: Progress Reporting

**Objective:** Test real-time progress feedback during categorization.

**Steps:**
1. Categorize large directory (50+ files)
2. Observe progress dialog during operation

**Expected Results:**
- [ ] Dialog appears immediately when starting
- [ ] Shows current file being processed
- [ ] Progress bar updates smoothly
- [ ] File counter increments (e.g., "15/50")
- [ ] Cache hits vs LLM calls distinguished
- [ ] Stop button is always accessible

**Information Displayed:**
- [ ] Current file name
- [ ] Progress percentage
- [ ] Files processed / total
- [ ] Cache status (hit or miss)
- [ ] Queue of pending files

**Responsiveness:**
- [ ] UI doesn't freeze during processing
- [ ] Can stop at any time
- [ ] Stop request is honored within 2-3 seconds

---

### Test 20: Multilingual UI

**Objective:** Test user interface translations.

**Test Each Language:**
1. Go to Settings → Language
2. Select language
3. Application should restart or update UI
4. Verify all text is translated

**Languages to Test:**
- [ ] English: All UI elements in English
- [ ] Dutch: All UI elements in Dutch
- [ ] French: All UI elements in French
- [ ] German: All UI elements in German
- [ ] Italian: All UI elements in Italian
- [ ] Polish: All UI elements in Polish
- [ ] Portuguese: All UI elements in Portuguese
- [ ] Spanish: All UI elements in Spanish
- [ ] Turkish: All UI elements in Turkish

**Check Translation Coverage:**
- [ ] Menu items
- [ ] Button labels
- [ ] Dialog titles
- [ ] Status messages
- [ ] Error messages
- [ ] Tooltips

**Quality Checks:**
- [ ] No untranslated strings (still in English)
- [ ] Translations are natural and correct
- [ ] UI layout accommodates longer translations
- [ ] No truncated text or overlapping elements

---

### Test 21: Settings Persistence

**Objective:** Verify all settings are saved and restored.

**Steps:**
1. Configure all available settings:
   - Select LLM
   - Set categorization mode
   - Enable/disable options
   - Configure whitelists
   - Set language preferences
2. Close application
3. Reopen application
4. Verify all settings are preserved

**Settings to Verify:**
- [ ] Selected LLM model
- [ ] API keys (stored securely)
- [ ] Categorization mode (Refined/Consistent)
- [ ] Include subdirectories option
- [ ] Assign subcategories option
- [ ] Category language
- [ ] UI language
- [ ] Whitelist selection
- [ ] File explorer visibility
- [ ] Window size and position

---

## Platform & Performance Testing

### Test 22: GPU Backend Detection

**Objective:** Test automatic GPU backend selection.

**Steps:**
1. Check available backends using diagnostic tool
2. Launch application with different backend flags

**Test Scenarios:**

**A. Auto-Detection (default)**
- Launch without flags
- Expected: Best available backend selected (Vulkan > CUDA > CPU)

**B. Force CUDA (if available)**
- Linux: `./run_aifilesorter.sh --cuda=on --vulkan=off`
- Windows: `StartAiFileSorter.exe --cuda=on --vulkan=off`
- Expected: CUDA backend used

**C. Force Vulkan (if available)**
- Launch with `--vulkan=on --cuda=off`
- Expected: Vulkan backend used

**D. Force CPU**
- Launch with `--cuda=off --vulkan=off`
- Expected: CPU (OpenBLAS) backend used

**Verification:**
- [ ] Check startup logs for backend selection
- [ ] Verify LLM inference uses correct backend
- [ ] Monitor GPU utilization (if GPU backend)
- [ ] Compare performance between backends

**Performance Comparison:**
- CPU time per file: ___________
- CUDA time per file: ___________
- Vulkan time per file: ___________
- Metal time per file: ___________ (macOS only)

---

### Test 23: Performance Benchmarking

**Objective:** Measure application performance under different conditions.

**Benchmark A: Small Dataset**
- Files: 10
- Expected time: < 30 seconds (local LLM), < 10 seconds (API)

**Benchmark B: Medium Dataset**
- Files: 50
- Expected time: < 3 minutes (local LLM), < 1 minute (API)

**Benchmark C: Large Dataset**
- Files: 200
- Expected time: < 15 minutes (local LLM), < 5 minutes (API)

**Metrics to Record:**
- Total time: ___________
- Time per file: ___________
- Cache hit rate: ___________
- Memory usage: ___________
- CPU usage: ___________
- GPU usage: ___________ (if applicable)

**Performance Tests:**
- [ ] Cold start (no cache): Time recorded
- [ ] Warm start (with cache): Significant speedup observed
- [ ] Concurrent operations: No UI freezing
- [ ] Large files: Handled without issues
- [ ] Many files: Scales linearly

---

### Test 24: Cross-Platform Compatibility

**Objective:** Verify application works across different platforms.

**Test on Each Platform:**

**Windows:**
- [ ] Windows 10: Application runs
- [ ] Windows 11: Application runs
- [ ] All features work correctly
- [ ] GPU backends available (CUDA, Vulkan)

**macOS:**
- [ ] Intel Mac: Application runs
- [ ] Apple Silicon: Application runs natively
- [ ] Metal backend available
- [ ] All features work correctly

**Linux:**
- [ ] Ubuntu/Debian: Runs from package
- [ ] Fedora/RHEL: Runs from package
- [ ] Arch/Manjaro: Runs from package
- [ ] Build from source works
- [ ] GPU backends available (CUDA, Vulkan)

**Platform-Specific Features:**
- [ ] File paths handled correctly
- [ ] Line endings appropriate
- [ ] Permissions respected
- [ ] Native look and feel

---

## Error Handling Testing

### Test 25: Error Scenarios

**Objective:** Verify application handles errors gracefully.

**Test Scenarios:**

**A. Invalid Directory**
1. Select non-existent directory path
2. Click Analyze

**Expected:**
- [ ] Clear error message displayed
- [ ] Error code shown (e.g., "Error 1234")
- [ ] Resolution steps provided
- [ ] Application remains stable

**B. Permission Denied**
1. Select directory without read permissions
2. Click Analyze

**Expected:**
- [ ] Permission error reported
- [ ] Specific files causing issue identified
- [ ] Can skip problematic files
- [ ] No crash

**C. Network Failure (API)**
1. Disconnect internet
2. Try API-based categorization

**Expected:**
- [ ] Connection error detected
- [ ] Retry logic activates
- [ ] User notified of network issue
- [ ] Can cancel or retry

**D. Invalid API Key**
1. Enter incorrect API key
2. Try categorization

**Expected:**
- [ ] Authentication error reported
- [ ] Prompted to re-enter key
- [ ] No sensitive data in error message
- [ ] Application remains stable

**E. Out of Memory**
1. Try categorizing very large directory (thousands of files)

**Expected:**
- [ ] Memory usage monitored
- [ ] Graceful degradation if memory low
- [ ] Progress saved before any crash
- [ ] Can resume after restart

**F. Database Corruption**
1. Manually corrupt database file (test environment only)
2. Launch application

**Expected:**
- [ ] Corruption detected
- [ ] Database recreated or repaired
- [ ] User notified
- [ ] Application recovers

---

### Test 26: Error Reporting System

**Objective:** Test comprehensive error code system.

**Steps:**
1. Trigger various errors intentionally
2. Review error dialogs and messages

**Error Dialog Features:**
- [ ] Error code displayed prominently (e.g., "Error 1401")
- [ ] User-friendly message shown
- [ ] Resolution steps provided
- [ ] Technical details available
- [ ] "Copy Error Details" button works

**Test Error Categories:**
- [ ] 1000-1099: Network errors
- [ ] 1100-1199: API errors
- [ ] 1200-1299: File system errors
- [ ] 1300-1399: Database errors
- [ ] 1400-1499: LLM errors
- [ ] 1500-1599: Configuration errors

**Error Logs:**
1. Trigger errors
2. Check log files:
   - `logs/errors.log`
   - `logs/COPILOT_ERROR_*.md` (if created)

**Expected:**
- [ ] All errors logged with details
- [ ] Log files are readable
- [ ] Timestamps are accurate
- [ ] Stack traces included (for debugging)

---

## Post-Testing Validation

### Test 27: Data Integrity

**Objective:** Verify no data loss or corruption during testing.

**Checks:**
1. Test files in original locations (after undo)
2. Database integrity check
3. Configuration file valid
4. No orphaned temporary files

**SQL Integrity Check:**
```sql
PRAGMA integrity_check;
PRAGMA foreign_key_check;
```

**Expected Results:**
- [ ] All test files intact
- [ ] No corrupted files
- [ ] Database passes integrity check
- [ ] Configuration is valid
- [ ] No temp files left behind

---

### Test 28: Clean Uninstall

**Objective:** Verify application can be cleanly uninstalled.

**Steps:**
1. Note all installation locations
2. Uninstall application:
   - Windows: Use uninstaller or "Add/Remove Programs"
   - Linux: `sudo make uninstall` or package manager
   - macOS: Delete .app bundle and support files

**Verify Removal:**
- [ ] Executable removed
- [ ] Libraries removed
- [ ] Desktop shortcuts removed
- [ ] Menu entries removed

**User Data:**
- [ ] Configuration files remain (expected)
- [ ] Database remains (expected)
- [ ] User can delete data manually if desired

**Expected Locations:**
- Windows: `%APPDATA%\aifilesorter`
- Linux: `~/.config/aifilesorter`, `~/.local/share/aifilesorter`
- macOS: `~/Library/Preferences/aifilesorter`, `~/Library/Application Support/aifilesorter`

---

## Testing Checklist Summary

### Essential Tests (Must Pass)
- [ ] Test 1: Application Startup
- [ ] Test 3: File Scanning
- [ ] Test 4: Local LLM Categorization
- [ ] Test 13: Dry Run / Preview Mode
- [ ] Test 14: Persistent Undo
- [ ] Test 25: Error Scenarios
- [ ] Test 27: Data Integrity

### Important Tests (Should Pass)
- [ ] Test 2: Directory Selection
- [ ] Test 7: Categorization Modes
- [ ] Test 15: Sortable Review Table
- [ ] Test 16: Database Cache Management
- [ ] Test 19: Progress Reporting
- [ ] Test 21: Settings Persistence
- [ ] Test 23: Performance Benchmarking

### Optional Tests (Nice to Have)
- [ ] Test 5: OpenAI API Categorization
- [ ] Test 6: Google Gemini API Categorization
- [ ] Test 8: Category Whitelists
- [ ] Test 9: Multilingual Categorization
- [ ] Test 10-12: User Profiling Features
- [ ] Test 17: File Tinder Tool
- [ ] Test 18: File Explorer Dock
- [ ] Test 20: Multilingual UI
- [ ] Test 22: GPU Backend Detection
- [ ] Test 24: Cross-Platform Compatibility

### Final Validation
- [ ] Test 28: Clean Uninstall

---

## Test Report Template

After completing tests, document results:

```
Test Date: ___________
Tester: ___________
Platform: ___________
Version: ___________

Tests Passed: ___ / ___
Tests Failed: ___
Tests Skipped: ___

Critical Issues Found:
1. 
2. 
3. 

Non-Critical Issues:
1. 
2. 
3. 

Performance Notes:


Recommendations:


Overall Assessment: [ ] PASS / [ ] FAIL / [ ] CONDITIONAL
```

---

## Troubleshooting Common Issues

### Issue: Application won't start
**Solutions:**
1. Run diagnostic tool for detailed analysis
2. Check `startup_log.txt` for errors
3. Verify all dependencies installed
4. Check permissions on executable

### Issue: LLM categorization fails
**Solutions:**
1. Verify backend availability (CPU/CUDA/Vulkan)
2. Check model files downloaded
3. Ensure sufficient RAM/VRAM
4. Review LLM error logs

### Issue: API categorization fails
**Solutions:**
1. Verify internet connection
2. Check API key validity
3. Review rate limits
4. Check API service status

### Issue: Database errors
**Solutions:**
1. Run database integrity check
2. Clear and rebuild database
3. Check file permissions
4. Verify disk space

### Issue: UI rendering problems
**Solutions:**
1. Verify Qt version compatibility
2. Check graphics drivers
3. Try different Qt platform plugin
4. Review display settings

---

## Additional Resources

- **Diagnostic Tool:** `diagnostic_tool.py`
- **Feature Documentation:** `FEATURE_ANALYSIS.md`
- **Error Codes:** `ERROR_CODES.md`
- **Troubleshooting:** `TROUBLESHOOTING_STARTUP.md`
- **Bug Reporting:** `QUICK_BUG_REPORT.md`

---

## Feedback

If you find issues with this testing guide or have suggestions for additional tests, please:
1. Document the issue in `bugs.md`
2. Create a bug report using `QUICK_BUG_REPORT.md` template
3. Share feedback with the development team

---

**End of Manual Testing Guide**
