#!/bin/bash
# Cross-platform pre-build validation script
# Runs on macOS/Linux; validates Windows C source for common issues
# Does NOT require MSVC or Windows

set -e

ERRORS=0
WARNINGS=0

RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

check_file_exists() {
    local file=$1
    if [ ! -f "$file" ]; then
        echo -e "${RED}ERROR: $file not found${NC}"
        ERRORS=$((ERRORS + 1))
    fi
}

check_pattern() {
    local pattern=$1
    local file=$2
    local description=$3

    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo -e "${RED}ERROR: $file - $description${NC}"
        ERRORS=$((ERRORS + 1))
    fi
}

check_include() {
    local header=$1
    local file=$2

    if ! grep -q "#include.*$header" "$file" 2>/dev/null; then
        echo -e "${YELLOW}WARNING: $file - missing expected include <$header>${NC}"
        WARNINGS=$((WARNINGS + 1))
    fi
}

echo "=== Pre-build validation (macOS) ==="
echo ""

# Check essential source files exist
echo "Checking source files..."
check_file_exists "inject/inject.c"
check_file_exists "inject/tray.c"
check_file_exists "inject/update.c"
check_file_exists "inject/settings.c"
check_file_exists "autosort/autosort_module.c"
check_file_exists "vendor/7tt-inject/inject.vcxproj"
echo "  ✓ All source files present"

# Check inject.c has required includes for stdio
echo ""
echo "Checking critical includes..."
check_include "stdio.h" "inject/inject.c"
check_include "windows.h" "inject/inject.c"
check_include "time.h" "inject/inject.c"
check_include "winhttp.h" "inject/update.c"

# Check for unmatched braces (basic check)
echo ""
echo "Checking syntax basics..."
for file in inject/*.c autosort/*.c; do
    if [ -f "$file" ]; then
        open_braces=$(grep -o '{' "$file" | wc -l)
        close_braces=$(grep -o '}' "$file" | wc -l)
        if [ $open_braces -ne $close_braces ]; then
            echo -e "${RED}ERROR: $file - mismatched braces ($open_braces open, $close_braces close)${NC}"
            ERRORS=$((ERRORS + 1))
        fi
    fi
done
echo "  ✓ Brace matching OK"

# Check for common typos
echo ""
echo "Checking for common issues..."
for file in inject/*.c autosort/*.c; do
    if [ -f "$file" ]; then
        # Check for file I/O functions without stdio.h
        if grep -q "freopen_s\|fprintf\|fopen" "$file" && ! grep -q "#include.*stdio.h" "$file"; then
            echo -e "${RED}ERROR: $file - uses file I/O but missing #include <stdio.h>${NC}"
            ERRORS=$((ERRORS + 1))
        fi
    fi
done
echo "  ✓ No obvious symbol issues"

# Check version string exists
echo ""
echo "Checking version constants..."
check_file_exists "inject/autosort_version.h"
if grep -q "AUTOSORT_VERSION_STR" inject/autosort_version.h; then
    echo "  ✓ Version string defined"
else
    echo -e "${RED}ERROR: inject/autosort_version.h - missing AUTOSORT_VERSION_STR${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Check resource file exists
echo ""
echo "Checking resources..."
check_file_exists "inject/inject.rc"
check_file_exists "inject/resource.h"
echo "  ✓ Resource files present"

# Summary
echo ""
echo "=== Validation Summary ==="
echo -e "Errors:   ${RED}$ERRORS${NC}"
echo -e "Warnings: ${YELLOW}$WARNINGS${NC}"

if [ $ERRORS -gt 0 ]; then
    echo -e "${RED}VALIDATION FAILED${NC}"
    exit 1
fi

echo -e "${GREEN}VALIDATION PASSED${NC} (ready to build on Windows)"
exit 0
