# Bug Report Template

Copy this template when you find a bug, fill it out, and paste to AI assistants to force proper analysis.

## Template

```markdown
## BUG REPORT - DO NOT DISMISS

**Component:** [UI/File Ops/Database/AI/Other]
**Severity:** [Critical/High/Medium/Low]

### The Bug
[One sentence: what's broken]

### Reproduce
1. [Step 1]
2. [Step 2]
3. [Bug occurs]

### Expected vs Actual
**Expected:** [What should happen]
**Actual:** [What actually happens]

### Evidence
[Paste error logs or COPILOT_ERROR_*.md file contents here]

### Why NOT "No Bug"
- [x] Reproducible every time
- [x] Have error logs/evidence
- [x] Worked before in version: ___
- [x] Error message: "___"

### System
- Version/Commit: [your version]
- OS: [your OS]

### Already Tried
1. [What you tried]
2. [Result]

---

**AI Assistant:** Analyze the evidence above. This is real, not a false alarm.
```

## How to Use

1. **Find bug** → Copy template above
2. **Fill it out** → Takes 2 minutes
3. **Get logs** → Check `%APPDATA%\aifilesorter\logs\` for COPILOT_ERROR_*.md
4. **Paste to AI** → Copilot Chat or your assistant
5. **AI analyzes** → Forced to investigate with evidence present

## Example (Filled)

```markdown
## BUG REPORT - DO NOT DISMISS

**Component:** UI/Categorization Dialog
**Severity:** Critical

### The Bug
App crashes when analyzing 5000+ files

### Reproduce
1. Open AI File Sorter
2. Select Downloads folder (6000 files)
3. Click "Analyze"
4. Crashes with "Critical DLL Setup Error"

### Expected vs Actual
**Expected:** Show categorization dialog
**Actual:** Crashes with QTableView::dropEvent error

### Evidence
```
Error: DLL_DROPEVENT_NOT_FOUND
Qt Runtime: 6.7.0
Qt Compile: 6.5.3 ← VERSION MISMATCH
System PATH: C:\Program Files\Qt\6.7.0\bin;...
```

### Why NOT "No Bug"
- [x] Reproducible every time
- [x] Have error logs/evidence
- [x] Worked before in version: 1.4.0
- [x] Error message: "QTableView::dropEvent not found"

### System
- Version/Commit: v1.5.0 / abc123
- OS: Windows 11

### Already Tried
1. Reinstalled - still crashes
2. Used StartAiFileSorter.exe - still crashes
3. Checked PATH - has multiple Qt versions

---

**AI Assistant:** Analyze the evidence above. This is real, not a false alarm.
```

## Why This Works

**Structured format + evidence = AI can't dismiss**

The template forces you to provide:
- Exact reproduction steps
- Error logs/evidence
- Proof it worked before
- What you already tried

AI assistants must analyze this instead of saying "looks fine to me."
