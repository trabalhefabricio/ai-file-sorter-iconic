#!/usr/bin/env bash
# AI File Sorter - Comprehensive Bug Fix Validation Tests
# Tests for bug prevention checklist items

set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/tests/build"
mkdir -p "$BUILD_DIR"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

test_passed() {
    echo -e "${GREEN}✓ PASS:${NC} $1"
    ((TESTS_PASSED++))
}

test_failed() {
    echo -e "${RED}✗ FAIL:${NC} $1"
    ((TESTS_FAILED++))
}

echo "AI File Sorter - Bug Prevention Validation Tests"
echo "================================================="
echo ""

# Test 1: Memory Safety - No unsafe string operations
echo "Test 1: Memory Safety - Unsafe String Operations"
UNSAFE_COUNT=$(find "$ROOT_DIR/app" -type f \( -name "*.cpp" -o -name "*.hpp" \) \
    -exec grep -l "strcpy\|strcat\|sprintf" {} + 2>/dev/null | wc -l) || UNSAFE_COUNT=0

if [ "$UNSAFE_COUNT" -eq 0 ]; then
    test_passed "No unsafe string operations found"
else
    test_failed "Found $UNSAFE_COUNT files with unsafe string operations"
fi

# Test 2: Thread Safety - No raw mutex locks without RAII
echo "Test 2: Thread Safety - Raw Mutex Usage"
RAW_LOCKS=$(find "$ROOT_DIR/app/lib" -name "*.cpp" \
    -exec grep -c '\.lock()' {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || RAW_LOCKS=0

if [ "$RAW_LOCKS" -eq 0 ]; then
    test_passed "No raw mutex.lock() calls - proper RAII usage"
else
    test_failed "Found $RAW_LOCKS raw mutex.lock() calls - should use lock_guard"
fi

# Test 3: Resource Management - Balanced open/close calls
echo "Test 3: Resource Management - File Handle Balance"
OPENS=$(find "$ROOT_DIR/app/lib" -name "*.cpp" \
    -exec grep -c '\.open(' {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || OPENS=0
CLOSES=$(find "$ROOT_DIR/app/lib" -name "*.cpp" \
    -exec grep -c '\.close()' {} + 2>/dev/null | awk '{sum+=$1} END {print sum+0}') || CLOSES=0

if [ "$OPENS" -le "$CLOSES" ]; then
    test_passed "Resource cleanup balanced (opens: $OPENS, closes: $CLOSES)"
else
    test_failed "Resource leak risk (opens: $OPENS, closes: $CLOSES)"
fi

# Test 4: SQL Security - No string concatenation in SQL
echo "Test 4: SQL Security - Injection Prevention"
SQL_CONCAT=$(find "$ROOT_DIR/app/lib" -name "*.cpp" \
    -exec grep -n "SELECT.*+\|INSERT.*+\|UPDATE.*+\|DELETE.*+" {} + 2>/dev/null | wc -l) || SQL_CONCAT=0

if [ "$SQL_CONCAT" -eq 0 ]; then
    test_passed "No SQL string concatenation - using prepared statements"
else
    test_failed "Found $SQL_CONCAT potential SQL injection risks"
fi

# Test 5: Error Handling - DialogUtils has copy functionality
echo "Test 5: Error Handling - Copyable Error Dialogs"
if grep -q "setTextInteractionFlags" "$ROOT_DIR/app/lib/DialogUtils.cpp" 2>/dev/null; then
    test_passed "Error dialogs support text selection and copying"
else
    test_failed "Error dialogs missing text selection capability"
fi

# Test 6: Input Validation - Path sanitization functions exist
echo "Test 6: Input Validation - Path Sanitization"
if grep -q "sanitize_path_label" "$ROOT_DIR/app/lib/Utils.cpp" 2>/dev/null; then
    test_passed "Path sanitization function exists"
else
    test_failed "Path sanitization function missing"
fi

# Test 7: Qt Memory Management - Check for parent-child relationships
echo "Test 7: Qt Memory Management - Parent Widget Usage"
# Count QWidget constructors that accept parent parameter
PARENT_USAGE=$(find "$ROOT_DIR/app/lib" -name "*Dialog.cpp" -print0 2>/dev/null | \
    xargs -0 grep -h 'QWidget.*parent' 2>/dev/null | wc -l) || PARENT_USAGE=0

if [ "$PARENT_USAGE" -gt 10 ]; then
    test_passed "Good Qt parent-child usage ($PARENT_USAGE instances)"
else
    test_failed "Limited Qt parent-child usage ($PARENT_USAGE instances)"
fi

# Test 8: Exception Safety - Check for proper catch blocks
echo "Test 8: Exception Safety - Catch Block Usage"
CATCH_BLOCKS=$(find "$ROOT_DIR/app/lib" -name "*.cpp" -print0 2>/dev/null | \
    xargs -0 grep -h 'catch' 2>/dev/null | wc -l) || CATCH_BLOCKS=0

if [ "$CATCH_BLOCKS" -gt 30 ]; then
    test_passed "Good exception handling ($CATCH_BLOCKS catch blocks)"
else
    test_failed "Limited exception handling ($CATCH_BLOCKS catch blocks)"
fi

# Test 9: Bounds Checking - Vector at() usage
echo "Test 9: Bounds Checking - Safe Container Access"
# This is informational - we don't require at() everywhere, just checking patterns
SAFE_ACCESS=$(find "$ROOT_DIR/app/lib" -name "*.cpp" -print0 2>/dev/null | \
    xargs -0 grep -h 'size()\|empty()' 2>/dev/null | wc -l) || SAFE_ACCESS=0

if [ "$SAFE_ACCESS" -gt 100 ]; then
    test_passed "Good bounds checking patterns ($SAFE_ACCESS size/empty checks)"
else
    test_failed "Limited bounds checking ($SAFE_ACCESS checks)"
fi

# Test 10: Logging - Logger null checks
echo "Test 10: Logging Safety - Null Logger Checks"
LOGGER_CHECKS=$(find "$ROOT_DIR/app/lib" -name "*.cpp" -print0 2>/dev/null | \
    xargs -0 grep -h 'if.*logger\|logger.*&&' 2>/dev/null | wc -l) || LOGGER_CHECKS=0

if [ "$LOGGER_CHECKS" -gt 30 ]; then
    test_passed "Good logger null checking ($LOGGER_CHECKS checks)"
else
    test_failed "Limited logger null checking ($LOGGER_CHECKS checks)"
fi

echo ""
echo "================================================="
echo "Test Results Summary"
echo "================================================="
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
echo "Total:  $((TESTS_PASSED + TESTS_FAILED))"
echo ""

if [ "$TESTS_FAILED" -eq 0 ]; then
    echo -e "${GREEN}✓ All bug prevention tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed. Review the failures above.${NC}"
    exit 1
fi
