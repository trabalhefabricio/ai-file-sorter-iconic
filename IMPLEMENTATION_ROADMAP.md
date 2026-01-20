# AI File Sorter - Implementation Roadmap

**Created:** January 20, 2026  
**Status:** Planning Complete - Ready for Implementation  
**Approach:** Separate PRs per Feature

---

## Overview

This document outlines the implementation plan for adding 5 major features to AI File Sorter, based on comprehensive bug analysis of the newstuff branch.

---

## Current PR: Bug Analysis Foundation

**PR:** `copilot/list-new-features-added`  
**Status:** ✅ Complete  
**Deliverable:** BUG_ANALYSIS_REPORT.md

This PR provides:
- Analysis of 12 bugs found in newstuff branch (2 CRITICAL, 4 HIGH, 6 MEDIUM)
- Detailed bug descriptions with file locations and line numbers
- Code snippets showing each issue
- Reproduction steps
- Recommended fixes
- Foundation for all future feature PRs

---

## Feature Implementation Plan

Each feature will be implemented in a **separate PR** with bug fixes from the analysis.

### PR #1: Enhanced Gemini API with Sophisticated Rate Limiting

**Branch:** `feature/enhanced-gemini-api`  
**Dependencies:** None  
**Bugs to Fix:**
- Bug #1: Detached thread with dangling pointer (CRITICAL)
- Bug #3: Unhandled std::stol exceptions (HIGH)
- Bug #4: Data race in progress callback (HIGH)
- Bug #8: Thread-unsafe static state (MEDIUM)
- Bug #12: Bare catch(...) blocks (LOW)

**Implementation:**
- Token bucket algorithm with dynamic refill rate
- Per-model state tracking
- Progressive timeout extension (20s → 240s)
- Circuit breaker pattern
- Advanced jitter for retry scheduling
- Adaptive capacity adjustment

**Testing:**
- Unit tests for rate limiter
- Integration tests with mock API
- Stress tests for concurrent requests
- Thread safety verification with ThreadSanitizer

---

### PR #2: File Tinder Tool

**Branch:** `feature/file-tinder`  
**Dependencies:** None  
**Bugs to Fix:**
- Bug #6: Silent database operation failures (MEDIUM)
- Bug #11: Generic file error messages (LOW)

**Implementation:**
- Swipe-style file cleanup interface
- Arrow key navigation (←/→/↑/↓)
- File preview (images, text, metadata)
- Review marked files before deletion
- Session state persistence
- Database-backed decision tracking

**Testing:**
- UI interaction tests
- File preview tests for multiple formats
- Database persistence tests
- Keyboard navigation tests

---

### PR #3: Enhanced Whitelist Tree Editor + Management

**Branch:** `feature/whitelist-editor`  
**Dependencies:** None  
**Bugs to Fix:**
- Bug #2: Null pointer dereferences (CRITICAL)
- Bug #5: Memory leak in on_remove_item (HIGH)
- Bug #10: Missing null checks in item_to_node (MEDIUM)

**Implementation:**
- Hierarchical tree-based category structure
- Visual whitelist editing with drag-and-drop
- Keyboard shortcuts for quick editing
- Category/subcategory relationship management
- Import/export whitelist configurations
- Proper Qt memory management

**Testing:**
- Tree manipulation tests
- Null pointer safety tests
- Memory leak detection with AddressSanitizer
- Import/export validation tests

---

### PR #4: Cache Management Dialog

**Branch:** `feature/cache-manager`  
**Dependencies:** None  
**Bugs to Fix:**
- Bug #9: Premature success messages (MEDIUM)
- Bug #7: SQL statement resource leaks (MEDIUM)

**Implementation:**
- View cache statistics (entry count, size, dates)
- Clear all cache with confirmation
- Clear cache older than N days
- Optimize database (VACUUM) to reclaim space
- Real-time statistics refresh
- Proper error handling and user feedback

**Testing:**
- Cache clearing tests
- Database optimization tests
- Error handling tests
- UI feedback tests

---

### PR #5: Content-Based File Sorting

**Branch:** `feature/content-based-sorting`  
**Dependencies:** Enhanced Gemini API (PR #1)  
**Bugs to Fix:** None (new feature)

**Implementation:**
- Enhanced categorization prompts with file type knowledge
- Examples: "serum" → "VST plugin" → "Music Production" folder
- File extension to type mapping database
- Common application file recognition
- Fallback to filename-based categorization

**Prompt Enhancement Examples:**
```
Before: "Categorize file: serum.dll"
After:  "Categorize file: serum.dll (VST audio plugin used in music production)"

Before: "Categorize file: project.flp"
After:  "Categorize file: project.flp (FL Studio project file for music production)"
```

**Testing:**
- File type detection tests
- Prompt enhancement tests
- Categorization accuracy tests
- Fallback mechanism tests

---

### PR #6: Comprehensive Diagnostic Tool (FINAL)

**Branch:** `feature/diagnostic-tool`  
**Dependencies:** ALL previous feature PRs  
**Purpose:** Test and validate all implemented features

**Bug Testing Scope:**
- **Original 12 bugs** from BUG_ANALYSIS_REPORT.md
- **Accumulated bugs** discovered during PRs #1-5
- **Regression detection** - ensure no bugs were reintroduced
- **New issue detection** - identify any problems in integrated system

**Implementation:**
- Feature-by-feature testing framework
- Automated bug detection for ALL identified bugs (original + accumulated)
- Simulation of feature operations:
  - Gemini API rate limiting scenarios
  - File Tinder workflow simulations
  - Whitelist editor operations
  - Cache management operations
  - Content-based sorting tests
- Detailed error reporting with:
  - Exact file locations
  - Line numbers
  - Stack traces
  - Reproduction steps
- JSON report generation for CI/CD
- Exit codes indicating severity

**Testing:**
- Test the tester - ensure diagnostic tool catches known issues
- Performance benchmarks
- Regression detection
- CI/CD integration tests

---

## Bug Fix Priority

### Must Fix Before Feature Implementation

**CRITICAL (Block All)**:
1. Detached thread with dangling pointer (GeminiClient)
2. Null pointer dereferences (WhitelistTreeEditor)

**HIGH (Block Related Features)**:
3. Unhandled exceptions (GeminiClient) - Block PR #1
4. Data race (GeminiClient) - Block PR #1
5. Memory leak (WhitelistTreeEditor) - Block PR #3
6. Silent DB failures (FileTinderDialog) - Block PR #2

**MEDIUM (Fix During Implementation)**:
7. SQL statement leaks (DatabaseManager) - Fix in PR #4
8. Thread-unsafe static state (GeminiClient) - Fix in PR #1
9. Premature success messages (CacheManagerDialog) - Fix in PR #4
10. Missing null checks (WhitelistTreeEditor) - Fix in PR #3

**LOW (Nice to Have)**:
11. Generic error messages (FileTinderDialog) - Fix in PR #2
12. Bare catch blocks (GeminiClient) - Fix in PR #1

---

## Development Guidelines

### For Each Feature PR:

1. **Start from main branch** (not newstuff)
2. **Reference bug analysis** - Link to BUG_ANALYSIS_REPORT.md
3. **Fix identified bugs** before adding functionality
4. **Document any NEW bugs found** - Add to PR description for inclusion in final diagnostic tool
5. **Add comprehensive tests**:
   - Unit tests for core logic
   - Integration tests for feature workflows
   - UI tests where applicable
   - Memory safety tests (AddressSanitizer)
   - Thread safety tests (ThreadSanitizer)
6. **Update documentation**:
   - Add feature description to README
   - Document new API/UI elements
   - Include usage examples
7. **Run sanitizers**:
   ```bash
   cmake -DCMAKE_CXX_FLAGS="-fsanitize=address,thread,undefined" ..
   make
   ctest
   ```
8. **Code review** before merging

### Bug Accumulation Strategy:

**Important**: All bugs discovered during feature implementation (PRs #1-5) must be documented and will accumulate for testing in the final diagnostic tool PR.

- **PR #1-5**: Document any new bugs found in PR description
- **PR #6**: Diagnostic tool will test for:
  - All 12 original bugs from BUG_ANALYSIS_REPORT.md
  - Any new bugs discovered in feature PRs #1-5
  - Verification that all bugs remain fixed
  - Detection of any regressions

This ensures comprehensive testing of all known issues at the end of the implementation process.

### Commit Message Format:
```
[Feature Name] Brief description

- Implement core functionality
- Fix Bug #X: Description
- Add unit tests
- Update documentation

Resolves #issue_number
References: BUG_ANALYSIS_REPORT.md
```

---

## Timeline Estimate

**Per Feature PR:**
- Implementation: 3-5 days
- Testing: 1-2 days
- Code review & fixes: 1-2 days
- **Total per feature:** ~1 week

**Total Timeline:**
- PR #1 (Gemini API): 1 week
- PR #2 (File Tinder): 1 week
- PR #3 (Whitelist Editor): 1 week
- PR #4 (Cache Manager): 1 week
- PR #5 (Content-Based): 1 week
- PR #6 (Diagnostic Tool): 1 week
- **Total:** ~6 weeks with sequential development

**Parallel Development:**
- PRs #1, #2, #3, #4 can be developed in parallel (independent)
- PR #5 depends on PR #1
- PR #6 depends on all others
- **Total:** ~3 weeks with parallel development

---

## Success Criteria

### Per Feature:
- ✅ All identified bugs fixed
- ✅ Unit tests pass with 80%+ coverage
- ✅ No AddressSanitizer warnings
- ✅ No ThreadSanitizer warnings
- ✅ Documentation updated
- ✅ Code review approved

### Final (PR #6):
- ✅ Diagnostic tool tests all features
- ✅ No regressions detected
- ✅ All 12 bugs remain fixed
- ✅ Performance benchmarks met
- ✅ CI/CD pipeline green

---

## Risk Mitigation

**Risk:** Features might interfere with each other  
**Mitigation:** Keep features modular, minimize shared state

**Risk:** Bug fixes might introduce new bugs  
**Mitigation:** Comprehensive testing, sanitizers, code review

**Risk:** Content-based sorting depends on Gemini API  
**Mitigation:** Implement PR #1 first, PR #5 can reuse tested API

**Risk:** Diagnostic tool might miss issues  
**Mitigation:** Test against known bugs, verify with manual testing

---

## Next Steps

1. **Review this roadmap** - Ensure approach is correct
2. **Start PR #1** - Enhanced Gemini API
3. **Implement incrementally** - One feature at a time
4. **Test thoroughly** - Run sanitizers, write tests
5. **Document everything** - Keep README updated
6. **Final validation** - PR #6 diagnostic tool

---

**Current Status:** ✅ Bug Analysis Complete - Ready to Begin Feature Implementation

**First PR:** feature/enhanced-gemini-api

**Reference:** BUG_ANALYSIS_REPORT.md for detailed bug information
