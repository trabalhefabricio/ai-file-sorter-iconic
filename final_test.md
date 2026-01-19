# AI File Sorter - Diagnostic Summary

**Generated:** 2026-01-19T00:43:39.850042  
**Duration:** 0.03 seconds  
**Platform:** Linux 6.11.0-1018-azure

## Overall Health: CRITICAL

### Summary Statistics

| Status | Count |
|--------|-------|
| ✓ Passed | 68 |
| ⚠ Warnings | 2 |
| ✗ Failed | 2 |
| ℹ Info | 9 |
| **Total** | **82** |

---

## System

- **ℹ Operating System:** Linux 6.11.0-1018-azure (#18~24.04.1-Ubuntu SMP Sat Jun 28 04:46:03 UTC 2025)
- **✓ Python Version:** 3.12.3
- **ℹ CPU Cores:** 4 cores available
- **✓ System Memory:** 15.6 GB total
- **✓ Disk Space:** 21.2 GB free (71.6 GB total, 70.4% used)

## File Structure

- **✗ Executable: Main Binary:** Not found
  - 💡 *Rebuild the application*
- **✗ Executable: Launch Script:** Not found
  - 💡 *Rebuild the application*
- **✓ Include Directory:** Found (59 files, 0.1 MB)
- **✓ Library Directory:** Found (56 files, 12.0 MB)
- **✓ Resources Directory:** Found (47 files, 4.1 MB)
- **✓ Scripts Directory:** Found (7 files, 0.0 MB)
- **✓ C++ Headers:** Found (45 files)
- **✓ Resource Files:** Found (1 files)
- **✓ CMake Files:** Found (1 files)

## Dependencies

- **⚠ Qt6 Framework:** Not found via pkg-config
- **✓ Library: libcurl:** Available
- **✓ Library: SQLite3:** Available

## LLM Backends

- **✓ Backend: CPU (OpenBLAS):** Available (0 libraries, 0.0 MB)
- **ℹ Backend: CUDA (NVIDIA GPU):** Not found (optional)
- **ℹ Backend: Vulkan (Cross-platform GPU):** Not found (optional)
- **⚠ Precompiled LLM Libraries:** Directory not found
  - 💡 *Run build_llama script for your platform*
- **ℹ Local LLM Models:** Models directory not created yet

## Database

- **ℹ Database File:** Not created yet (will be created on first run)

## Configuration

- **ℹ Main Configuration:** Not created yet (will be created on first run)
- **✓ Config Directory Permissions:** Read/Write access

## Feature: Categorization

- **✓ Implementation File:** Found (957 lines, 35.7 KB)
- **✓ Core Methods:** 5/5 critical methods found
- **✓ Timeout Configuration:** Timeout environment variables supported
- **✓ Label Validation:** Label validation logic present
- **✓ Header File:** Found

## Feature: File Scanner

- **✓ Implementation File:** Found (148 lines, 4.6 KB)
- **✓ File Filtering:** File filtering logic present
- **✓ Header File:** Found

## Feature: Database

- **✓ Implementation File:** Found (2783 lines, 102.5 KB)
- **✓ Database Schema:** 15/15 tables defined
- **✓ Database Indexes:** Index definitions present
- **✓ Taxonomy Methods:** 3/3 taxonomy methods found
- **✓ Header File:** Found

## Feature: LLM Clients

- **✓ Local LLM Client:** Implemented (1649 lines, 53.6 KB)
- **✓ Backend Support:** 4 backends: Metal, CUDA, Vulkan, CPU
- **✓ GPU Memory Management:** GPU memory management present
- **✓ Model Loading:** Model loading logic present
- **✓ Environment Configuration:** 5/5 config variables supported
- **✓ OpenAI Client:** OpenAI integration present
- **✓ Gemini Client:** Implemented (824 lines, 30.3 KB)
- **✓ Gemini API Integration:** Google Gemini API integration present

## Feature: User Profile

- **✓ Profile Manager:** Implemented (657 lines, 23.9 KB)
- **✓ Profile Methods:** 5/5 profile methods found
- **✓ Confidence Scoring:** Confidence calculation logic present
- **✓ Folder Inclusion Levels:** Folder learning granularity supported
- **✓ Profile Dialog:** Found

## Feature: Undo

- **✓ Implementation File:** Found (153 lines, 4.7 KB)
- **✓ Core Methods:** 3/3 undo methods found
- **✓ JSON Serialization:** JSON plan serialization present
- **✓ Plan Validation:** File integrity validation present
- **✓ Header File:** Found

## Feature: File Tinder

- **✓ Implementation File:** Found (540 lines, 18.0 KB)
- **✓ Swipe Actions:** 3/3 actions found
- **✓ State Persistence:** Decision tracking present

## Feature: Cache

- **✓ Implementation File:** Found (212 lines, 7.4 KB)
- **✓ Cache Operations:** Cache clearing functionality present
- **✓ Cache Statistics:** Cache statistics tracking present

## Feature: Whitelist

- **✓ Implementation File:** Found (240 lines, 7.9 KB)
- **✓ Whitelist Operations:** 3/3 operations found

## Feature: Consistency

- **✓ Implementation File:** Found (637 lines, 20.6 KB)

## Feature: API Usage

- **✓ Usage Tracker:** Implemented (124 lines, 4.3 KB)
- **✓ Tracking Metrics:** 3/3 metrics tracked
- **✓ Usage Dialog:** Found

## Feature: Translation

- **✓ Translation Manager:** Implemented (709 lines, 49.1 KB)
- **✓ Language Switching:** Dynamic language switching supported
- **✓ Translation Files:** 5 languages: de, es, it, fr, tr

## Feature: LLM Selection

- **✓ Selection Dialog:** Implemented (792 lines, 27.3 KB)
- **✓ Provider Selection:** 3/3 providers: openai, gemini, local
- **✓ Custom LLM Dialog:** Found
- **✓ Model Downloader:** Found

## Feature: UI

- **✓ Categorization Dialog:** Found (1224 lines, 39.2 KB)
- **✓ Progress Dialog:** Found (115 lines, 2.8 KB)
- **✓ Dry Run Preview:** Found (59 lines, 2.3 KB)
- **✓ Folder Learning Dialog:** Found (96 lines, 3.2 KB)

## Logs

- **ℹ Log Directory:** Not created yet (created on first run)

## Performance

- **⊘ Disk I/O Performance:** Skipped in quick mode
- **ℹ Available Storage:** 21.2 GB free

---
*Generated by AI File Sorter Thorough Diagnostic Tool v2.1*
