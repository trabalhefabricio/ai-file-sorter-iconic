# ErrorReporter System

## Overview

The ErrorReporter system provides comprehensive, structured error tracking for AI File Sorter. It captures rich context about errors to enable data-driven debugging and smarter fixes.

## Why We Need This

**Problem:** Users reported `dropEvent@QTableView` DLL errors, but we lacked detailed context about:
- Which Qt version was actually running
- What was in the system PATH
- Which DLLs were loaded
- What environment variables were set
- Error frequency and patterns

**Solution:** ErrorReporter captures all this context automatically and logs it in both human-readable and machine-parseable formats.

## Usage

### Quick Reporting (Recommended)

Use the convenience macros for most cases:

```cpp
// Generic error
REPORT_ERROR(ErrorReporter::Category::STARTUP, 
            ErrorReporter::Severity::CRITICAL,
            "APP_INIT_FAILED", 
            "Failed to initialize application");

// DLL-specific error
REPORT_DLL_ERROR("DLL_DROPEVENT_NOT_FOUND",
                "QTableView::dropEvent symbol not found",
                "Qt6Widgets.dll",
                "?dropEvent@QTableView@@MEAAXPEAVQDropEvent@@@Z");

// Qt initialization error
REPORT_QT_ERROR("QT_WIDGET_CREATE_FAILED",
               "Failed to create QTableView widget");

// Startup error
REPORT_STARTUP_ERROR("LOGGER_INIT_FAILED",
                    "Could not initialize logging system");
```

### Advanced Reporting

For complex scenarios, create a full ErrorContext:

```cpp
ErrorReporter::ErrorContext context;
context.category = ErrorReporter::Category::DLL_LOADING;
context.severity = ErrorReporter::Severity::CRITICAL;
context.error_code = "DLL_VERSION_MISMATCH";
context.message = "Qt DLL version does not match compiled version";
context.dll_name = "Qt6Core.dll";
context.dll_path = "C:\\Windows\\System32\\Qt6Core.dll";
context.dll_version = "6.7.0";
context.missing_symbol = "QTableView::dropEvent";

// Add extra context
context.extra_data["expected_version"] = "6.5.3";
context.extra_data["user_action"] = "Clicked Analyze button";

std::string error_id = ErrorReporter::report_error(context);
```

### Adding Context After Reporting

```cpp
std::string error_id = REPORT_QT_ERROR("WIDGET_FAIL", "Widget creation failed");

// Add more context as you discover it
ErrorReporter::add_context("widget_type", "QTableView");
ErrorReporter::add_context("parent_widget", "CategorizationDialog");
ErrorReporter::add_context("retry_count", "3");
```

## Error Categories

- **DLL_LOADING** - DLL path issues, version mismatches, missing symbols
- **QT_INITIALIZATION** - Qt application startup, widget creation failures
- **STARTUP** - General application startup problems
- **RUNTIME** - Errors during normal operation
- **FILESYSTEM** - File/directory access issues
- **NETWORK** - API calls, downloads
- **DATABASE** - SQLite errors
- **MEMORY** - Allocation failures
- **CONFIGURATION** - Config files, settings
- **USER_ACTION** - User-triggered errors
- **UNKNOWN** - Uncategorized

## Severity Levels

- **CRITICAL** - Application cannot continue, must exit
- **ERROR_HIGH** - Major functionality broken
- **ERROR_MEDIUM** - Minor functionality broken
- **WARNING** - Potential issue, application continues
- **INFO** - Informational, not an error

## Output Files

### errors.log (Human-Readable)

Located at: `%APPDATA%\aifilesorter\logs\errors.log` (Windows) or `~/.cache/aifilesorter/logs/errors.log` (Linux/macOS)

Format:
```
========================================
Error ID: ERR-1736003456789-4523
Category: DLL_LOADING
Severity: CRITICAL
Code: DLL_DROPEVENT_NOT_FOUND
Message: QTableView::dropEvent symbol not found in Qt6Widgets.dll
Location: app/lib/CategorizationDialog.cpp:223 in CategorizationDialog::setup_ui

System Context:
  OS: Windows 10 22H2
  App Version: 1.5.0
  Qt Compile: 6.5.3
  Qt Runtime: 6.7.0
  System PATH (first 5): C:\Program Files\Qt\6.7.0\bin; C:\Windows\System32; ...

DLL Context:
  DLL Name: Qt6Widgets.dll
  DLL Path: C:\Program Files\Qt\6.7.0\bin\Qt6Widgets.dll
  Missing Symbol: ?dropEvent@QTableView@@MEAAXPEAVQDropEvent@@@Z

Environment Variables:
  QT_PLUGIN_PATH: C:\Program Files\AI File Sorter\plugins
  PATH: ...
========================================
```

### errors.jsonl (Machine-Parseable)

Same directory as errors.log, JSON Lines format (one JSON object per line):

```json
{"error_id":"ERR-1736003456789-4523","category":"DLL_LOADING","severity":"CRITICAL","error_code":"DLL_DROPEVENT_NOT_FOUND","message":"QTableView::dropEvent symbol not found","source_file":"app/lib/CategorizationDialog.cpp","source_line":223,"function_name":"CategorizationDialog::setup_ui","system_context":{"os_version":"Windows 10","app_version":"1.5.0","qt_compile_version":"6.5.3","qt_runtime_version":"6.7.0","working_directory":"C:\\Program Files\\AI File Sorter","command_line_args":"aifilesorter.exe --console-log","system_path_preview":"C:\\Program Files\\Qt\\6.7.0\\bin; C:\\Windows\\System32; ..."},"dll_context":{"dll_name":"Qt6Widgets.dll","dll_path":"C:\\Program Files\\Qt\\6.7.0\\bin\\Qt6Widgets.dll","missing_symbol":"?dropEvent@QTableView@@MEAAXPEAVQDropEvent@@@Z"},"env_vars":{"QT_PLUGIN_PATH":"C:\\Program Files\\AI File Sorter\\plugins","PATH":"..."},"timestamp":"2026-01-04T14:30:45"}
```

## Analysis Tools

### Error Frequency Analysis

```cpp
auto frequencies = ErrorReporter::get_error_frequencies();
for (const auto& [error_code, count] : frequencies) {
    std::cout << error_code << ": " << count << " occurrences\n";
}
```

Output:
```
DLL_DROPEVENT_NOT_FOUND: 47
DLL_VERSION_MISMATCH: 12
QT_WIDGET_CREATE_FAILED: 3
```

### Export to JSON

```cpp
ErrorReporter::export_to_json("error_report.json");
```

Generates a formatted JSON file with:
- Summary statistics (total errors, export timestamp)
- All errors in an array
- Suitable for sharing with developers or automated analysis

## Integration Points

### Initialization

In `main.cpp`, after logger setup:

```cpp
bool initialize_loggers() {
    Logger::setup_loggers();
    
    std::string log_dir = Logger::get_log_directory();
    ErrorReporter::initialize(APP_VERSION, log_dir);
    
    return true;
}
```

### DLL Loading Errors (startapp_windows.cpp)

```cpp
if (AddDllDirectory(exeDirW.c_str()) == nullptr) {
    DWORD error = GetLastError();
    REPORT_DLL_ERROR("DLL_PATH_SETUP_FAILED",
                    "Failed to add application directory to DLL search path",
                    "Application directory",
                    std::to_string(error));
}
```

### Qt Initialization Errors (main.cpp)

```cpp
try {
    QApplication app(argc, argv);
} catch (const std::exception& e) {
    REPORT_QT_ERROR("QT_APP_CREATION_FAILED", e.what());
    throw;
}
```

### Widget Creation Errors

```cpp
table_view = new QTableView(this);
if (!table_view) {
    REPORT_QT_ERROR("QTABLEVIEW_CREATE_FAILED", 
                   "Failed to allocate QTableView widget");
    throw std::runtime_error("Widget creation failed");
}
```

## Best Practices

1. **Report Early, Report Often** - Better to have too much data than too little
2. **Use Appropriate Categories** - Helps with filtering and analysis
3. **Meaningful Error Codes** - Use descriptive codes like `DLL_DROPEVENT_NOT_FOUND` not `ERR001`
4. **Add Context** - The more context, the easier to diagnose
5. **Don't Log Secrets** - Avoid logging API keys, passwords, etc.

## Future Enhancements

- Automatic error pattern detection
- Email/webhook notifications for critical errors
- Error dashboard UI
- Automated bug report generation
- Cloud-based error aggregation (opt-in)
- ML-based error classification

## Debugging Tips

### View Recent Errors

```bash
# Linux/macOS
tail -f ~/.cache/aifilesorter/logs/errors.log

# Windows PowerShell
Get-Content "$env:APPDATA\aifilesorter\logs\errors.log" -Tail 50 -Wait
```

### Parse JSON Errors

```bash
# Count errors by category
cat errors.jsonl | jq -r '.category' | sort | uniq -c

# Find all DLL errors
cat errors.jsonl | jq 'select(.category == "DLL_LOADING")'

# Get most recent error
tail -1 errors.jsonl | jq '.'
```

### Export for Analysis

```cpp
// In your test/debug code
ErrorReporter::export_to_json("debug_report.json");
```

Then open `debug_report.json` in your favorite JSON viewer or analysis tool.

## See Also

- `app/include/ErrorReporter.hpp` - Full API documentation
- `app/lib/ErrorReporter.cpp` - Implementation details
- `ERROR_CODE_EXAMPLES.cpp` - Example error codes
- `STARTUP_ERROR_FIXES_SUMMARY.md` - Related startup error fixes
