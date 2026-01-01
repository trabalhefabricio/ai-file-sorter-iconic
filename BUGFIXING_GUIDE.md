# AI File Sorter - Comprehensive Bug Prevention & Fixing Guide

## Overview
This document provides a systematic approach to bug prevention, detection, and fixing for the AI File Sorter application. It serves as both a reference guide and a checklist for maintaining code quality.

## Code Quality Assessment Summary

### Current State Analysis (January 2026)
The codebase demonstrates **high code quality** with many best practices already in place:

#### ✅ Strengths
1. **Memory Safety**
   - No unsafe C string operations (strcpy, sprintf, strcat)
   - RAII patterns properly used (unique_ptr with custom deleters)
   - Smart pointers preferred over raw new/delete
   - File handles properly managed with RAII (LLMDownloader.cpp:379)

2. **Thread Safety**
   - Mutexes properly used for critical sections (LocalLLMClient.cpp:212-216)
   - Static initialization protected
   - Atomic operations where appropriate

3. **Error Handling**
   - Comprehensive error code system (100+ error codes)
   - Structured exception handling with AppException
   - Proper exception propagation in promise/future code
   - Filesystem errors properly caught and logged

4. **Input Validation**
   - Path validation and sanitization (MovableCategorizedFile.cpp:72-100)
   - Reserved Windows names checked
   - Forbidden characters filtered
   - Length limits enforced (80 chars for labels)

5. **Resource Management**
   - Optional values checked before access
   - Database connections properly managed
   - CURL resources cleaned up

6. **Recent Bug Fixes**
   - Fallback loop prevention (startapp_windows.cpp:749-751)
   - freopen_s return value checking (startapp_windows.cpp:805-816)

## Bug Prevention Checklist

### 1. Memory Safety & Resource Management

#### Before Committing Code:
- [ ] All raw pointers have clear ownership semantics
- [ ] Resources use RAII (unique_ptr, shared_ptr, lock_guard)
- [ ] No manual new/delete except in custom deleters
- [ ] File handles, database connections closed automatically
- [ ] Qt parent-child relationships correctly established
- [ ] Threads either joined or properly detached

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Manual memory management
char* buffer = new char[1024];
// ... use buffer ...
delete[] buffer;  // Can leak on exception

// ✅ GOOD: RAII with smart pointers
auto buffer = std::make_unique<char[]>(1024);
// Automatically cleaned up
```

### 2. Null Pointer & Optional Safety

#### Before Committing Code:
- [ ] All optional<T> checked with has_value() before using value()
- [ ] Pointer returns from functions checked before dereference
- [ ] Logger existence checked: `if (auto logger = Logger::get_logger("name"))`
- [ ] Qt widget pointers validated before use
- [ ] Array/vector indices validated before access

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Unchecked optional access
auto result = some_function();
auto value = result.value();  // Throws if empty

// ✅ GOOD: Check before access
auto result = some_function();
if (result.has_value()) {
    auto value = result.value();
    // use value
}
```

### 3. Thread Safety & Concurrency

#### Before Committing Code:
- [ ] Shared mutable state protected by mutex
- [ ] Lock guards used instead of manual lock/unlock
- [ ] No deadlock potential (consistent lock ordering)
- [ ] Qt signals across threads use QueuedConnection
- [ ] Static variable initialization thread-safe
- [ ] Race conditions in detached threads prevented

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Manual locking can leak on exception
mutex.lock();
do_something();  // If this throws, mutex stays locked
mutex.unlock();

// ✅ GOOD: RAII lock guard
std::lock_guard<std::mutex> lock(mutex);
do_something();  // Mutex unlocked even on exception
```

### 4. Bounds Checking & Array Access

#### Before Committing Code:
- [ ] Vector/array access uses .at() or bounds checking
- [ ] Loop indices validated against container size
- [ ] String operations check for empty strings
- [ ] String::substr validated before use
- [ ] Division checks for zero denominator
- [ ] Integer overflow considered for calculations

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Unchecked array access
auto item = vec[index];  // No bounds check

// ✅ GOOD: Check bounds first
if (index < vec.size()) {
    auto item = vec[index];
}
```

### 5. Error Handling & Exception Safety

#### Before Committing Code:
- [ ] All filesystem operations wrapped in try-catch
- [ ] Network errors properly handled and logged
- [ ] JSON parsing errors caught and reported
- [ ] Database errors logged with context
- [ ] User-facing errors use ErrorCode system
- [ ] Catch-all handlers only where appropriate
- [ ] Error messages are actionable

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Silent failure
try {
    risky_operation();
} catch (...) {
    // Error swallowed, no logging
}

// ✅ GOOD: Log and handle appropriately
try {
    risky_operation();
} catch (const std::exception& e) {
    logger->error("Operation failed: {}", e.what());
    // Take corrective action or propagate
}
```

### 6. Qt-Specific Issues

#### Before Committing Code:
- [ ] All QObjects have proper parent set
- [ ] Signal/slot connections use correct syntax
- [ ] UI operations only on main thread
- [ ] Cross-thread signals use Qt::QueuedConnection
- [ ] Event loop not blocked by long operations
- [ ] Resources not leaked in rejected dialogs
- [ ] Widgets deleted when no longer needed

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Memory leak, no parent
auto dialog = new QDialog();
dialog->exec();
// Dialog never deleted

// ✅ GOOD: Stack allocation or parent
QDialog dialog(this);
dialog.exec();
// Automatically destroyed
```

### 7. Platform-Specific Issues

#### Before Committing Code:
- [ ] Path separators use std::filesystem, not hardcoded
- [ ] Text encoding handled (UTF-8/UTF-16)
- [ ] Windows reserved names checked (CON, PRN, etc.)
- [ ] Case sensitivity differences handled (Windows vs Unix)
- [ ] Line endings normalized (\r\n vs \n)
- [ ] File permissions checked on Unix

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Hardcoded path separator
std::string path = dir + "/" + file;  // Fails on Windows

// ✅ GOOD: Use filesystem library
std::filesystem::path path = dir / file;
```

### 8. API Integration & Network

#### Before Committing Code:
- [ ] Timeouts set for all network operations
- [ ] Retry logic has exponential backoff
- [ ] Rate limiting properly implemented
- [ ] API keys never hardcoded or logged
- [ ] JSON parsing validates structure
- [ ] Network errors display helpful messages
- [ ] Circuit breaker pattern for degraded APIs

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: No timeout, infinite wait
curl_easy_perform(curl);

// ✅ GOOD: Set reasonable timeout
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
curl_easy_perform(curl);
```

### 9. Input Validation & Sanitization

#### Before Committing Code:
- [ ] All user input validated before use
- [ ] Path components sanitized
- [ ] SQL queries use prepared statements
- [ ] Shell commands properly escaped
- [ ] File uploads size-limited
- [ ] Regular expressions bounded

#### Common Pitfalls to Avoid:
```cpp
// ❌ BAD: Unsanitized user input in path
std::filesystem::path dest = base / user_input;

// ✅ GOOD: Sanitize first
std::string sanitized = sanitize_path_label(user_input);
std::filesystem::path dest = base / sanitized;
```

### 10. Logging & Debugging

#### Before Committing Code:
- [ ] Critical operations logged at info level
- [ ] Errors logged with context (file, function)
- [ ] Sensitive data (API keys, paths) redacted
- [ ] Debug logging can be enabled
- [ ] Log messages are actionable
- [ ] Performance-critical paths avoid excessive logging

## Bug Detection Strategies

### 1. Static Analysis
```bash
# Run clang-tidy
clang-tidy app/lib/*.cpp -- -I./app/include

# Run cppcheck (if available)
cppcheck --enable=all --suppress=missingInclude app/
```

### 2. Runtime Analysis
```bash
# Build with AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..

# Build with ThreadSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..

# Build with UndefinedBehaviorSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined" ..
```

### 3. Code Review Checklist
- [ ] All error cases handled
- [ ] Edge cases tested (empty input, max values, etc.)
- [ ] Resource leaks checked
- [ ] Thread safety verified
- [ ] Performance acceptable
- [ ] Code follows project style
- [ ] Comments explain "why", not "what"

### 4. Testing Strategy
- [ ] Unit tests for business logic
- [ ] Integration tests for workflows
- [ ] Edge case tests (empty, null, max values)
- [ ] Error path tests (network failures, disk full)
- [ ] Performance tests for critical paths
- [ ] Manual testing of UI flows

## Common Bug Patterns to Watch For

### 1. Iterator Invalidation
```cpp
// ❌ BAD: Iterator invalidated by erase
for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (should_remove(*it)) {
        vec.erase(it);  // it now invalid!
    }
}

// ✅ GOOD: Use erase-remove idiom or update iterator
vec.erase(std::remove_if(vec.begin(), vec.end(), predicate), vec.end());
```

### 2. String View Lifetime
```cpp
// ❌ BAD: string_view outlives string
std::string_view get_name() {
    std::string temp = "name";
    return temp;  // Returns view to dead string
}

// ✅ GOOD: Return string by value
std::string get_name() {
    return "name";
}
```

### 3. Race Conditions
```cpp
// ❌ BAD: Check-then-act race
if (!map.contains(key)) {
    map[key] = value;  // Another thread could have inserted
}

// ✅ GOOD: Atomic operation with lock
std::lock_guard<std::mutex> lock(mutex);
if (!map.contains(key)) {
    map[key] = value;
}
```

### 4. Resource Ordering
```cpp
// ❌ BAD: Potential deadlock
// Thread 1              Thread 2
lock(mutex_a);          lock(mutex_b);
lock(mutex_b);          lock(mutex_a);  // DEADLOCK!

// ✅ GOOD: Consistent ordering
// Both threads
lock(mutex_a);
lock(mutex_b);
```

## Emergency Bug Fix Protocol

### When a Critical Bug is Reported:

1. **Assess Severity**
   - Security issue? → IMMEDIATE
   - Data loss? → URGENT
   - Crash? → HIGH
   - UI glitch? → MEDIUM
   - Cosmetic? → LOW

2. **Reproduce**
   - Get exact steps to reproduce
   - Identify minimum version affected
   - Check logs for error codes/stack traces

3. **Isolate**
   - Binary search through commits if needed
   - Use git bisect for regression hunting
   - Create minimal test case

4. **Fix**
   - Fix root cause, not symptoms
   - Add test to prevent regression
   - Update error handling if needed
   - Document fix in commit message

5. **Verify**
   - Test the fix thoroughly
   - Run full test suite
   - Check for side effects
   - Verify on all platforms

6. **Deploy**
   - Hotfix for critical issues
   - Include in next release for minor issues
   - Update CHANGELOG.md
   - Notify users if needed

## Continuous Improvement

### Regular Maintenance Tasks
- [ ] Weekly: Review new compiler warnings
- [ ] Monthly: Run static analysis tools
- [ ] Quarterly: Update dependencies
- [ ] Annually: Review and update this guide

### Metrics to Track
- Test coverage percentage
- Number of open bugs by severity
- Mean time to fix bugs
- Regression rate
- Build warning count

### Learning from Bugs
After fixing each bug:
1. Document the bug pattern
2. Add to automated tests
3. Update this guide if needed
4. Share knowledge with team
5. Consider tooling to prevent similar bugs

## Conclusion

This guide represents a **defense-in-depth** approach to code quality:
1. **Prevention**: Write safe code from the start
2. **Detection**: Find bugs early with tools
3. **Isolation**: Minimize bug impact scope
4. **Recovery**: Fix bugs systematically
5. **Learning**: Prevent recurrence

The AI File Sorter codebase already demonstrates many of these best practices. This guide ensures they continue to be applied consistently as the project evolves.

---

**Document Version**: 1.0  
**Last Updated**: January 1, 2026  
**Maintainer**: Development Team
