# Diagnostic Tools Comparison

AI File Sorter now has **three diagnostic tools** for different use cases. This guide helps you choose the right tool.

## Quick Comparison Table

| Feature | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|---------|-------------------|------------------------|----------------------|
| **Language** | Python | Python | Bash |
| **Lines of Code** | ~960 | ~1,800 | ~450 |
| **Checks Performed** | 26 | **51+** | 50+ |
| **Cross-Platform** | ✓ Yes | ✓ Yes | Linux/macOS only |
| **Windows Support** | ✓ Yes | ✓ Yes | ✗ No |
| **Dependencies** | Python stdlib | Python stdlib | Bash, grep, find |
| **Feature Testing** | Basic | **Comprehensive** | Source-code focused |
| **Report Formats** | JSON | **JSON, HTML, Markdown** | Terminal only |
| **Color Output** | ✓ Yes | ✓ Yes | ✓ Yes |
| **Recommendations** | ✗ No | **✓ Yes** | ✗ No |
| **Performance Tests** | ✗ No | **✓ Yes** | ✗ No |
| **API Testing** | ✗ No | **✓ Yes (optional)** | ✗ No |
| **Quick Mode** | ✗ No | **✓ Yes** | N/A |
| **Health Status** | Basic | **Advanced (4 levels)** | Pass/Fail |
| **Categorized Results** | Basic | **Advanced (10 categories)** | Basic |
| **Execution Time** | 1-5 sec | 2-15 sec | 1-3 sec |
| **Best For** | General checks | **Complete validation** | Code structure |

## Tool Descriptions

### 1. diagnostic_tool.py (Original)

**Purpose:** General system health check and feature validation

**When to use:**
- Quick system overview
- Checking if files exist
- Basic dependency validation
- Simple JSON report needed

**Strengths:**
- Simple and straightforward
- Fast execution
- Good for basic checks

**Limitations:**
- No performance testing
- No API connectivity tests
- Limited recommendations
- Basic reporting only

**Example:**
```bash
python3 diagnostic_tool.py --verbose --output report.json
```

---

### 2. thorough_diagnostic.py (New - v2.0) ⭐ **RECOMMENDED**

**Purpose:** Comprehensive testing of ALL features and system requirements

**When to use:**
- **Pre-release validation** - Ensure everything works
- **Bug investigation** - Detailed diagnostics for issues
- **Daily development** - Quick mode for rapid validation
- **User support** - Generate reports for troubleshooting
- **CI/CD integration** - Automated testing pipelines

**Strengths:**
- ✅ **Most comprehensive** - Tests everything
- ✅ **Actionable recommendations** - Tells you how to fix issues
- ✅ **Multiple report formats** - JSON, HTML, Markdown
- ✅ **Quick mode** - 2-5 seconds for rapid checks
- ✅ **Performance benchmarks** - Disk and database speed
- ✅ **API testing** - OpenAI and Gemini connectivity
- ✅ **Health status** - 4 levels (Excellent, Good, Needs Attention, Critical)
- ✅ **Cross-platform** - Windows, Linux, macOS

**Example:**
```bash
# Quick daily check
python3 thorough_diagnostic.py --quick

# Full diagnostic with HTML report
python3 thorough_diagnostic.py -v --html --markdown

# Pre-release validation
python3 thorough_diagnostic.py --verbose --test-apis --html
```

**Output Examples:**

Terminal:
```
╔════════════════════════════════════════════════════════════════════════════╗
║          AI FILE SORTER - THOROUGH DIAGNOSTIC TOOL v2.0                    ║
║              Comprehensive Feature & System Validation                     ║
╚════════════════════════════════════════════════════════════════════════════╝

================================================================================
FEATURE IMPLEMENTATION VALIDATION
================================================================================
  ✓ Feature: Core Categorization: Implemented (957 lines, 35.7 KB)
  ✓ Feature: File Scanner: Implemented (148 lines, 4.6 KB)
  ✓ Feature: Gemini Client: Implemented (824 lines, 30.3 KB)
  ✓ Feature: User Profile Manager: Implemented (657 lines, 23.9 KB)
  ✓ Feature: File Tinder: Implemented (540 lines, 18.0 KB)
  ...
  
Overall Health: EXCELLENT
```

---

### 3. feature_validator.sh (Bash-based)

**Purpose:** Fast validation of source code structure and feature implementations

**When to use:**
- Linux/macOS development
- Quick pre-commit checks
- Verify source code structure
- CI/CD on Unix systems

**Strengths:**
- Very fast (~1 second)
- No Python required
- Good for source code checks

**Limitations:**
- Unix-only (no Windows)
- No performance testing
- Basic reporting
- Less detailed than thorough_diagnostic

**Example:**
```bash
./feature_validator.sh --verbose
```

---

## Decision Matrix: Which Tool Should I Use?

### Scenario 1: Daily Development ✏️

**Use:** `thorough_diagnostic.py --quick`

**Why:** Fast (2-5 sec), comprehensive, catches issues early

```bash
python3 thorough_diagnostic.py --quick
```

---

### Scenario 2: Pre-Commit Validation ✅

**Use:** `thorough_diagnostic.py --quick` or `feature_validator.sh`

**Why:** Quick validation before committing changes

```bash
python3 thorough_diagnostic.py --quick
```

---

### Scenario 3: Pre-Release Testing 🚀

**Use:** `thorough_diagnostic.py --verbose --html --test-apis`

**Why:** Complete validation with detailed reports

```bash
python3 thorough_diagnostic.py -v --html --markdown --test-apis
```

---

### Scenario 4: Bug Investigation 🐛

**Use:** `thorough_diagnostic.py --verbose --html`

**Why:** Detailed diagnostics with actionable recommendations

```bash
python3 thorough_diagnostic.py --verbose --html --output bug_report.json
```

---

### Scenario 5: User Support 💬

**Use:** `thorough_diagnostic.py --html`

**Why:** Generate easy-to-read HTML report for troubleshooting

```bash
python3 thorough_diagnostic.py --html --output user_diagnostic.json
# Send user_diagnostic.html to user
```

---

### Scenario 6: CI/CD Pipeline 🔄

**Use:** `thorough_diagnostic.py --quick` or `feature_validator.sh`

**Why:** Fast automated validation in build pipelines

```bash
python3 thorough_diagnostic.py --quick --output ci_diagnostic.json
```

---

### Scenario 7: Performance Analysis ⚡

**Use:** `thorough_diagnostic.py` (without --quick)

**Why:** Includes disk I/O and database benchmarks

```bash
python3 thorough_diagnostic.py --verbose
```

---

### Scenario 8: API Troubleshooting 🌐

**Use:** `thorough_diagnostic.py --test-apis`

**Why:** Tests OpenAI and Gemini API connectivity

```bash
python3 thorough_diagnostic.py --test-apis --verbose
```

---

## Feature Coverage Comparison

### System Information
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| OS & Platform | ✓ | ✓ | ✓ |
| Python Version | ✓ | ✓ | ✗ |
| CPU Info | ✗ | ✓ | ✗ |
| Memory | ✓ Basic | ✓ Detailed | ✗ |
| Disk Space | ✓ | ✓ | ✗ |

### File Structure
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Executables | ✓ | ✓ | ✓ |
| Directories | ✓ | ✓ | ✓ |
| Source Files | ✗ | ✓ | ✓ |
| Permissions | ✗ | ✓ | ✗ |

### Dependencies
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Qt6 | ✓ | ✓ | ✗ |
| System Libraries | ✗ | ✓ | ✗ |
| Version Checks | ✗ | ✓ | ✗ |

### LLM Backends
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| CPU Backend | ✓ | ✓ | ✓ |
| CUDA Backend | ✓ | ✓ | ✓ |
| Vulkan Backend | ✓ | ✓ | ✓ |
| Local Models | ✓ | ✓ | ✗ |
| Model Details | ✗ | ✓ | ✗ |

### Database
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| File Exists | ✓ | ✓ | ✗ |
| Integrity Check | ✗ | ✓ | ✗ |
| Schema Validation | ✓ Basic | ✓ All 12 tables | ✗ |
| Statistics | ✓ Basic | ✓ Detailed | ✗ |
| Performance | ✗ | ✓ | ✗ |

### Features
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Feature Count | 7 features | **22 features** | 15+ features |
| Implementation | ✓ | ✓ | ✓ |
| Line Counts | ✗ | ✓ | ✗ |
| File Sizes | ✗ | ✓ | ✗ |

### Configuration
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Config File | ✓ | ✓ | ✗ |
| API Keys | ✓ Basic | ✓ Detailed | ✗ |
| Permissions | ✗ | ✓ | ✗ |

### Logs
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Log Files | ✓ | ✓ | ✗ |
| Error Logs | ✓ | ✓ | ✗ |
| Recent Errors | ✗ | ✓ | ✗ |

### Performance
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Disk I/O | ✗ | ✓ | ✗ |
| Database | ✗ | ✓ | ✗ |

### API
| Check | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|-------|-------------------|------------------------|----------------------|
| Connectivity | ✗ | ✓ | ✗ |
| OpenAI | ✗ | ✓ | ✗ |
| Gemini | ✗ | ✓ | ✗ |

---

## Report Format Comparison

### JSON Reports

**diagnostic_tool.py:**
- Basic structure
- Results list
- Summary stats

**thorough_diagnostic.py:**
- Advanced structure
- Categorized results
- Metadata
- Health status
- Recommendations
- Timestamps

### HTML Reports

**diagnostic_tool.py:** ✗ Not available

**thorough_diagnostic.py:** ✓ Full interactive HTML with:
- Styled interface
- Health badges
- Color-coded results
- Collapsible sections
- Responsive design

### Markdown Reports

**diagnostic_tool.py:** ✗ Not available

**thorough_diagnostic.py:** ✓ Clean markdown with:
- Summary tables
- Categorized sections
- Status indicators
- Recommendations

---

## Execution Time Comparison

| Tool | Quick Mode | Full Mode |
|------|-----------|-----------|
| diagnostic_tool.py | N/A | 1-5 seconds |
| **thorough_diagnostic.py** | **2-5 seconds** | **5-15 seconds** |
| feature_validator.sh | 1-3 seconds | N/A |

---

## Recommendation Summary

### ⭐ **Primary Tool (Recommended for Most Uses)**

**thorough_diagnostic.py** - Use this for:
- Daily development (`--quick`)
- Pre-release validation (`--verbose --html`)
- Bug investigation
- User support
- CI/CD integration

### **Secondary Tools (Specific Use Cases)**

**diagnostic_tool.py** - Use for:
- Legacy compatibility
- Simple JSON reports
- Basic checks only

**feature_validator.sh** - Use for:
- Linux/macOS quick checks
- Pre-commit hooks (Unix)
- Source code validation

---

## Migration Guide

### From diagnostic_tool.py to thorough_diagnostic.py

**Old command:**
```bash
python3 diagnostic_tool.py --verbose --output report.json
```

**New equivalent:**
```bash
python3 thorough_diagnostic.py --verbose --output report.json
```

**Better alternative:**
```bash
python3 thorough_diagnostic.py -v --html --markdown --output report.json
```

Gets you JSON + HTML + Markdown reports with the same effort!

---

## FAQ

**Q: Can I use all three tools?**

A: Yes! They complement each other:
- Use `thorough_diagnostic.py` for most cases
- Keep `diagnostic_tool.py` for backward compatibility
- Use `feature_validator.sh` for quick Unix checks

**Q: Which is the fastest?**

A: `feature_validator.sh` (~1 sec), but `thorough_diagnostic.py --quick` is nearly as fast (2-5 sec) and much more comprehensive.

**Q: Which should I use for bug reports?**

A: `thorough_diagnostic.py --html` - generates easy-to-read HTML reports.

**Q: Which for CI/CD?**

A: `thorough_diagnostic.py --quick` - fast, comprehensive, cross-platform.

**Q: Do they have different output?**

A: Yes! thorough_diagnostic.py provides:
- More checks (51 vs 26)
- Better recommendations
- Multiple report formats
- Health status levels
- Performance benchmarks

---

## Conclusion

**Use `thorough_diagnostic.py` as your primary diagnostic tool.** It's the most comprehensive, provides actionable recommendations, supports multiple report formats, and works on all platforms.

Keep the other tools for specific use cases or backward compatibility.

---

**Last Updated:** 2026-01-18  
**Version:** 1.0  
**Tools Compared:** diagnostic_tool.py (v1), thorough_diagnostic.py (v2), feature_validator.sh
