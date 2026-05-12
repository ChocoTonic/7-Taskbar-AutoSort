# Build Scripts

## validate-build.sh

Cross-platform pre-build validation (macOS/Linux).

**Runs without MSVC** — validates C source for common issues before pushing to GitHub Actions.

### Usage

```bash
./scripts/validate-build.sh
```

### Checks performed

- Source files exist (inject.c, tray.c, update.c, settings.c, autosort_module.c)
- Critical includes present (#include <stdio.h>, <windows.h>, <winhttp.h>, etc.)
- Brace matching (mismatched `{}`)
- File I/O functions paired with stdio.h
- Version string constant defined
- Resource files present

### Exit codes

- `0` — All checks passed, safe to push
- `1` — One or more errors found, fix before pushing

### What it doesn't check

- MSVC-specific errors (use `make lint` on Windows for that)
- Function signatures or type mismatches (requires MSVC compiler)
- Linker issues or missing libraries
- Runtime errors

### On Windows

Run the full build with lint checks:

```bash
make lint CONFIG=Release PLATFORM=x64
```

This uses MSVC's `/analyze` flag to catch:
- Undefined symbols
- Type mismatches
- Uninitialized variables
- Potential buffer overruns
- Memory leaks
