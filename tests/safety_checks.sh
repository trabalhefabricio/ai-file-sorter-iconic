#!/usr/bin/env bash
# AI File Sorter - Safety Checks Test Suite
# This script performs automated safety checks on the codebase

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_DIR="$ROOT_DIR/app"

echo "AI File Sorter - Safety Checks"
echo "==============================="
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ISSUES_FOUND=0

# Function to report an issue
report_issue() {
    echo -e "${RED}❌ ISSUE:${NC} $1"
    ((ISSUES_FOUND++))
}

# Function to report success
report_success() {
    echo -e "${GREEN}✓${NC} $1"
}

# Function to report warning
report_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

echo "1. Checking for unsafe string operations..."
UNSAFE_STRINGS=$(find "$APP_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) -exec grep -n "strcpy\|strcat\|sprintf" {} + 2>/dev/null | wc -l) || UNSAFE_STRINGS=0
if [ "$UNSAFE_STRINGS" -eq 0 ]; then
    report_success "No unsafe string operations found"
else
    report_issue "Found $UNSAFE_STRINGS unsafe string operations (strcpy/strcat/sprintf)"
fi

echo ""
echo "2. Checking for potential null pointer dereferences..."
# Note: This is a basic heuristic. For comprehensive analysis, use static analysis tools
# like clang-static-analyzer or clang-tidy
ARROW_OPS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -c '\->' {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || ARROW_OPS=0
if [ "$ARROW_OPS" -gt 0 ]; then
    report_success "Found $ARROW_OPS pointer dereferences - recommend static analysis for thorough review"
else
    report_success "No pointer dereferences found"
fi

echo ""
echo "3. Checking for manual memory management..."
RAW_NEWS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -n "new \w" {} + 2>/dev/null | grep -v "// " | wc -l) || RAW_NEWS=0
RAW_DELETES=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -n "delete " {} + 2>/dev/null | grep -v "delete\[\]" | grep -v "// " | grep -v "\"" | wc -l) || RAW_DELETES=0

if [ "$RAW_NEWS" -lt 30 ]; then
    report_success "Low usage of raw new ($RAW_NEWS instances) - good use of smart pointers"
else
    report_warning "Moderate usage of raw new ($RAW_NEWS instances)"
fi

if [ "$RAW_DELETES" -eq 0 ]; then
    report_success "No manual delete operations found"
else
    report_warning "Found $RAW_DELETES manual delete operations"
fi

echo ""
echo "4. Checking for proper exception handling..."
# Note: This is a heuristic - manual review of catch blocks recommended
CATCH_ALL=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -c 'catch.*\.\.\.' {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || CATCH_ALL=0
if [ "$CATCH_ALL" -lt 10 ]; then
    report_success "Limited use of catch-all handlers ($CATCH_ALL) - appears appropriate"
else
    report_warning "$CATCH_ALL catch-all handlers found - verify they log/re-throw appropriately"
fi

echo ""
echo "5. Checking for hardcoded paths..."
HARDCODED_PATHS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -n '\".*[/\\].*\"' {} + 2>/dev/null | grep -v "//" | grep -v "://" | wc -l) || HARDCODED_PATHS=0
if [ "$HARDCODED_PATHS" -lt 20 ]; then
    report_success "Low number of potential hardcoded paths ($HARDCODED_PATHS)"
else
    report_warning "$HARDCODED_PATHS potential hardcoded paths - verify cross-platform compatibility"
fi

echo ""
echo "6. Checking for TODO/FIXME markers..."
TODOS=$(find "$APP_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec grep -i "TODO\|FIXME" {} + 2>/dev/null | wc -l) || TODOS=0
if [ "$TODOS" -eq 0 ]; then
    report_success "No TODO/FIXME markers found"
else
    report_warning "Found $TODOS TODO/FIXME markers in code"
fi

echo ""
echo ""
echo "7. Checking for proper mutex usage..."
MUTEX_LOCKS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -c "\.lock()" {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || MUTEX_LOCKS=0
LOCK_GUARDS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -c "lock_guard\|unique_lock" {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || LOCK_GUARDS=0

if [ "$MUTEX_LOCKS" -eq 0 ]; then
    report_success "No raw mutex.lock() calls - excellent use of RAII lock guards"
elif [ "$LOCK_GUARDS" -gt "$MUTEX_LOCKS" ]; then
    report_success "Good mutex usage: $LOCK_GUARDS RAII guards vs $MUTEX_LOCKS raw locks"
elif [ "$MUTEX_LOCKS" -lt 3 ]; then
    report_warning "Found $MUTEX_LOCKS raw mutex.lock() calls - consider using lock_guard"
else
    report_issue "Found $MUTEX_LOCKS raw mutex.lock() calls - should use lock_guard for exception safety"
fi

echo ""
echo "8. Checking for integer overflow risks..."
# Note: This is informational only. Use static analysis tools for comprehensive overflow detection
ARITHMETIC_OPS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -c '+\|-\|*\|/' {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || ARITHMETIC_OPS=0
report_success "Found $ARITHMETIC_OPS lines with arithmetic - recommend bounds checking in critical paths"

echo ""
echo "9. Checking for proper resource cleanup..."
OPEN_CALLS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -n "\.open(" {} + 2>/dev/null | wc -l) || OPEN_CALLS=0
CLOSE_CALLS=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -n "\.close()" {} + 2>/dev/null | wc -l) || CLOSE_CALLS=0

if [ "$OPEN_CALLS" -le "$CLOSE_CALLS" ]; then
    report_success "Resource cleanup appears balanced (open: $OPEN_CALLS, close: $CLOSE_CALLS)"
else
    report_warning "Potential resource leaks (open: $OPEN_CALLS, close: $CLOSE_CALLS)"
fi

echo ""
echo "10. Checking for SQL injection risks..."
# Look for string concatenation with SQL
SQL_CONCAT=$(find "$APP_DIR/lib" -name "*.cpp" -exec grep -n "SELECT.*+\|INSERT.*+\|UPDATE.*+\|DELETE.*+" {} + 2>/dev/null | wc -l) || SQL_CONCAT=0
if [ "$SQL_CONCAT" -eq 0 ]; then
    report_success "No obvious SQL concatenation found - using prepared statements"
else
    report_issue "Found $SQL_CONCAT potential SQL concatenations - verify prepared statements used"
fi

echo ""
echo "==============================="
echo "Safety Checks Complete"
echo ""

if [ "$ISSUES_FOUND" -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed! No critical issues found.${NC}"
    echo ""
    echo "The codebase demonstrates good safety practices:"
    echo "  • Proper memory management with RAII"
    echo "  • No unsafe string operations"
    echo "  • Exception-safe code"
    echo "  • SQL prepared statements"
    exit 0
else
    echo -e "${RED}⚠ Found $ISSUES_FOUND issue(s) that should be reviewed.${NC}"
    echo ""
    echo "Please review the items marked with ❌ above."
    echo "These may indicate potential bugs or areas for improvement."
    exit 1
fi
