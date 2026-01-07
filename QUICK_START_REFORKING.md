# Quick Start: Reforking Guide
## TL;DR - Get Started in 10 Minutes

**Last Updated:** January 6, 2026  
**For detailed guide, see:** [REFORKING_STRATEGY.md](REFORKING_STRATEGY.md)

---

## The Answer: What Should You Do?

### ✅ **BEST OPTION: Fresh Fork + Feature-by-Feature Migration**

**Time:** 6-8 weeks with AI assistance  
**Difficulty:** Medium (with this guide: Easy)  
**Result:** Professional, maintainable codebase

---

## Why This is The Right Choice

✅ **You have everything needed:**
- 7 implemented custom features (~6,000 LOC)
- 32 comprehensive documentation files
- Working reference code
- Clear database schema
- Detailed implementation plans

✅ **Long-term benefits:**
- Clean git history
- Easy upstream updates forever
- No technical debt
- Professional structure

✅ **It's actually feasible:**
- Your docs make extraction easy
- AI can help with complex parts
- Features are well-separated
- Can implement in phases

---

## What You're Moving Over

### TIER 1: Core Features (Must Have) - 4 weeks
1. **User Profiling System** (2,500 LOC) - 1.5-2 weeks
2. **Gemini API Integration** (600 LOC) - 3-5 days  
3. **API Usage Tracking** (500 LOC) - 2-3 days

### TIER 2: Enhanced Features (Should Have) - 1 week
4. **File Tinder Tool** (600 LOC) - 2-3 days
5. **Cache Manager** (300 LOC) - 1-2 days
6. **Enhanced Undo** (400 LOC) - 2 days
7. **Dry Run Preview** (350 LOC) - 1-2 days

### TIER 3: Optional - 3 days
8. **Error Reporting** (800 LOC) - 2-3 days

### Documentation - 2 days
- Copy 32 markdown files
- Update README

**Total Custom Code:** ~6,000 lines  
**Total Time:** 6-8 weeks

---

## Start NOW (Next 30 Minutes)

### Step 1: Backup Current Fork (5 min)

```bash
cd /path/to/ai-file-sorter-iconic
git bundle create ../ai-file-sorter-backup.bundle --all
```

### Step 2: Extract Features (10 min)

```bash
# Create extraction directory
mkdir ../feature-extraction
cd ../feature-extraction

# Copy implemented features
mkdir -p features/{tier1,tier2,tier3}

# Tier 1 - Core
mkdir -p features/tier1/{user-profiling,gemini-api,api-tracking}
cp ../ai-file-sorter-iconic/app/{include,lib}/UserProfile* features/tier1/user-profiling/
cp ../ai-file-sorter-iconic/app/{include,lib}/FolderLearning* features/tier1/user-profiling/
cp ../ai-file-sorter-iconic/app/{include,lib}/GeminiClient.* features/tier1/gemini-api/
cp ../ai-file-sorter-iconic/app/{include,lib}/APIUsage* features/tier1/api-tracking/
cp ../ai-file-sorter-iconic/app/{include,lib}/UsageStats* features/tier1/api-tracking/

# Tier 2 - Enhanced
mkdir -p features/tier2/{file-tinder,cache-manager,undo,dry-run}
cp ../ai-file-sorter-iconic/app/{include,lib}/FileTinder* features/tier2/file-tinder/
cp ../ai-file-sorter-iconic/app/{include,lib}/CacheManager* features/tier2/cache-manager/
cp ../ai-file-sorter-iconic/app/{include,lib}/UndoManager.* features/tier2/undo/
cp ../ai-file-sorter-iconic/app/{include,lib}/DryRunPreview* features/tier2/dry-run/

# Tier 3 - Optional
mkdir -p features/tier3/error-system
cp ../ai-file-sorter-iconic/app/{include,lib}/ErrorReporter.* features/tier3/error-system/
cp ../ai-file-sorter-iconic/app/include/ErrorCode.hpp features/tier3/error-system/

# Copy all documentation
cp ../ai-file-sorter-iconic/*.md ./
```

### Step 3: Fork Original Repo (5 min)

1. Go to: https://github.com/hyperfield/ai-file-sorter
2. Click "Fork"
3. Name: `ai-file-sorter` or `ai-file-sorter-v2`
4. Create fork

### Step 4: Clone & Verify (10 min)

```bash
cd ..
git clone https://github.com/YOUR_USERNAME/ai-file-sorter.git ai-file-sorter-new
cd ai-file-sorter-new

# Add upstream
git remote add upstream https://github.com/hyperfield/ai-file-sorter.git

# Verify it builds
cd app
make -j4  # Linux/macOS
# OR: .\build_windows.ps1  # Windows

# Test it runs
./bin/aifilesorter
```

**✅ If it builds and runs, you're ready to start implementing!**

---

## Week-by-Week Plan

### Week 1: Foundation
- Day 1: Setup (done above)
- Day 2-3: Database schema
- Day 4-5: Test & commit

### Week 2-3: Quick Wins
- Day 6-7: Cache Manager
- Day 8-9: Dry Run Preview
- Day 10-12: Enhanced Undo

### Week 3-5: Core Features
- Day 13-17: Gemini API
- Day 18-20: API Tracking
- Day 21-30: User Profiling

### Week 6: Utilities
- Day 31-33: File Tinder
- Day 34-36: Error Reporting (optional)

### Week 7-8: Polish
- Day 37-40: Documentation
- Day 41-49: Testing & release

---

## Your First Feature (Cache Manager - Tomorrow)

**Easiest feature to start with. Build confidence fast.**

```bash
cd ai-file-sorter-new
git checkout -b feature/cache-manager develop

# Copy files
cp ../feature-extraction/features/tier2/cache-manager/CacheManagerDialog.hpp app/include/
cp ../feature-extraction/features/tier2/cache-manager/CacheManagerDialog.cpp app/lib/

# Edit app/CMakeLists.txt or Makefile
# Add: app/lib/CacheManagerDialog.cpp

# Edit app/lib/MainApp.cpp
# Add include:
#include "CacheManagerDialog.hpp"

# Add menu action in create_menus():
QAction* cache_action = settings_menu->addAction(tr("Manage Cache..."));
connect(cache_action, &QAction::triggered, this, &MainApp::on_manage_cache);

# Add slot:
void MainApp::on_manage_cache() {
    CacheManagerDialog dialog(*db_manager_, this);
    dialog.exec();
}

# Build & test
cd app && make clean && make -j4
./bin/aifilesorter
# Click Settings → Manage Cache

# If it works:
git add .
git commit -m "Add cache management dialog"
git push origin feature/cache-manager
```

**🎉 First feature done! Repeat for others.**

---

## Key Commands Cheatsheet

```bash
# Feature branch workflow
git checkout -b feature/NAME develop
# ... work ...
git add .
git commit -m "Message"
git push origin feature/NAME

# Build & test
make clean && make -j4
./bin/aifilesorter

# Check database tables
sqlite3 ~/.local/share/aifilesorter/database.db ".tables"

# Find code references
grep -r "ClassName" app/
```

---

## When You Get Stuck

1. **Read:** [REFORKING_STRATEGY.md](REFORKING_STRATEGY.md) - Full detail
2. **Check:** Feature files in `feature-extraction/`
3. **Review:** Your 32 documentation files
4. **Ask AI:** Use prompts from REFORKING_STRATEGY.md
5. **Test:** Each feature individually before moving on

---

## Success Checklist

After 6-8 weeks, you should have:

- [ ] Clean fork of hyperfield/ai-file-sorter ✓
- [ ] All 7 custom features working ✓
- [ ] 16 database tables (8 used, 8 future-ready) ✓
- [ ] 32 documentation files ✓
- [ ] Professional git history ✓
- [ ] Can sync with upstream ✓
- [ ] Release v1.6.0-custom published ✓

---

## Bottom Line

**Don't overthink it. Just start:**

1. ✅ Backup (5 min) - Do now
2. ✅ Extract (10 min) - Do now
3. ✅ Fork & clone (10 min) - Do now
4. ✅ Start with Cache Manager (tomorrow)
5. ✅ Follow the week-by-week plan
6. ✅ Use AI for complex parts
7. ✅ Test thoroughly
8. ✅ Release in 6-8 weeks

**You've got this! 🚀**

---

## Questions?

- **Detailed guide:** [REFORKING_STRATEGY.md](REFORKING_STRATEGY.md)
- **Implementation plan:** [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
- **Feature details:** [FEATURE_ANALYSIS.md](FEATURE_ANALYSIS.md)
- **Future ideas:** [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)

---

**Ready? Start with Step 1 above. See you at v1.6.0-custom! 🎯**
