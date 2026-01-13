# Quick Feature Testing Checklist

A condensed reference for rapid feature validation. For detailed testing procedures, see [MANUAL_TESTING_GUIDE.md](MANUAL_TESTING_GUIDE.md).

## Pre-Flight Checks

**Run before starting any manual tests:**

```bash
# 1. Run diagnostic tool
python3 diagnostic_tool.py --verbose --output diagnostic_report.json

# 2. Run feature validator
./feature_validator.sh --verbose

# 3. Check results
#    - All critical checks should pass (✓)
#    - Warnings (⚠) are acceptable for optional features
#    - Failures (✗) must be addressed
```

---

## Core Feature Quick Tests (15 minutes)

### ✓ Application Launch
```
[ ] Windows: StartAiFileSorter.exe opens
[ ] Linux: ./run_aifilesorter.sh or installed launcher works
[ ] macOS: AI File Sorter.app opens
[ ] Main window displays correctly
[ ] No startup errors
```

### ✓ Basic Categorization
```
[ ] Select test directory (10-20 files)
[ ] Choose local LLM or API
[ ] Click "Analyze"
[ ] Files categorized successfully
[ ] Results appear in review dialog
[ ] Categories make sense
```

### ✓ File Operations
```
[ ] Enable "Dry run" checkbox
[ ] Click "Confirm & Sort!"
[ ] Preview shows From → To moves
[ ] Disable dry run, sort again
[ ] Files moved to categories
[ ] Edit → "Undo last run" works
[ ] Files restored to original locations
```

**Time: ~15 minutes | Priority: CRITICAL**

---

## Feature Matrix Quick Reference

| Feature | Location | Test Time | Priority |
|---------|----------|-----------|----------|
| **File Scanning** | Main window | 2 min | Critical |
| **Local LLM** | Settings → Select LLM | 5 min | Critical |
| **OpenAI API** | Settings → Select LLM | 3 min | High |
| **Gemini API** | Settings → Select LLM | 3 min | High |
| **Dry Run** | Results dialog checkbox | 2 min | Critical |
| **Undo** | Edit → Undo last run | 2 min | Critical |
| **Cache Manager** | Settings → Manage Cache | 3 min | Medium |
| **File Tinder** | Tools → File Tinder | 5 min | Medium |
| **User Profile** | Help → View User Profile | 3 min | Medium |
| **Whitelists** | Settings → Manage category whitelists | 5 min | Medium |
| **Categorization Modes** | Main window radio buttons | 3 min | High |
| **Subcategories** | Main window checkbox | 2 min | High |
| **Learning Control** | Gear icon next to folder | 3 min | Medium |
| **Multilingual UI** | Settings → Language | 2 min | Low |
| **Multilingual Categories** | Settings → Category Language | 2 min | Low |

---

## Critical Path Testing (30 minutes)

**Essential flow that must work:**

1. **Setup (5 min)**
   - [ ] Launch application
   - [ ] Select LLM (local or API)
   - [ ] Configure basic settings

2. **Categorization (10 min)**
   - [ ] Select directory
   - [ ] Enable subcategories
   - [ ] Click "Analyze"
   - [ ] Wait for completion
   - [ ] Review results

3. **Preview (5 min)**
   - [ ] Enable dry run
   - [ ] Preview moves
   - [ ] Verify accuracy
   - [ ] Close preview

4. **Execute (5 min)**
   - [ ] Disable dry run
   - [ ] Confirm & Sort
   - [ ] Verify files moved
   - [ ] Check folder structure

5. **Undo (5 min)**
   - [ ] Close application
   - [ ] Reopen application
   - [ ] Edit → Undo last run
   - [ ] Verify files restored

**If all steps pass: Core functionality is working ✓**

---

## Platform-Specific Quick Tests

### Windows (10 minutes)
```
[ ] StartAiFileSorter.exe launches (not aifilesorter.exe directly)
[ ] Qt DLLs load correctly
[ ] CUDA backend available (if NVIDIA GPU)
[ ] Vulkan backend available (if compatible GPU)
[ ] CPU backend works as fallback
[ ] API keys stored in %APPDATA%\aifilesorter\config.ini
[ ] Database at %APPDATA%\aifilesorter\aifilesorter.db
```

### Linux (10 minutes)
```
[ ] Application launches from menu or command
[ ] Qt6 libraries found (system or bundled)
[ ] CUDA backend available (if NVIDIA GPU with drivers)
[ ] Vulkan backend available (if Mesa/drivers)
[ ] CPU backend works (OpenBLAS)
[ ] Config at ~/.config/aifilesorter/config.ini
[ ] Database at ~/.local/share/aifilesorter/aifilesorter.db
```

### macOS (10 minutes)
```
[ ] .app bundle opens without errors
[ ] Metal backend available (Apple Silicon)
[ ] CPU backend works
[ ] Config at ~/Library/Preferences/aifilesorter/config.ini
[ ] Database at ~/Library/Application Support/aifilesorter/aifilesorter.db
[ ] Gatekeeper allows execution (not quarantined)
```

---

## Error Scenario Quick Tests (10 minutes)

**Test error handling:**

```
[ ] Invalid directory path → Clear error message
[ ] Permission denied → Specific error, graceful handling
[ ] No internet (API mode) → Retry dialog appears
[ ] Invalid API key → Authentication error shown
[ ] Out of disk space → Warning before operation
[ ] Corrupted database → Recreated automatically
```

**Each error should:**
- Show error code (e.g., "Error 1234")
- Display user-friendly message
- Provide resolution steps
- Include "Copy Error Details" button

---

## Performance Quick Checks (5 minutes)

### Small Dataset (10 files)
```
Expected: < 30 seconds (local), < 10 seconds (API)
Actual: ___________
[ ] Pass / [ ] Fail
```

### Medium Dataset (50 files)
```
Expected: < 3 minutes (local), < 1 minute (API)
Actual: ___________
[ ] Pass / [ ] Fail
```

### Cache Performance
```
[ ] First run: Normal speed
[ ] Second run (cached): Significantly faster (90%+ cache hits)
[ ] Cache hit rate: __________%
```

---

## Advanced Feature Sampling (15 minutes)

**Test a subset of advanced features:**

### User Profiling (5 min)
```
[ ] Enable "Learn from my organization patterns"
[ ] Categorize 2-3 diverse folders
[ ] Help → View User Profile
[ ] Check characteristics tab has entries
[ ] Verify confidence levels > 0
```

### File Tinder (5 min)
```
[ ] Tools → File Tinder
[ ] Select directory
[ ] Arrow keys work (→ keep, ← delete, ↓ skip, ↑ back)
[ ] Preview shows correctly
[ ] Progress bar updates
[ ] Can save session
```

### Whitelist (5 min)
```
[ ] Settings → Manage category whitelists
[ ] Create new whitelist with 5 categories
[ ] Enable whitelist on main window
[ ] Categorize files
[ ] Verify only whitelist categories used
```

---

## Data Integrity Checks (5 minutes)

**After all testing:**

```bash
# 1. Check test files
ls test_files/  # Should still exist

# 2. Check database
python3 -c "
import sqlite3
conn = sqlite3.connect('~/.local/share/aifilesorter/aifilesorter.db')
cursor = conn.cursor()
cursor.execute('PRAGMA integrity_check')
print(cursor.fetchone()[0])  # Should print 'ok'
conn.close()
"

# 3. Check config
cat ~/.config/aifilesorter/config.ini  # Should be valid INI format

# 4. Check logs
ls -lh ~/.local/share/aifilesorter/logs/  # Check for error logs
```

**Expected:**
- [ ] All test files intact
- [ ] Database integrity OK
- [ ] Config file valid
- [ ] No unexpected errors in logs

---

## Smoke Test (5 minutes)

**Absolute minimum test before shipping:**

1. Launch app → No crash ✓
2. Select directory → Path appears ✓
3. Click Analyze → Progress shown ✓
4. Files categorized → Results shown ✓
5. Click Confirm & Sort → Files moved ✓
6. Close app → No crash ✓
7. Reopen app → Loads correctly ✓
8. Edit → Undo → Files restored ✓

**If all pass: Application is minimally functional**

---

## Quick Test Report Template

```
Date: ___________
Tester: ___________
Platform: ___________
Build: ___________

Critical Path: [ ] PASS / [ ] FAIL
Core Features: [ ] PASS / [ ] FAIL
Error Handling: [ ] PASS / [ ] FAIL
Performance: [ ] PASS / [ ] FAIL
Data Integrity: [ ] PASS / [ ] FAIL

Issues Found:
1. 
2. 
3. 

Overall: [ ] Ship It / [ ] Fix Issues / [ ] Major Problems
```

---

## Common Issues & Quick Fixes

| Issue | Quick Fix |
|-------|-----------|
| App won't start | Run `diagnostic_tool.py`, check logs |
| LLM fails | Verify backend, check RAM/GPU |
| API fails | Check internet, verify key |
| Slow performance | Clear cache, check system resources |
| Files not moving | Check permissions, verify paths |
| Database errors | Delete and rebuild database |
| UI glitches | Update graphics drivers, check Qt version |

---

## Command Reference

```bash
# Run full diagnostic
python3 diagnostic_tool.py --verbose --output report.json

# Run feature validator
./feature_validator.sh --verbose

# Check database integrity
sqlite3 ~/.local/share/aifilesorter/aifilesorter.db "PRAGMA integrity_check"

# Clear cache (manual)
sqlite3 ~/.local/share/aifilesorter/aifilesorter.db "DELETE FROM categorization_cache"

# View logs
tail -f ~/.local/share/aifilesorter/logs/errors.log

# Force backend (Linux)
./run_aifilesorter.sh --cuda=on --vulkan=off   # CUDA only
./run_aifilesorter.sh --cuda=off --vulkan=on   # Vulkan only
./run_aifilesorter.sh --cuda=off --vulkan=off  # CPU only
```

---

## When to Run Each Test Suite

| Scenario | Test Suite | Duration |
|----------|------------|----------|
| **Daily development** | Smoke test | 5 min |
| **Before commit** | Critical path + Feature validator | 30 min |
| **Before PR** | Core features + Advanced sampling | 1 hour |
| **Before release** | Full manual testing guide | 4-6 hours |
| **After major change** | Full diagnostic + Affected features | 1-2 hours |
| **Bug investigation** | Diagnostic tool + Error scenarios | 30 min |

---

## Automation Integration

**CI/CD Pipeline:**
```yaml
pre-commit:
  - feature_validator.sh
  - unit tests
  
pre-merge:
  - feature_validator.sh --verbose
  - integration tests
  - smoke test
  
pre-release:
  - diagnostic_tool.py
  - feature_validator.sh
  - smoke test
  - performance benchmarks
```

---

## Resources

- **Full Manual Testing Guide:** [MANUAL_TESTING_GUIDE.md](MANUAL_TESTING_GUIDE.md)
- **Diagnostic Tool:** `diagnostic_tool.py`
- **Feature Validator:** `feature_validator.sh`
- **Feature Documentation:** [FEATURE_ANALYSIS.md](FEATURE_ANALYSIS.md)
- **Error Reference:** [ERROR_CODES.md](ERROR_CODES.md)
- **Bug Reporting:** [QUICK_BUG_REPORT.md](QUICK_BUG_REPORT.md)

---

**Last Updated:** 2026-01-13  
**Document Version:** 1.0
