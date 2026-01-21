# Enhanced Startup Error Handling

## Overview

This document describes the enhanced error handling system that provides users with better error information and guidance when errors occur during application startup.

## Problem Statement

Users were experiencing errors during application startup and needed:
1. A clear way to understand what went wrong
2. Easy access to detailed error information for troubleshooting
3. Actionable guidance on how to get help or fix issues
4. Context information that can be shared with support or AI assistants (GitHub Copilot)

## Solution

We enhanced the existing error reporting infrastructure with user-friendly dialogs that:
- Display clear error messages
- Show the location of detailed error report files (COPILOT_ERROR_*.md)
- Provide quick access to copy error reports to clipboard
- Offer direct navigation to logs folder
- Include step-by-step guidance for getting help

## Components

### 1. Enhanced DialogUtils

**File:** `app/include/DialogUtils.hpp`, `app/lib/DialogUtils.cpp`

Added `show_startup_error_dialog()` function with the following features:

```cpp
static void show_startup_error_dialog(QWidget* parent, 
                                     const std::string& error_message,
                                     const std::string& error_report_file = "");
```

**Features:**
- Displays error message with contextual icons (📋, 💡)
- Shows error report file path when available
- **Three action buttons:**
  1. **"Copy Error Report"** - Copies entire COPILOT_ERROR_*.md file contents to clipboard
  2. **"Open Logs Folder"** - Opens the logs directory in file explorer
  3. **"Copy Error Info"** - Copies just the error message to clipboard
- **User-friendly guidance:**
  - Tells users exactly what to do with the error information
  - Explains how to use GitHub Copilot for help
  - Provides clear step-by-step instructions

### 2. ErrorReporter Enhancements

**Files:** `app/include/ErrorReporter.hpp`, `app/lib/ErrorReporter.cpp`

Added tracking for the most recent error report file:

```cpp
static std::string get_last_error_report_path();
```

**Changes:**
- Tracks `last_error_report_path_` as a static member
- Updates the path whenever a new COPILOT_ERROR file is created
- Allows exception handlers to reference the detailed error report

### 3. Main Application Exception Handling

**File:** `app/main.cpp`

Enhanced all exception handlers in the main function to use the new dialog system:

**Exception Types Handled:**
1. `ErrorCodes::AppException` - Application-specific errors with error codes
2. `std::runtime_error` - Runtime errors (I/O, network, etc.)
3. `std::exception` - Generic standard exceptions
4. `...` - Unknown/uncaught exceptions

**For Each Exception Type:**
```cpp
// Get error report file if available
std::string error_report_file = ErrorReporter::get_last_error_report_path();

// Build user-friendly error message
std::string error_msg = "Error: " + details + "\n\nGuidance...";

#ifdef _WIN32
if (QApplication::instance()) {
    // Use enhanced dialog with error report access
    DialogUtils::show_startup_error_dialog(nullptr, error_msg, error_report_file);
} else {
    // Fallback to Windows MessageBox
    MessageBoxW(...);
}
#else
// Linux/Mac: stderr output
std::fprintf(stderr, ...);
#endif
```

### 4. Fallback Error Reporting

**File:** `app/main.cpp`

Added fallback error file creation for very early failures (before ErrorReporter initializes):

```cpp
// In initialize_loggers() catch block:
std::string fallback_file = log_dir + "/STARTUP_ERROR.txt";
std::ofstream out(fallback_file);
if (out.is_open()) {
    out << "CRITICAL STARTUP ERROR\n";
    out << "Error: " << e.what() << "\n";
    out << "Troubleshooting guidance...\n";
    out.close();
}
```

## User Experience

### Before Enhancement

When an error occurred:
1. User saw a basic error dialog with technical message
2. No clear guidance on what to do next
3. No easy way to access detailed error information
4. Difficult to provide context when asking for help

### After Enhancement

When an error occurs:
1. **Clear Error Dialog** displays with:
   - Readable error message
   - Location of detailed error report
   - Visual indicators (📋 for reports, 💡 for tips)

2. **Multiple Action Options:**
   - Click "Copy Error Report" to copy full COPILOT_ERROR file
   - Click "Open Logs Folder" to browse error reports
   - Click "Copy Error Info" to copy just the message

3. **Step-by-Step Guidance:**
   ```
   💡 To get help:
   1. Click 'Copy Error Report' below
   2. Paste into GitHub Copilot Chat or GitHub issue
   3. Ask: "How do I fix this error?"
   ```

4. **Easy Troubleshooting:**
   - User clicks one button to copy error report
   - Pastes into Copilot/support channel
   - Gets context-aware help immediately

## Technical Details

### Error Report File Format

The COPILOT_ERROR_*.md files contain:
- Error ID and timestamp
- Error category and severity
- Error code and message
- Source file, line, and function
- Code snippet with context
- System information (OS, Qt versions, etc.)
- Environment context (PATH, working directory, etc.)
- Troubleshooting steps specific to error category
- Ready-made question for GitHub Copilot

### File Naming Convention

```
COPILOT_ERROR_ERR-<timestamp>-<random>.md
```

Example: `COPILOT_ERROR_ERR-1706003456789-4523.md`

### Logs Directory Locations

- **Windows:** `%APPDATA%\aifilesorter\logs\`
- **Linux:** `~/.cache/aifilesorter/logs/`
- **macOS:** `~/Library/Application Support/aifilesorter/logs/`

## Integration Points

### When to Use show_startup_error_dialog()

Use this dialog for:
- ✅ Startup/initialization failures
- ✅ Critical errors that prevent app launch
- ✅ Exceptions caught in main()
- ✅ Any error where detailed report might help user

Do NOT use for:
- ❌ Runtime errors after app is running (use DialogUtils::show_error_dialog())
- ❌ User-triggered validation errors (use appropriate validation dialogs)
- ❌ Non-critical warnings

### Code Pattern

```cpp
try {
    // Critical initialization or operation
} catch (const std::exception& e) {
    // Log the error
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->critical("Error: {}", e.what());
    }
    
    // Get error report file if available
    std::string error_report_file = ErrorReporter::get_last_error_report_path();
    
    // Build clear error message
    std::string error_msg = "Clear description: " + std::string(e.what());
    
    // Show enhanced dialog
    DialogUtils::show_startup_error_dialog(nullptr, error_msg, error_report_file);
    
    return EXIT_FAILURE;
}
```

## Testing

### Manual Test Scenarios

1. **Logger Initialization Failure**
   - Simulate by removing write permissions on logs directory
   - Expected: Dialog shows STARTUP_ERROR.txt location
   - Verify: Can copy error info and open logs folder

2. **Runtime Exception During Startup**
   - Trigger by introducing a test exception in main()
   - Expected: Dialog shows COPILOT_ERROR file location
   - Verify: Can copy full error report with code context

3. **Qt Available vs Not Available**
   - Test both code paths on Windows
   - Expected: Enhanced dialog when Qt available, MessageBox fallback otherwise

4. **Multiple Errors in Sequence**
   - Trigger multiple errors
   - Expected: Each error creates new COPILOT_ERROR file
   - Verify: get_last_error_report_path() returns most recent

### Automated Tests (Future)

Consider adding unit tests for:
- DialogUtils button interactions
- Error report file creation
- get_last_error_report_path() tracking
- Fallback error file creation

## Benefits

### For Users
- ✅ Clear understanding of what went wrong
- ✅ Easy access to detailed diagnostics
- ✅ One-click copy of error information
- ✅ Step-by-step guidance for getting help
- ✅ No need to manually find log files

### For Support/Developers
- ✅ Users provide complete error context
- ✅ Standardized error report format
- ✅ Easier to diagnose issues remotely
- ✅ Better data for bug reports

### For AI Assistants (Copilot)
- ✅ Structured error information
- ✅ Code context included
- ✅ System configuration details
- ✅ Ready-made troubleshooting prompt
- ✅ Can provide targeted solutions

## Future Enhancements

Potential improvements:
1. **Automatic Error Reporting** - Option to send errors anonymously
2. **Error Pattern Detection** - Identify common issues and show targeted fixes
3. **Interactive Troubleshooting** - Guided wizards for common problems
4. **Error History Viewer** - UI to browse past errors
5. **Error Statistics** - Track most common errors for prioritization

## Related Files

- `app/include/DialogUtils.hpp` - Dialog utility declarations
- `app/lib/DialogUtils.cpp` - Dialog implementations
- `app/include/ErrorReporter.hpp` - Error reporting system declarations
- `app/lib/ErrorReporter.cpp` - Error reporting implementation
- `app/main.cpp` - Main application entry point with exception handling
- `app/include/ErrorCode.hpp` - Error code definitions
- `ERROR_REPORTING_FOR_COPILOT_USERS.md` - User-facing documentation

## Changelog

### 2026-01-21 - Initial Implementation
- Added `show_startup_error_dialog()` to DialogUtils
- Added `get_last_error_report_path()` to ErrorReporter
- Enhanced all exception handlers in main.cpp
- Added fallback error file for pre-initialization failures
- Documentation created

---

*This enhancement directly addresses the user need for meaningful error information that can be easily shared "no matter what context" as stated in the problem statement.*
