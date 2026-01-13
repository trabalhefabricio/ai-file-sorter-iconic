# Testing and Diagnostic Tools

This directory contains comprehensive testing and diagnostic tools for AI File Sorter.

## Quick Start

### 1. Run Diagnostic Tool (Recommended First Step)

```bash
# Basic diagnostic
python3 diagnostic_tool.py

# Verbose output with JSON report
python3 diagnostic_tool.py --verbose --output diagnostic_report.json

# Windows
python diagnostic_tool.py --verbose --output diagnostic_report.json
```

**What it checks:**
- System information (OS, architecture, Python version)
- File system structure (executables, directories)
- Dependencies (Qt, libraries)
- LLM backends (CPU, CUDA, Vulkan, Metal)
- Database integrity
- Configuration files
- Log files
- Feature implementations
- Performance metrics

**Output:** Color-coded terminal output + JSON report

**Duration:** ~1-5 seconds

---

### 2. Run Feature Validator

```bash
# Basic validation
./feature_validator.sh

# Verbose output
./feature_validator.sh --verbose
```

**What it validates:**
- Source code structure
- Feature implementations
- Database schema
- Configuration files
- Translation files
- Test infrastructure
- Documentation
- Dependencies
- LLM backends
- Code quality
- Security checks

**Output:** Color-coded terminal output with pass/warn/fail indicators

**Duration:** ~1-3 seconds

---

### 3. Manual Testing

Follow one of these guides based on your needs:

#### Quick Testing (5-30 minutes)
See [QUICK_TESTING_CHECKLIST.md](QUICK_TESTING_CHECKLIST.md)

- **Smoke Test:** 5 minutes - Absolute minimum validation
- **Core Features:** 15 minutes - Essential functionality
- **Critical Path:** 30 minutes - End-to-end workflow

#### Comprehensive Testing (4-6 hours)
See [MANUAL_TESTING_GUIDE.md](MANUAL_TESTING_GUIDE.md)

- 28 detailed test cases
- Step-by-step procedures
- Expected results
- Troubleshooting guides
- Test report templates

---

## Tool Details

### diagnostic_tool.py

**Purpose:** Comprehensive system health check and validation

**Features:**
- ✅ Cross-platform (Windows, Linux, macOS)
- ✅ Color-coded output
- ✅ JSON report generation
- ✅ Verbose mode
- ✅ No dependencies beyond Python stdlib
- ✅ 26+ diagnostic checks

**Usage:**
```bash
python3 diagnostic_tool.py [--verbose] [--output FILE]
```

**Options:**
- `--verbose, -v` : Show detailed information
- `--output, -o FILE` : Save JSON report to file (default: auto-generated)

**Example Output:**
```
╔════════════════════════════════════════════════════════════════╗
║           AI FILE SORTER - DIAGNOSTIC TOOL                     ║
╚════════════════════════════════════════════════════════════════╝

================================================================================
SYSTEM INFORMATION
================================================================================
  ℹ Platform: Linux 5.15.0
  ℹ Architecture: x86_64
  ℹ Python Version: 3.10.12

================================================================================
FILE SYSTEM STRUCTURE
================================================================================
  ✓ Executable: aifilesorter: Found
  ✓ Directory: app/include: Found (59 files)
  ✓ Directory: app/lib: Found (56 files)
  ...

Overall Health: EXCELLENT
```

**Exit Codes:**
- 0: Success
- 1: Interrupted by user or unexpected error

---

### feature_validator.sh

**Purpose:** Automated validation of feature implementations

**Features:**
- ✅ Bash-based (Linux/macOS native)
- ✅ Color-coded output
- ✅ Fast execution (<1 minute)
- ✅ CI/CD friendly
- ✅ 50+ validation checks

**Usage:**
```bash
./feature_validator.sh [--verbose]
```

**Options:**
- `--verbose, -v` : Show detailed information

**Example Output:**
```
╔════════════════════════════════════════════════════════════════╗
║        AI FILE SORTER - AUTOMATED FEATURE VALIDATOR            ║
╚════════════════════════════════════════════════════════════════╝

═══════════════════════════════════════════════════════════
FEATURE IMPLEMENTATION CHECK
═══════════════════════════════════════════════════════════

  ✓ PASS: File scanning implemented
  ✓ PASS: Categorization service implemented
  ✓ PASS: Google Gemini API implemented
  ✓ PASS: User profiling implemented
  ...

Results:
  ✓ Passed:  28
  ⚠ Warnings: 5
  ✗ Failed:  0

╔════════════════════════════════════════════════════════════════╗
║  ⚠ VALIDATION PASSED WITH WARNINGS                             ║
╚════════════════════════════════════════════════════════════════╝
```

**Exit Codes:**
- 0: Passed (with or without warnings)
- 1: Failed

---

## Testing Guides

### QUICK_TESTING_CHECKLIST.md

**Purpose:** Rapid testing reference for quick validation

**Contents:**
- Pre-flight checks
- 5-minute smoke test
- 15-minute core feature tests
- 30-minute critical path testing
- Platform-specific quick tests
- Error scenario tests
- Performance quick checks
- Quick issue resolution

**When to use:**
- Daily development validation
- Pre-commit checks
- Quick bug verification
- CI/CD pipeline integration

---

### MANUAL_TESTING_GUIDE.md

**Purpose:** Comprehensive manual testing procedures

**Contents:**
- 28 detailed test cases covering:
  - Core features (scanning, categorization)
  - AI/LLM integrations (local, OpenAI, Gemini)
  - User profiling and learning
  - File management (dry run, undo)
  - UI/UX features (dialogs, widgets)
  - Platform-specific features
  - Error handling
  - Performance benchmarking
- Step-by-step procedures
- Expected results for each test
- Troubleshooting sections
- Test report templates

**When to use:**
- Pre-release validation
- Major feature changes
- Bug investigation
- Quality assurance cycles

---

## Integration with Development Workflow

### Daily Development

```bash
# Quick check before committing
./feature_validator.sh

# If issues found, run full diagnostic
python3 diagnostic_tool.py --verbose
```

**Time:** 1-2 minutes

---

### Pre-Commit

```bash
# 1. Run feature validator
./feature_validator.sh --verbose

# 2. If warnings or failures, investigate
python3 diagnostic_tool.py --verbose --output pre_commit_diagnostic.json

# 3. Run relevant manual tests from QUICK_TESTING_CHECKLIST.md
```

**Time:** 5-10 minutes

---

### Pre-PR (Pull Request)

```bash
# 1. Full diagnostic
python3 diagnostic_tool.py --verbose --output pr_diagnostic.json

# 2. Feature validation
./feature_validator.sh --verbose

# 3. Core feature manual testing (30 min)
# Follow QUICK_TESTING_CHECKLIST.md → "Critical Path Testing"

# 4. Test affected features specifically
# Use relevant sections from MANUAL_TESTING_GUIDE.md
```

**Time:** 30-60 minutes

---

### Pre-Release

```bash
# 1. Full diagnostic on all platforms
python3 diagnostic_tool.py --verbose --output release_diagnostic_linux.json
# Repeat on Windows, macOS

# 2. Feature validation on all platforms
./feature_validator.sh --verbose
# Repeat on Windows, macOS

# 3. Complete manual testing
# Follow entire MANUAL_TESTING_GUIDE.md

# 4. Performance benchmarking
# Use performance test sections
```

**Time:** 4-8 hours (depending on platforms tested)

---

## CI/CD Integration

### Example GitHub Actions Workflow

```yaml
name: Feature Validation

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.10'
      
      - name: Run diagnostic tool
        run: |
          python3 diagnostic_tool.py --output diagnostic_report.json
      
      - name: Run feature validator
        run: |
          chmod +x feature_validator.sh
          ./feature_validator.sh --verbose
      
      - name: Upload diagnostic report
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: diagnostic-report
          path: diagnostic_report.json
```

### Example GitLab CI

```yaml
validate:
  stage: test
  script:
    - python3 diagnostic_tool.py --output diagnostic_report.json
    - ./feature_validator.sh --verbose
  artifacts:
    paths:
      - diagnostic_report.json
    reports:
      junit: diagnostic_report.json
```

---

## Troubleshooting

### Diagnostic Tool Issues

**Python not found:**
```bash
# Linux/macOS: Install Python 3
sudo apt install python3  # Debian/Ubuntu
brew install python3      # macOS

# Windows: Download from python.org
```

**Permission denied:**
```bash
chmod +x diagnostic_tool.py
```

**Module import errors:**
- Diagnostic tool uses only Python standard library
- Ensure Python 3.7+ is installed

---

### Feature Validator Issues

**Script not executable:**
```bash
chmod +x feature_validator.sh
```

**Bash not found (Windows):**
- Use Git Bash or WSL (Windows Subsystem for Linux)
- Or run diagnostic_tool.py instead (cross-platform)

**Command not found errors:**
- Some checks require `git`, `make`, `cmake`
- These are optional; warnings will be shown

---

## Testing Metrics

### Code Coverage

Current test coverage:
- Unit tests: ~60% of core utilities
- Integration tests: Manual only (see guides)
- Feature coverage: 100% (all features have test cases)

### Validation Coverage

Automated validation checks:
- File structure: 100%
- Feature implementation: 100%
- Database schema: 95%
- Dependencies: 90%
- Configuration: 100%
- Documentation: 100%

---

## Contributing

### Adding New Tests

**For diagnostic_tool.py:**
1. Add new method to `DiagnosticTool` class
2. Call from `run_all_checks()`
3. Use `add_result()` to report findings
4. Update this README

**For feature_validator.sh:**
1. Add new test section with `print_header()`
2. Use `pass()`, `fail()`, `warn()` for results
3. Update this README

**For manual tests:**
1. Add test case to MANUAL_TESTING_GUIDE.md
2. Follow existing format (objective, steps, expected results)
3. Add quick reference to QUICK_TESTING_CHECKLIST.md

---

## Related Documentation

- **Feature Analysis:** [FEATURE_ANALYSIS.md](FEATURE_ANALYSIS.md) - Complete feature documentation
- **Error Codes:** [ERROR_CODES.md](ERROR_CODES.md) - Error reference
- **Troubleshooting:** [TROUBLESHOOTING_STARTUP.md](TROUBLESHOOTING_STARTUP.md) - Startup issues
- **Bug Reporting:** [QUICK_BUG_REPORT.md](QUICK_BUG_REPORT.md) - Bug report template
- **Implementation Plan:** [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) - Feature roadmap

---

## FAQ

### Q: Which tool should I run first?

**A:** Always start with `diagnostic_tool.py`. It provides the most comprehensive overview and will identify critical issues.

### Q: How long does full testing take?

**A:** 
- Automated tools: 1-5 minutes
- Quick manual tests: 5-30 minutes
- Full manual testing: 4-6 hours

### Q: Can I automate manual tests?

**A:** Some tests can be automated with UI testing frameworks (Qt Test, Playwright). The manual testing guide serves as a specification for automation.

### Q: What if diagnostic tool shows failures?

**A:** 
1. Review the failure details
2. Check if files are missing (rebuild needed?)
3. Verify dependencies installed
4. See TROUBLESHOOTING section above

### Q: Do I need to run all manual tests?

**A:** No. Use QUICK_TESTING_CHECKLIST.md for rapid validation. Full manual testing is for pre-release validation.

### Q: Can I run tests on CI/CD?

**A:** Yes! Both diagnostic_tool.py and feature_validator.sh are CI/CD friendly. See "CI/CD Integration" section above.

---

## Support

For issues with testing tools:
1. Check troubleshooting section above
2. Review error messages carefully
3. Create issue with diagnostic report attached
4. Include test environment details (OS, Python version, etc.)

---

## Version History

- **v1.0** (2026-01-13)
  - Initial release
  - Diagnostic tool with 26+ checks
  - Feature validator with 50+ validations
  - Comprehensive manual testing guide (28 tests)
  - Quick testing checklist

---

## License

Same as AI File Sorter - GNU AGPL v3

---

**Last Updated:** 2026-01-13  
**Maintainer:** AI File Sorter Development Team
