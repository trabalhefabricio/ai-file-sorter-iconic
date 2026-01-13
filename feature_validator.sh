#!/usr/bin/env bash
# AI File Sorter - Automated Feature Validation Script
#
# This script performs automated validation of features that can be tested
# without user interaction. Use this as a quick health check.
#
# Usage: ./feature_validator.sh [--verbose]

set -e

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Configuration
VERBOSE=0
PASSED=0
FAILED=0
WARNINGS=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--verbose]"
            exit 1
            ;;
    esac
done

# Helper functions
print_header() {
    echo -e "\n${BOLD}${MAGENTA}═══════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${MAGENTA}$1${NC}"
    echo -e "${BOLD}${MAGENTA}═══════════════════════════════════════════════════════════════════${NC}\n"
}

print_test() {
    echo -e "${CYAN}Testing: $1${NC}"
}

pass() {
    echo -e "${GREEN}  ✓ PASS:${NC} $1"
    PASSED=$((PASSED + 1))
}

fail() {
    echo -e "${RED}  ✗ FAIL:${NC} $1"
    FAILED=$((FAILED + 1))
}

warn() {
    echo -e "${YELLOW}  ⚠ WARN:${NC} $1"
    WARNINGS=$((WARNINGS + 1))
}

info() {
    if [ $VERBOSE -eq 1 ]; then
        echo -e "${BLUE}  ℹ INFO:${NC} $1"
    fi
}

# Start validation
echo -e "${BOLD}${BLUE}"
cat << "EOF"
╔════════════════════════════════════════════════════════════════════════════╗
║           AI FILE SORTER - AUTOMATED FEATURE VALIDATOR                     ║
║                 Quick Health Check & Validation                            ║
╚════════════════════════════════════════════════════════════════════════════╝
EOF
echo -e "${NC}"

START_TIME=$(date +%s)

# ==================== File Structure Validation ====================
print_header "FILE STRUCTURE VALIDATION"

print_test "Application source structure"
if [ -d "app" ] && [ -d "app/lib" ] && [ -d "app/include" ]; then
    pass "Source directories exist (app, app/lib, app/include)"
else
    fail "Missing critical source directories"
fi

print_test "Build artifacts"
if [ -f "app/bin/aifilesorter" ] || [ -f "app/bin/aifilesorter-bin" ]; then
    pass "Application executable found"
elif [ -f "app/build-windows/Release/aifilesorter.exe" ]; then
    pass "Windows application executable found"
else
    warn "Application executable not found (may need to build)"
fi

print_test "Critical source files"
CRITICAL_FILES=(
    "app/main.cpp"
    "app/include/MainApp.hpp"
    "app/lib/DatabaseManager.cpp"
    "app/lib/CategorizationService.cpp"
)

for file in "${CRITICAL_FILES[@]}"; do
    if [ -f "$file" ]; then
        info "Found: $file"
    else
        fail "Missing: $file"
    fi
done

# ==================== Feature Implementation Check ====================
print_header "FEATURE IMPLEMENTATION CHECK"

print_test "Core Features"
CORE_FEATURES=(
    "app/lib/FileScanner.cpp:File scanning"
    "app/lib/CategorizationService.cpp:Categorization service"
    "app/lib/LocalLLMClient.cpp:Local LLM support"
    "app/lib/LLMClient.cpp:OpenAI API support"
)

for entry in "${CORE_FEATURES[@]}"; do
    file="${entry%%:*}"
    feature="${entry##*:}"
    if [ -f "$file" ]; then
        pass "$feature implemented"
    else
        fail "$feature implementation not found"
    fi
done

print_test "Custom Features"
CUSTOM_FEATURES=(
    "app/lib/GeminiClient.cpp:Google Gemini API"
    "app/lib/UserProfileManager.cpp:User profiling"
    "app/lib/FileTinderDialog.cpp:File Tinder tool"
    "app/lib/CacheManagerDialog.cpp:Cache manager"
    "app/lib/DryRunPreviewDialog.cpp:Dry run preview"
    "app/lib/UndoManager.cpp:Persistent undo"
)

for entry in "${CUSTOM_FEATURES[@]}"; do
    file="${entry%%:*}"
    feature="${entry##*:}"
    if [ -f "$file" ]; then
        pass "$feature implemented"
    else
        warn "$feature implementation not found"
    fi
done

# ==================== Database Schema Validation ====================
print_header "DATABASE SCHEMA VALIDATION"

print_test "Database schema definition"
if [ -f "app/include/DatabaseManager.hpp" ]; then
    # Check for expected table definitions in the code
    EXPECTED_TABLES=(
        "categorization_cache"
        "taxonomy"
        "confidence_scores"
        "api_usage_tracking"
        "user_profiles"
        "categorization_sessions"
        "undo_history"
        "file_tinder_state"
    )
    
    for table in "${EXPECTED_TABLES[@]}"; do
        if grep -q "$table" app/lib/DatabaseManager.cpp 2>/dev/null; then
            info "Table '$table' referenced in code"
        else
            warn "Table '$table' not found in DatabaseManager"
        fi
    done
    pass "Database schema files exist"
else
    fail "DatabaseManager header not found"
fi

# ==================== Configuration Validation ====================
print_header "CONFIGURATION VALIDATION"

print_test "Build configuration"
if [ -f "app/Makefile" ] || [ -f "app/CMakeLists.txt" ]; then
    pass "Build system configured"
else
    fail "No build system configuration found"
fi

print_test "Resource files"
if [ -d "app/resources" ]; then
    ICON_COUNT=$(find app/resources -name "*.png" -o -name "*.svg" 2>/dev/null | wc -l)
    if [ "$ICON_COUNT" -gt 0 ]; then
        pass "Resource files found ($ICON_COUNT images)"
    else
        warn "No image resources found"
    fi
else
    warn "Resources directory not found"
fi

# ==================== Translation Files Check ====================
print_header "INTERNATIONALIZATION CHECK"

print_test "Translation files"
if [ -d "app/resources/i18n" ]; then
    LANG_COUNT=$(find app/resources/i18n -name "*.ts" 2>/dev/null | wc -l)
    if [ "$LANG_COUNT" -gt 0 ]; then
        pass "Translation files found ($LANG_COUNT languages)"
        if [ $VERBOSE -eq 1 ]; then
            find app/resources/i18n -name "*.ts" -exec basename {} \; | while read file; do
                info "Found translation: $file"
            done
        fi
    else
        warn "No translation files found"
    fi
else
    warn "i18n directory not found"
fi

# ==================== Test Infrastructure Check ====================
print_header "TEST INFRASTRUCTURE CHECK"

print_test "Unit tests"
if [ -d "tests/unit" ]; then
    TEST_COUNT=$(find tests/unit -name "*.cpp" 2>/dev/null | wc -l)
    if [ "$TEST_COUNT" -gt 0 ]; then
        pass "Unit tests found ($TEST_COUNT test files)"
    else
        warn "No unit test files found"
    fi
else
    warn "Unit test directory not found"
fi

print_test "Test scripts"
TEST_SCRIPTS=(
    "tests/run_all_tests.sh"
    "tests/run_database_tests.sh"
    "tests/bug_prevention_tests.sh"
)

for script in "${TEST_SCRIPTS[@]}"; do
    if [ -f "$script" ]; then
        info "Found: $(basename $script)"
    fi
done

# ==================== Documentation Check ====================
print_header "DOCUMENTATION CHECK"

print_test "User documentation"
USER_DOCS=(
    "README.md"
    "CHANGELOG.md"
    "MANUAL_TESTING_GUIDE.md"
)

for doc in "${USER_DOCS[@]}"; do
    if [ -f "$doc" ]; then
        pass "Found: $doc"
    else
        warn "Missing: $doc"
    fi
done

print_test "Developer documentation"
DEV_DOCS=(
    "FEATURE_ANALYSIS.md"
    "IMPLEMENTATION_PLAN.md"
    "ERROR_CODES.md"
    "TROUBLESHOOTING_STARTUP.md"
)

for doc in "${DEV_DOCS[@]}"; do
    if [ -f "$doc" ]; then
        info "Found: $doc"
    else
        info "Missing: $doc"
    fi
done

# ==================== Dependency Check ====================
print_header "DEPENDENCY CHECK"

print_test "System dependencies"

# Check for common build tools
check_command() {
    if command -v "$1" &> /dev/null; then
        pass "$1 available"
        if [ $VERBOSE -eq 1 ]; then
            VERSION=$($1 --version 2>/dev/null | head -n1 || echo "unknown")
            info "  Version: $VERSION"
        fi
    else
        warn "$1 not found in PATH"
    fi
}

check_command "git"
check_command "make"
check_command "cmake"
check_command "g++"
check_command "python3"

# Check for Qt
print_test "Qt framework"
if command -v pkg-config &> /dev/null; then
    if pkg-config --exists Qt6Widgets 2>/dev/null; then
        QT_VERSION=$(pkg-config --modversion Qt6Widgets)
        pass "Qt6 found (version $QT_VERSION)"
    else
        warn "Qt6 not found via pkg-config"
    fi
else
    info "pkg-config not available, cannot check Qt"
fi

# ==================== LLM Backend Check ====================
print_header "LLM BACKEND CHECK"

print_test "GGML libraries"
if [ -d "app/lib/ggml" ]; then
    for variant in wocuda wcuda wvulkan; do
        if [ -d "app/lib/ggml/$variant" ]; then
            LIB_COUNT=$(find "app/lib/ggml/$variant" -name "*.so" -o -name "*.dll" -o -name "*.dylib" 2>/dev/null | wc -l)
            if [ "$LIB_COUNT" -gt 0 ]; then
                pass "Backend $variant available ($LIB_COUNT libraries)"
            else
                info "Backend $variant directory exists but no libraries"
            fi
        else
            info "Backend $variant not found (optional)"
        fi
    done
else
    warn "GGML library directory not found"
fi

print_test "Precompiled llama libraries"
if [ -d "app/lib/precompiled" ]; then
    VARIANT_COUNT=$(find app/lib/precompiled -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
    if [ "$VARIANT_COUNT" -gt 0 ]; then
        pass "Found $VARIANT_COUNT precompiled variant(s)"
    else
        warn "Precompiled directory exists but is empty"
    fi
else
    warn "Precompiled libraries directory not found"
fi

# ==================== Code Quality Checks ====================
print_header "CODE QUALITY CHECKS"

print_test "Source code statistics"
if command -v cloc &> /dev/null; then
    info "Running cloc (this may take a moment)..."
    CLOC_OUTPUT=$(cloc app --quiet --csv 2>/dev/null || echo "")
    if [ -n "$CLOC_OUTPUT" ]; then
        pass "Source code analysis complete"
        if [ $VERBOSE -eq 1 ]; then
            echo "$CLOC_OUTPUT" | grep "C++\|C/C++" | while IFS=, read lang files blank comment code; do
                info "  C++ files: $files, Lines: $code"
            done
        fi
    fi
else
    info "cloc not installed (optional)"
fi

print_test "Error handling coverage"
if [ -f "app/include/ErrorCode.hpp" ]; then
    ERROR_CODE_COUNT=$(grep -c "enum class Code" app/include/ErrorCode.hpp 2>/dev/null || echo "0")
    if grep -q "enum class Code" app/include/ErrorCode.hpp 2>/dev/null; then
        UNIQUE_CODES=$(grep -oE '[0-9]{4}' app/include/ErrorCode.hpp 2>/dev/null | sort -u | wc -l)
        pass "Error code system implemented ($UNIQUE_CODES unique error codes)"
    else
        warn "Error code enum not found"
    fi
else
    warn "ErrorCode.hpp not found"
fi

# ==================== Security Checks ====================
print_header "SECURITY CHECKS"

print_test "API key handling"
API_KEY_REFS=$(grep -r "api.*key" app/include/*.hpp app/lib/*.cpp 2>/dev/null | grep -v "API key" | grep -v "api_key" | head -n 1)
if [ -n "$API_KEY_REFS" ]; then
    warn "Found potential hardcoded API key references (review needed)"
else
    pass "No obvious hardcoded API keys found"
fi

print_test "Sensitive data handling"
if grep -r "password\|secret\|token" app/lib/*.cpp 2>/dev/null | grep -v "// " | grep -v "password:" | wc -l > /dev/null; then
    info "Found sensitive data references (verify proper handling)"
else
    pass "No obvious sensitive data issues"
fi

# ==================== Diagnostic Tool Check ====================
print_header "DIAGNOSTIC TOOLS CHECK"

print_test "Diagnostic tools"
DIAG_TOOLS=(
    "diagnostic_tool.py:Python diagnostic tool"
    "emergency_diagnostic.bat:Windows emergency diagnostic"
    "MANUAL_TESTING_GUIDE.md:Manual testing guide"
)

for entry in "${DIAG_TOOLS[@]}"; do
    file="${entry%%:*}"
    tool="${entry##*:}"
    if [ -f "$file" ]; then
        pass "$tool available"
    else
        warn "$tool not found"
    fi
done

# ==================== Final Summary ====================
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

print_header "VALIDATION SUMMARY"

echo -e "${BOLD}Results:${NC}"
echo -e "  ${GREEN}✓ Passed:  $PASSED${NC}"
echo -e "  ${YELLOW}⚠ Warnings: $WARNINGS${NC}"
echo -e "  ${RED}✗ Failed:  $FAILED${NC}"
echo -e "\n${BOLD}Duration: ${DURATION}s${NC}"

# Overall status
echo ""
if [ $FAILED -eq 0 ]; then
    if [ $WARNINGS -eq 0 ]; then
        echo -e "${GREEN}${BOLD}╔════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}${BOLD}║  ✓ VALIDATION PASSED - ALL GREEN!     ║${NC}"
        echo -e "${GREEN}${BOLD}╚════════════════════════════════════════╝${NC}"
        exit 0
    else
        echo -e "${YELLOW}${BOLD}╔════════════════════════════════════════╗${NC}"
        echo -e "${YELLOW}${BOLD}║  ⚠ VALIDATION PASSED WITH WARNINGS    ║${NC}"
        echo -e "${YELLOW}${BOLD}╚════════════════════════════════════════╝${NC}"
        exit 0
    fi
else
    echo -e "${RED}${BOLD}╔════════════════════════════════════════╗${NC}"
    echo -e "${RED}${BOLD}║  ✗ VALIDATION FAILED                   ║${NC}"
    echo -e "${RED}${BOLD}╚════════════════════════════════════════╝${NC}"
    echo -e "\n${YELLOW}Review the failures above and fix them before proceeding.${NC}"
    exit 1
fi
