# Thorough Diagnostic Tool - Complete Documentation

## Overview

The **Thorough Diagnostic Tool** (`thorough_diagnostic.py`) is a comprehensive, standalone diagnostic utility designed to thoroughly test **every feature** of AI File Sorter and validate all system requirements. It is completely separate from the main application and can be run independently for in-depth system validation.

## Key Features

✅ **Comprehensive Testing** - Tests ALL application features and components
✅ **Detailed Reports** - Generates JSON, HTML, and Markdown reports  
✅ **Actionable Recommendations** - Provides specific guidance for fixing issues  
✅ **Cross-Platform** - Works on Linux, macOS, and Windows  
✅ **No Dependencies** - Uses only Python standard library  
✅ **Fast & Flexible** - Quick mode for rapid checks, full mode for thorough validation  
✅ **Color-Coded Output** - Easy-to-read terminal output with status indicators  
✅ **Categorized Results** - Organized by System, Dependencies, Features, Database, etc.

## Installation

No installation required! The tool uses only Python standard library.

**Requirements:**
- Python 3.7 or higher
- No external packages needed

## Usage

### Basic Usage

```bash
# Simple diagnostic
python3 thorough_diagnostic.py

# Verbose output with detailed information
python3 thorough_diagnostic.py --verbose

# Quick mode (skip slow tests)
python3 thorough_diagnostic.py --quick
```

### Generate Reports

```bash
# Generate JSON report
python3 thorough_diagnostic.py --output my_report.json

# Generate HTML report (interactive, styled)
python3 thorough_diagnostic.py --html

# Generate Markdown summary
python3 thorough_diagnostic.py --markdown

# Generate all report types
python3 thorough_diagnostic.py --verbose --html --markdown
```

### Advanced Options

```bash
# Test API connectivity (requires internet & API keys)
python3 thorough_diagnostic.py --test-apis

# Full verbose diagnostic with all reports
python3 thorough_diagnostic.py -v --html --markdown --output full_diagnostic.json

# Quick check before committing code
python3 thorough_diagnostic.py --quick
```

## Command-Line Options

| Option | Short | Description |
|--------|-------|-------------|
| `--verbose` | `-v` | Enable verbose output with detailed information |
| `--output FILE` | `-o` | Save diagnostic report to specified JSON file |
| `--html` | | Generate interactive HTML report |
| `--markdown` | | Generate Markdown summary report |
| `--test-apis` | | Test API connectivity (OpenAI, Gemini) - requires internet |
| `--quick` | | Quick mode - skip slow tests for rapid validation |
| `--help` | `-h` | Show help message and exit |

## What It Tests

### 1. System Information ✓
- **Operating System** - Platform, version, architecture
- **Python Version** - Checks for Python 3.7+
- **CPU** - Core count and architecture
- **Memory** - Total RAM (recommends 4GB+ for LLM)
- **Disk Space** - Available storage (recommends 10GB+)

### 2. File Structure & Executables ✓
- **Main Executables** - Launcher and application binary
- **Launch Scripts** - Platform-specific startup scripts
- **Directory Structure** - Include, lib, resources, scripts
- **Source Files** - C++ headers, CMake files, resource files
- **File Permissions** - Execute permissions on binaries

### 3. Dependencies & Libraries ✓
- **Qt6 Framework** - Core, Gui, Widgets, Network, SQL modules
- **System Libraries** - libcurl, SQLite3, OpenSSL
- **Build Tools** - CMake, compilers (if applicable)
- **Platform-Specific** - macOS Metal, Windows MSVC, Linux packages

### 4. LLM Inference Backends ✓
- **CPU Backend** - OpenBLAS support
- **CUDA Backend** - NVIDIA GPU support (optional)
- **Vulkan Backend** - Cross-platform GPU support (optional)
- **Metal Backend** - macOS GPU support (optional)
- **Precompiled Libraries** - llama.cpp variants
- **Local Models** - Downloaded GGUF models

### 5. Database & Data Storage ✓
- **Database File** - Location and size
- **Database Integrity** - SQLite integrity check
- **Schema Validation** - All required tables present
- **Table Statistics** - Row counts per table
- **Tables Tested:**
  - categorization_cache
  - taxonomy
  - confidence_scores
  - content_analysis_cache
  - api_usage_tracking
  - user_profiles
  - user_corrections
  - categorization_sessions
  - undo_history
  - file_tinder_state
  - whitelists
  - folder_learning_config

### 6. Configuration Files ✓
- **Main Config** - config.ini location and contents
- **API Keys** - Presence check (without revealing keys)
- **Settings** - Language, theme, LLM model preferences
- **Directory Permissions** - Read/write access validation

### 7. Feature Implementation ✓
Tests source code presence for ALL features:

#### Core Features
- ✓ Core Categorization Service
- ✓ File Scanner
- ✓ Database Manager
- ✓ LLM Client Interface
- ✓ Local LLM Client
- ✓ OpenAI Client
- ✓ Gemini Client
- ✓ Categorization Dialog

#### User Profiling Features
- ✓ User Profile Manager
- ✓ User Profile Dialog
- ✓ Folder Learning Dialog

#### File Management Features
- ✓ File Tinder (Swipe-style cleanup)
- ✓ Cache Manager
- ✓ Undo Manager
- ✓ Dry Run Preview Dialog

#### Configuration Features
- ✓ Whitelist Manager
- ✓ Custom LLM Dialog
- ✓ LLM Selection Dialog
- ✓ API Usage Statistics

#### Additional Features
- ✓ Translation Manager
- ✓ Consistency Service
- ✓ Categorization Progress Dialog

### 8. Internationalization ✓
- **Translation Files** - Available languages
- **Supported Languages:** German (de), Spanish (es), Italian (it), French (fr), Turkish (tr)

### 9. Log Files & Error Reporting ✓
- **Log Directory** - Location and contents
- **Error Logs** - Recent errors and warnings
- **Copilot Reports** - User-friendly error reports
- **Log Analysis** - Recent error previews

### 10. Performance Benchmarks ✓
- **Disk I/O** - Read/write speed tests
- **Database Performance** - Query speed benchmarks
- **Memory Usage** - Available system memory

### 11. API Connectivity (Optional) ✓
- **Internet Connection** - General connectivity
- **OpenAI Endpoint** - api.openai.com reachability
- **Gemini Endpoint** - Google AI API reachability

## Output & Reports

### Terminal Output

Color-coded status indicators:
- ✓ **Green** = OK (passed)
- ⚠ **Yellow** = WARNING (needs attention)
- ✗ **Red** = FAIL (critical issue)
- ℹ **Blue** = INFO (informational)
- ⊘ **Gray** = SKIP (skipped in quick mode)

Example output:
```
╔════════════════════════════════════════════════════════════════════════════╗
║          AI FILE SORTER - THOROUGH DIAGNOSTIC TOOL v2.0                    ║
║              Comprehensive Feature & System Validation                     ║
╚════════════════════════════════════════════════════════════════════════════╝

================================================================================
SYSTEM INFORMATION
================================================================================
  ℹ Operating System: Linux 6.11.0 (Ubuntu 24.04)
  ✓ Python Version: 3.12.3
  ℹ CPU Cores: 4 cores available
  ✓ System Memory: 15.6 GB total
  ✓ Disk Space: 21.2 GB free (71.6 GB total, 70.4% used)
```

### JSON Report

Structured data for programmatic analysis:

```json
{
  "diagnostic_metadata": {
    "tool_version": "2.0",
    "timestamp": "2026-01-18T22:30:00.000Z",
    "duration_seconds": 3.45,
    "quick_mode": false
  },
  "system_info": {
    "platform": "Linux",
    "release": "6.11.0",
    "machine": "x86_64",
    "python_version": "3.12.3"
  },
  "summary": {
    "total_checks": 85,
    "by_status": {
      "OK": 65,
      "WARNING": 12,
      "FAIL": 2,
      "INFO": 6
    },
    "overall_health": "GOOD"
  },
  "results_by_category": { ... },
  "all_results": [ ... ]
}
```

### HTML Report

Interactive, styled HTML report with:
- Color-coded health badges
- Collapsible categories
- Detailed information panels
- Recommendations highlighted
- Professional styling

### Markdown Summary

Clean markdown summary perfect for:
- Documentation
- GitHub issues
- Email reports
- Quick reference

## Health Status Levels

The tool assigns an overall health status:

| Status | Criteria | Meaning |
|--------|----------|---------|
| **EXCELLENT** | 0 failures, 0 warnings | Everything perfect |
| **GOOD** | 0 failures, 1-5 warnings | Minor issues only |
| **NEEDS ATTENTION** | 0 failures, 6+ warnings | Multiple warnings to address |
| **CRITICAL** | 1+ failures | Critical issues require immediate attention |

## Recommendations

The tool provides **actionable recommendations** for every warning or failure:

Example:
```
⚠ Executable: Main Binary: Not found
  💡 Recommendation: Rebuild the application

✗ Database Integrity: Integrity check failed
  💡 Recommendation: Database may be corrupted. Consider backup and repair.

⚠ System Memory: 2.5 GB total
  💡 Recommendation: At least 4GB RAM recommended for LLM inference
```

## Use Cases

### Daily Development

```bash
# Quick sanity check
python3 thorough_diagnostic.py --quick
```

### Pre-Commit Validation

```bash
# Verify everything before committing
python3 thorough_diagnostic.py --verbose
```

### Bug Investigation

```bash
# Generate comprehensive report for issue
python3 thorough_diagnostic.py -v --html --markdown --output bug_report.json
```

### Pre-Release Testing

```bash
# Full diagnostic with all reports
python3 thorough_diagnostic.py --verbose --html --markdown --test-apis
```

### CI/CD Integration

```bash
# Automated testing in pipeline
python3 thorough_diagnostic.py --output ci_diagnostic.json
```

### User Support

```bash
# Generate report for troubleshooting
python3 thorough_diagnostic.py --html --output user_diagnostic.json
# Then send user_diagnostic.html to support
```

## Integration with Development Workflow

### GitHub Actions Example

```yaml
name: Thorough Diagnostic

on: [push, pull_request]

jobs:
  diagnose:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Run thorough diagnostic
        run: |
          python3 thorough_diagnostic.py --quick --output diagnostic.json
      
      - name: Upload diagnostic report
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: diagnostic-report
          path: |
            diagnostic.json
            diagnostic.html
            diagnostic.md
```

### Pre-commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

echo "Running thorough diagnostic..."
python3 thorough_diagnostic.py --quick

if [ $? -ne 0 ]; then
    echo "Diagnostic failed! Fix issues before committing."
    exit 1
fi
```

## Comparison with Other Diagnostic Tools

| Feature | diagnostic_tool.py | thorough_diagnostic.py | feature_validator.sh |
|---------|-------------------|------------------------|----------------------|
| Python-based | ✓ | ✓ | ✗ (Bash) |
| Cross-platform | ✓ | ✓ | Linux/macOS only |
| Feature testing | Basic | **Comprehensive** | Basic |
| Report formats | JSON | **JSON, HTML, Markdown** | Terminal only |
| Recommendations | ✗ | **✓** | ✗ |
| Performance tests | ✗ | **✓** | ✗ |
| API testing | ✗ | **✓ (optional)** | ✗ |
| Quick mode | ✗ | **✓** | N/A |
| Categorized results | Basic | **Advanced** | Basic |
| Line count | ~960 | **~1800** | ~450 |

## Troubleshooting

### "Command not found"

```bash
# Make executable
chmod +x thorough_diagnostic.py

# Run with python3 explicitly
python3 thorough_diagnostic.py
```

### "Permission denied" on Windows

```bash
# Run as administrator
python thorough_diagnostic.py
```

### "Module not found" errors

The tool uses only standard library. If you get import errors:
```bash
# Verify Python version (3.7+ required)
python3 --version

# Try with full path
/usr/bin/python3 thorough_diagnostic.py
```

### Slow performance

```bash
# Use quick mode to skip benchmarks
python3 thorough_diagnostic.py --quick
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success (diagnostic completed) |
| 1 | Interrupted by user (Ctrl+C) |

Note: The tool always completes and returns exit code 0, even if checks fail. Check the JSON report or terminal output for actual status.

## Best Practices

1. **Run before committing** - Catch issues early
2. **Use quick mode for rapid checks** - Save time during development
3. **Generate HTML reports for bug reports** - Easy to share and read
4. **Run full diagnostic before releases** - Ensure everything is working
5. **Archive diagnostic reports** - Track system health over time
6. **Test APIs separately** - Only when needed (requires internet)

## Examples

### Example 1: Daily Quick Check

```bash
$ python3 thorough_diagnostic.py --quick
# Takes ~2-5 seconds, skips slow tests
# Perfect for daily development validation
```

### Example 2: Pre-Release Full Validation

```bash
$ python3 thorough_diagnostic.py -v --html --markdown --output release_v1.6.0.json
# Generates:
# - release_v1.6.0.json (structured data)
# - release_v1.6.0.html (interactive report)
# - release_v1.6.0.md (markdown summary)
```

### Example 3: Bug Investigation

```bash
$ python3 thorough_diagnostic.py --verbose --test-apis --output bug_12345.json
# Full diagnostic including API connectivity
# Detailed output for debugging
```

### Example 4: CI/CD Pipeline

```bash
$ python3 thorough_diagnostic.py --quick --output ci_diagnostic.json
$ if [ $(jq '.summary.by_status.FAIL' ci_diagnostic.json) -gt 0 ]; then
$   echo "Critical failures detected!"
$   exit 1
$ fi
```

## FAQ

**Q: How long does it take?**
A: Quick mode: 2-5 seconds. Full mode: 5-15 seconds (depending on system).

**Q: Does it modify anything?**
A: No! It's read-only except for:
- Creating config directory if it doesn't exist
- Writing report files (JSON/HTML/Markdown)
- Small temporary file for disk I/O test (deleted immediately)

**Q: Can I run it while the app is running?**
A: Yes! It's completely separate and won't interfere.

**Q: What if I get warnings?**
A: Warnings are informational. Check the recommendation for guidance. Many warnings are expected before first run (e.g., "config not created yet").

**Q: What if I get failures?**
A: Failures indicate critical issues. Follow the recommendation to fix. Common failures:
- Missing executables → Rebuild application
- Database corrupted → Restore from backup
- Missing dependencies → Install required libraries

**Q: Can I customize what it tests?**
A: Not currently, but you can skip slow tests with `--quick` or skip API tests by not using `--test-apis`.

## Version History

**v2.0** (2026-01-18)
- Initial release of thorough diagnostic tool
- Comprehensive feature testing
- Multiple report formats (JSON, HTML, Markdown)
- Actionable recommendations
- Quick mode for rapid validation
- API connectivity testing
- Performance benchmarks
- Categorized results

## Contributing

To add new tests to thorough_diagnostic.py:

1. Add a new check method to the `ThoroughDiagnosticTool` class
2. Use `self.add_result()` to report findings with recommendations
3. Add the method to `run_all_checks()`
4. Update this documentation

## License

Same as AI File Sorter - GNU AGPL v3

## Support

For issues with the diagnostic tool:
1. Run with `--verbose` for detailed output
2. Check recommendations for each warning/failure
3. Generate HTML report for easier reading
4. Create GitHub issue with diagnostic report attached

---

**Last Updated:** 2026-01-18  
**Version:** 2.0  
**Maintainer:** AI File Sorter Development Team
