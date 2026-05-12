# 7-Taskbar-AutoSort: Development Conversation

**Date**: May 11, 2026  
**Project**: 7-Taskbar-AutoSort (Windows taskbar auto-sorting with auto-update)  
**Status**: Build validation system added; pre-release v1.0.22+

---

## Overview

This document captures the development conversation around adding build validation, cross-platform pre-build checks, and establishing a repeatable build pipeline for the Windows MSVC project.

### Context

The project was initially a headless console app. Recent work added:
- System tray icon with context menu
- Auto-update feature (GitHub releases integration)
- Settings dialog (update check interval)
- Debug logging for taskbar sort operations

The next phase focused on **build validation** to catch compilation errors locally before pushing to GitHub Actions.

---

## Problem Statement

**User Request**: "add maketarget are there not tests we can run to make sure it will build properly"

**Challenge**: The project is a Windows-specific MSVC application being developed on macOS. MSVC compiler isn't available on macOS, so:
- Can't run actual compilation locally
- Need cross-platform validation that works on macOS
- Need MSVC-specific static analysis for Windows builds

---

## Solution Implemented

### 1. Cross-Platform Validation Script (`scripts/validate-build.sh`)

**Purpose**: Run on macOS/Linux before pushing; catch common C issues without MSVC.

**Checks**:
- Source files exist (inject.c, tray.c, update.c, settings.c, autosort_module.c)
- Critical includes present (#include <stdio.h>, <windows.h>, <winhttp.h>, etc.)
- Brace matching ({} pairs)
- File I/O functions paired with stdio.h
- Version string constant defined (AUTOSORT_VERSION_STR)
- Resource files present (inject.rc, resource.h)

**Usage**:
```bash
./scripts/validate-build.sh
```

**Exit codes**:
- `0` — Validation passed, safe to push
- `1` — Errors found, fix before pushing

### 2. Makefile Lint Target

**Purpose**: Run on Windows; use MSVC static analysis to catch compilation issues.

**Command**:
```bash
make lint CONFIG=Release PLATFORM=x64
```

**Checks** (MSVC /analyze flag):
- Undefined symbols / unresolved externals
- Type mismatches (casting errors, function signature mismatches)
- Uninitialized variables
- Unreachable code
- Buffer overruns
- Memory leaks

### 3. Documentation

**File**: `scripts/README.md`

Explains:
- When to use each validation tool
- What each tool checks for
- What issues each tool **doesn't** catch
- Full build workflow

---

## Design Decisions

### Why Not Docker Windows Build?

**User Question**: "Docker Windows build (complex) how is this complex?"

**Answer**: It's not inherently complex, but the current approach is better:

| Approach | Pros | Cons |
|----------|------|------|
| **Native Windows Runner** (current) | Fast, simple, GitHub-hosted, no Docker overhead | Requires Windows runner pool |
| **Docker Windows Container** | Isolated environment, reproducible | ~2GB image, ~1-2 min startup overhead, more complex GitHub Actions setup |

The current GitHub Actions setup uses native Windows runners (already configured), which is faster and simpler than Docker.

### Why Cross-Platform Validation?

The validation script on macOS:
- **Doesn't require MSVC** — works on any Unix-like system
- **Catches syntax errors** before pushing to CI
- **Saves CI cycles** — avoids failed builds due to obvious issues
- **Fast feedback** — runs in <1 second
- **Complementary** — works alongside MSVC lint, not instead of it

---

## Workflow

### Before Committing (on macOS)

```bash
./scripts/validate-build.sh
```

If it passes, you're clear to commit and push.

### Before Releasing (on Windows)

```bash
make lint CONFIG=Release PLATFORM=x64
```

If no errors, proceed with:
```bash
make release
```

### In GitHub Actions

The CI/CD pipeline:
1. Checks out code
2. Builds with MSVC on Windows runner
3. Creates GitHub release with EXE artifact
4. Automatically tagged with semantic version (v1.0.X format)

---

## Files Modified / Created

### New Files
- `scripts/validate-build.sh` — Cross-platform validation script
- `scripts/README.md` — Build tools documentation

### Modified Files
- `Makefile` — Added lint target, updated help text

---

## Lint Failure Types (MSVC /analyze)

### Example 1: Missing Include
```c
freopen_s(&pFile, "CONOUT$", "w", stdout);  // ERROR: stdio.h not included
```
**MSVC detects**: C4013 'freopen_s' undefined

### Example 2: Type Mismatch
```c
int val = PSGetSingleInt(...);  // Returns different type than expected
DWORD dwSize = (int)val;        // Cast warning/error
```
**MSVC detects**: C4305 truncation, C4267 conversion

### Example 3: Uninitialized Variable
```c
int opts[OPTS_COUNT];
// Used without ZeroMemory
ExplorerInject(..., opts, ...);
```
**MSVC detects**: C6001 use of uninitialized memory

### Example 4: Buffer Overrun
```c
WCHAR szBuffer[64];
wcscpy_s(szBuffer, MAX_PATH, pInput);  // MAX_PATH > 64
```
**MSVC detects**: Buffer overrun warning via `/analyze`

---

## Testing & Verification

The validation script was tested on macOS and passes all checks:

```
=== Pre-build validation (macOS) ===

Checking source files...
  ✓ All source files present

Checking critical includes...

Checking syntax basics...
  ✓ Brace matching OK

Checking for common issues...
  ✓ No obvious symbol issues

Checking version constants...
  ✓ Version string defined

Checking resources...
  ✓ Resource files present

=== Validation Summary ===
Errors:   0
Warnings: 0
VALIDATION PASSED (ready to build on Windows)
```

---

## What the Validation Doesn't Catch

These require MSVC or runtime testing:
- Function signature mismatches (requires compiler)
- Incorrect library dependencies (requires linker)
- API availability on specific Windows versions
- Runtime crashes or logic errors
- Performance issues

Use `make lint` on Windows to catch those.

---

## Next Steps

1. **Before any commit**: Run `./scripts/validate-build.sh` on macOS
2. **Before release**: Run `make lint` on Windows
3. **Monitor**: GitHub Actions will catch remaining issues
4. **Debug**: If CI fails, check `build/obj/` logs for MSVC error details

---

## Summary

**Problem**: How to validate Windows builds locally on macOS?

**Solution**: 
- Cross-platform bash script for syntax/structure checks on macOS
- MSVC lint target for static analysis on Windows
- Clear separation: quick local checks + comprehensive compiler analysis

**Result**: Catch issues early, avoid wasted CI cycles, faster feedback loop.
