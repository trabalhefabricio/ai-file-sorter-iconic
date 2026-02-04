# Code Structure Improvements

This document describes the structural improvements made to the AI File Sorter codebase to improve reliability, maintainability, and extensibility.

## Overview

The improvements focus on five key areas:

1. **Unified Error Handling** - Consistent error types and result patterns
2. **Input Validation** - Centralized validation with clear error messages  
3. **LLM Service Abstraction** - Unified interface for all LLM backends
4. **Component Extraction** - Breaking down large classes into focused components
5. **Configuration Management** - Type-safe configuration with validation

---

## 1. Unified Error Handling (`Result.hpp`)

### Problem
The codebase used inconsistent error handling:
- Some functions threw exceptions
- Others returned `std::optional` without error details
- Some returned empty strings or `-1` to indicate errors
- Error messages were scattered and inconsistent

### Solution
Introduced a `Result<T>` type that can hold either a value or an error:

```cpp
#include "Result.hpp"
using namespace afs;

// Function that can fail
Result<int> divide(int a, int b) {
    if (b == 0) {
        return make_error(ErrorCode::InvalidInput, "Division by zero");
    }
    return a / b;
}

// Using the result
auto result = divide(10, 2);
if (result) {
    int value = result.value();  // 5
} else {
    Error err = result.error();
    log_error(err.format());  // "InvalidInput: Division by zero"
}
```

### Error Categories

Errors are organized into categories:
- **Validation** (100-199): Input validation errors
- **FileSystem** (200-299): File/directory access issues
- **Network** (300-399): Network connectivity issues
- **Api** (400-499): API-related errors
- **Database** (500-599): Database operations errors
- **LLM** (600-699): LLM inference errors
- **Internal** (900-999): Internal/programming errors

### Benefits
- **Type-safe**: Compiler ensures errors are handled
- **Informative**: Errors carry code, message, and details
- **Chainable**: Operations can be composed with `map()` and `and_then()`
- **Consistent**: All functions use the same pattern

---

## 2. Input Validation (`InputValidator.hpp`)

### Problem
Validation logic was duplicated across many files:
- Path validation in MainApp, FileScanner, Settings
- API key validation in LLMClient, GeminiClient
- Category validation in WhitelistStore, CategorizationService

### Solution
Centralized all validation in `InputValidator`:

```cpp
#include "InputValidator.hpp"
using namespace afs;

// Validate a directory path
auto result = InputValidator::validate_directory_path("/path/to/dir", 
    true /* must_exist */, true /* must_be_writable */);
if (!result) {
    show_error(result.error().message);
}

// Validate an API key
auto key_result = InputValidator::validate_api_key(api_key, "OpenAI");
if (!key_result) {
    // Error message includes provider name: "OpenAI API key cannot be empty"
}

// Sanitize user input for filesystem use
std::string safe_name = InputValidator::sanitize_filename(user_input);
```

### Validation Functions
- `validate_directory_path()` - Check directory exists/writable
- `validate_file_path()` - Check file exists
- `validate_api_key()` - Check API key format
- `validate_category_label()` - Check category name for filesystem safety
- `validate_non_empty()` - Check string is not empty/whitespace
- `validate_model_name()` - Check model identifier
- `is_reserved_filename()` - Check for Windows reserved names
- `sanitize_filename()` - Make user input filesystem-safe

### Benefits
- **DRY**: Validation logic in one place
- **Consistent**: Same rules applied everywhere
- **Comprehensive**: Handles edge cases (reserved names, special chars)
- **Testable**: Easy to unit test validation logic

---

## 3. LLM Service Abstraction (`LLMService.hpp`)

### Problem
LLM client creation was scattered:
- MainApp created clients directly
- Different interfaces for local vs. remote LLMs
- No consistent error handling across backends
- Hard to add new LLM providers

### Solution
Created a unified `LLMService` interface with factory methods:

```cpp
#include "LLMService.hpp"
using namespace afs;

// Create from settings (recommended)
auto service_result = LLMService::create_from_settings(settings, logger);
if (!service_result) {
    handle_error(service_result.error());
    return;
}
auto llm_service = std::move(service_result).value();

// Check if ready
if (auto ready = llm_service->check_ready(); !ready) {
    show_error(ready.error().message);
    return;
}

// Categorize a file
auto cat_result = llm_service->categorize(
    "document.pdf", "/path/to/document.pdf", FileType::File,
    context, &cancel_flag);

if (cat_result) {
    auto& result = cat_result.value();
    process(result.category, result.subcategory, result.duration);
}
```

### Interface
```cpp
class LLMService {
public:
    static Result<std::unique_ptr<LLMService>> create(const LLMConfig& config, ...);
    static Result<std::unique_ptr<LLMService>> create_from_settings(const Settings&, ...);

    virtual Result<CategorizationResult> categorize(...) = 0;
    virtual Result<std::string> complete(...) = 0;
    virtual Result<void> check_ready() const = 0;
    virtual bool is_local() const = 0;
    virtual std::string display_name() const = 0;
};
```

### Benefits
- **Unified**: Same interface for all LLM backends
- **Factory pattern**: Easy to add new providers
- **Error handling**: Consistent error reporting
- **Cancellation**: Built-in support for operation cancellation
- **Progress**: Callback support for long operations

---

## 4. Component Extraction

### Problem
`MainApp` was 1500+ lines handling too many responsibilities:
- File explorer management
- Settings persistence
- Analysis workflow
- Results display
- Localization
- Development tools

### Solution
Extract focused components:

#### FileExplorerManager
Handles all file explorer sidebar functionality:

```cpp
#include "FileExplorerManager.hpp"

// In MainApp
file_explorer_ = std::make_unique<FileExplorerManager>(this, settings_);
file_explorer_->setup();
file_explorer_->set_directory_selected_callback([this](const QString& path, bool user) {
    on_directory_selected(path, user);
});
file_explorer_->restore_state();
```

### Planned Components
- `SettingsSync` - Settings persistence and UI synchronization
- `AnalysisOrchestrator` - File analysis workflow coordination  
- `ResultsPresenter` - Results display and undo management
- `LocalizationManager` - Language switching and translation

### Benefits
- **Single Responsibility**: Each class does one thing
- **Testable**: Small components are easier to test
- **Maintainable**: Changes isolated to relevant component
- **Reusable**: Components can be used in TUI or future UIs

---

## 5. Configuration Management (`ConfigSchema.hpp`)

### Problem
Settings access was weakly typed:
- Raw string/int values from INI file
- Validation scattered across codebase
- Easy to use wrong key names
- No default value management

### Solution
Type-safe configuration schema:

```cpp
#include "ConfigSchema.hpp"
using namespace afs;

// Define schema with defaults and validation
ConfigSchema config;
config.openai_api_key = ConfigValue<std::string>{"", 
    validators::non_empty("OpenAI API Key")};
config.categorized_file_count = ConfigValue<int>{0,
    validators::min_value(0, "File count")};

// Set value with validation
if (auto result = config.openai_api_key.set(user_input); !result) {
    show_error(result.error().message);
}

// Get value (strongly typed)
std::string key = config.openai_api_key.get();

// Validate all settings
auto errors = config.validate_all();
for (const auto& err : errors) {
    log_warning(err.format());
}
```

### Benefits
- **Type-safe**: No string key mistakes
- **Validated**: Invalid values rejected at set time
- **Documented**: Schema serves as documentation
- **Testable**: Validation logic is testable

---

## Migration Guide

### Using Result Types

**Before:**
```cpp
std::string categorize(const std::string& name) {
    if (name.empty()) throw std::invalid_argument("Empty name");
    // ... may throw other exceptions
}
```

**After:**
```cpp
Result<std::string> categorize(const std::string& name) {
    if (name.empty()) {
        return make_error(ErrorCode::EmptyInput, "File name cannot be empty");
    }
    // ... return errors instead of throwing
}
```

### Using InputValidator

**Before:**
```cpp
if (path.empty() || !std::filesystem::exists(path)) {
    show_error("Invalid path");
    return;
}
```

**After:**
```cpp
if (auto result = InputValidator::validate_directory_path(path); !result) {
    show_error(result.error().message);
    return;
}
```

### Using LLMService

**Before:**
```cpp
std::unique_ptr<ILLMClient> client;
if (settings.get_llm_choice() == LLMChoice::Remote_OpenAI) {
    client = std::make_unique<LLMClient>(key, model);
} else {
    client = std::make_unique<LocalLLMClient>();
}
std::string result = client->categorize_file(...);
```

**After:**
```cpp
auto service = LLMService::create_from_settings(settings, logger);
if (!service) {
    show_error(service.error().message);
    return;
}
auto result = (*service)->categorize(...);
if (result) {
    process(result->category, result->subcategory);
}
```

---

## Testing

New unit tests added:
- `test_result.cpp` - Result type operations
- `test_input_validator.cpp` - Input validation

Run tests:
```bash
cmake -S app -B build -DAI_FILE_SORTER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 6. Repository Pattern (`CategorizationRepository.hpp`)

### Problem
Database access was tightly coupled:
- Direct SQLite calls throughout codebase
- Hard to mock for testing
- No transactional boundaries
- Inconsistent error handling

### Solution
Created `ICategorizationRepository` interface:

```cpp
#include "CategorizationRepository.hpp"
using namespace afs;

// Create repository
auto repo = CategorizationRepositoryFactory::create_sqlite(config_dir);
if (!repo) {
    handle_error(repo.error());
    return;
}

// Use repository
auto files = (*repo)->find_by_directory("/path/to/dir");
if (files) {
    for (const auto& file : files.value()) {
        process(file);
    }
}

// Transaction support
{
    auto guard = TransactionGuard::begin(**repo);
    if (!guard) return guard.error();
    
    (*repo)->save(entry1);
    (*repo)->save(entry2);
    
    guard->commit();
}
```

### Benefits
- **Testable**: Easy to mock with `create_memory()`
- **Consistent**: All operations return `Result<T>`
- **Transactional**: RAII guard for transactions
- **Extensible**: Could add different backends

---

## 7. Analysis Orchestrator (`AnalysisOrchestrator.hpp`)

### Problem
Analysis logic was scattered:
- MainApp handled scanning, caching, LLM calls, and progress
- Hard to test individual pieces
- Complex state management
- No clear workflow

### Solution
Created `AnalysisOrchestrator` to coordinate the workflow:

```cpp
#include "AnalysisOrchestrator.hpp"
using namespace afs;

AnalysisOrchestrator orchestrator(
    settings, repository, llm_service, scanner, logger);

AnalysisConfig config;
config.directory_path = "/path/to/analyze";
config.scan_options = FileScanOptions::Files | FileScanOptions::Directories;
config.use_consistency_hints = true;

// Validate before running
if (auto result = orchestrator.validate_config(config); !result) {
    show_error(result.error().message);
    return;
}

// Run with callbacks
AnalysisCallbacks callbacks;
callbacks.on_progress = [](const AnalysisProgress& p) {
    update_ui(p.processed_files, p.total_files);
};
callbacks.on_file_categorized = [](const CategorizedFile& f) {
    add_to_list(f);
};

auto result = orchestrator.run(config, callbacks, &cancel_flag);
if (result) {
    show_results(result->categorized_files);
}
```

### Benefits
- **Clear Workflow**: Defined sequence of operations
- **Testable**: Can inject mock dependencies
- **Progress Reporting**: Structured callbacks
- **Cancellation**: Built-in support

---

## Future Improvements

1. **Complete Component Extraction** - Finish extracting MainApp components
2. **Async Operations** - Use `std::future` or Qt concurrent for background work
3. **Dependency Injection** - Make components injectable for testing
4. **Event System** - Use Qt signals/slots or events for loose coupling
5. **Module System** - Consider C++20 modules for faster compilation
