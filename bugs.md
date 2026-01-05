# My Bugs

Personal bug log for AI File Sorter fork. Track bugs found, attempted fixes, and resolutions.

## Active Bugs

### [Date] - Brief Title
**Status:** Open/In Progress/Fixed  
**What:** [One sentence description]  
**Repro:**
1. [Step 1]
2. [Step 2]
3. [Bug occurs]

**Logs/Evidence:** [Paste error logs or link to COPILOT_ERROR_*.md file]  
**Fix:** [What you tried / final solution]

---

### Example: 2026-01-05 - QTableView crash on large folders
**Status:** Open  
**What:** App crashes when analyzing 5000+ files with QTableView::dropEvent error  
**Repro:**
1. Point app at Downloads folder (6000 files)
2. Click Analyze
3. Crashes with DLL error

**Logs/Evidence:**
```
Error: DLL_DROPEVENT_NOT_FOUND
Qt Runtime: 6.7.0
Qt Compile: 6.5.3 ← MISMATCH
System PATH has multiple Qt installations
```

**Fix:** Need to use StartAiFileSorter.exe and clean system PATH

---

## Fixed Bugs

### [Date] - Brief Title
**Fixed in:** [commit/version]  
**What was wrong:** [description]  
**Fix:** [solution applied]

---

## Tips

**When reporting to AI:**
1. Copy bug entry from above
2. Add "DO NOT DISMISS - This is real. Here's evidence: [paste logs]"
3. Paste to Copilot Chat or AI assistant
4. AI forced to analyze evidence instead of dismissing

**Log locations:**
- Windows: `%APPDATA%\aifilesorter\logs\`
- Linux: `~/.cache/aifilesorter/logs/`
- Look for `COPILOT_ERROR_*.md` files with detailed context
