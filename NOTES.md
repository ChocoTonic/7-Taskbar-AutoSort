# Implementation Notes

## Final state

7-Taskbar-AutoSort is a **fork** of 7+ Taskbar Tweaker. The full 7TT source
tree lives under `vendor/7tt-dll/`, `vendor/7tt-include/`, and
`vendor/7tt-inject/`. On top of it, two surgical additions wire in periodic
alphabetical sorting:

- `autosort/autosort_module.c` — a polling thread that, every 2 seconds, asks
  the taskbar UI thread to walk every grouped button group and call the
  existing `SortButtonGroupItems` routine.
- `vendor/7tt-dll/dll.c` — patched in three places to (a) include the new
  header, (b) call `AutosortInit()` after `WndProcInit()` succeeds, and
  (c) call `AutosortShutdown()` in the wait-thread cleanup path. Each change
  is marked `// taskbar-autosort addition`.
- `vendor/7tt-inject/explorer_inject.c` — patched to load `autosort.dll`
  instead of `inject.dll`. Marked `// taskbar-autosort: renamed DLL`.

Result on disk:

```
7-Taskbar-AutoSort/
├── 7-Taskbar-AutoSort.sln                Solution
├── LICENSE                               GPLv3 (inherited from 7TT)
├── Makefile                              MSBuild wrapper
├── README.md
├── NOTES.md (this file)
├── docs/DESIGN.md
├── .github/workflows/build.yml           GHA: windows-latest, x64 + Win32
├── autosort/
│   ├── autosort_module.c                 NEW — polling thread
│   └── autosort_module.h
├── inject/
│   └── inject.c                          NEW — console app calling ExplorerInject
└── vendor/
    ├── 7tt-dll/                          Vendored from 7TT dll/
    │   ├── dll.c                         MODIFIED (3 spots, marked)
    │   ├── autosort.vcxproj              NEW
    │   ├── _exports.def                  MODIFIED (LIBRARY name)
    │   └── … (all other 7TT dll/ files unchanged)
    ├── 7tt-include/                      Vendored from 7TT include/, unchanged
    └── 7tt-inject/                       Vendored from 7TT exe/explorer_inject*.c
        ├── explorer_inject.c             MODIFIED (DLL name only)
        ├── inject.vcxproj                NEW
        └── … (other vendored files unchanged)
```

## Build status

**Verified building** on Windows with VS 2026 Build Tools (Platform Toolset
v180). `make release` produces `build\Release-x64\7-Taskbar-AutoSort.exe`
(~123 KB) and `build\Release-x64\autosort.dll` (~282 KB). 0 warnings,
0 errors.

The PlatformToolset is set to `$(DefaultPlatformToolset)` so the project
builds on whichever C++ workload is installed — VS 2022 (v143) or newer.

Two non-default settings:

- `WholeProgramOptimization` is **off** in both vcxprojs. Required because
  the prebuilt `vendor/7tt-dll/MinHook/libMinHook.x64.lib` was compiled
  with VS 2022 v14.36, and LTCG (`/GL`) refuses to mix object files
  produced by different compiler major versions (error C1047). Build with
  WPO off works fine — slight perf cost, no functional change.
- vcxprojs live under `vendor/7tt-dll/` and `vendor/7tt-inject/` so the
  vendored 7TT `#include "..."` directives resolve naturally to sibling
  files. Our own sources are pulled in via relative paths
  (`..\..\autosort\autosort_module.c`, `..\..\inject\inject.c`).

## Build-side things that worked

| | |
|---|---|
| Vcxproj source-file lists | Match the original 7TT `dll/7 Taskbar Tweaker.vcxproj` (20 `.c` files + our 1) and `exe/exe.vcxproj` slice (2 `.c` files + our 1). |
| PCH (`stdafx.h`) | `include\stdafx.c` is `PrecompiledHeader=Create`; everything else inherits `Use`. The two `explorer_inject*.c` files are overridden to `NotUsing` because they live outside the dll/ tree (vendored from exe/). |
| Resource compile | `vendor/7tt-dll/rsrc.rc` compiles cleanly. |
| MinHook static lib link | `MinHook\libMinHook.x64.lib` resolves from the project dir as expected. |
| GHA workflow | `windows-latest` runner has VS 2022 (v143). The `$(DefaultPlatformToolset)` resolves to v143 there. Matrix builds x64 and Win32. |

## Runtime — still needs VM testing

The build works, but the *runtime behavior* hasn't been verified (per the
project's VM-only policy). On the VM:

1. **`7-Taskbar-AutoSort.exe` should stay alive** (console window). Ctrl-C
   uninjects.
2. **`autosort.dll` loaded into `explorer.exe`** — confirm with Process
   Explorer or Process Hacker.
3. **Open 3+ Firefox / Chrome windows** (or any multi-window app), wait
   ~2 seconds, watch the group order change to alphabetical.

Things that might surprise on first run:

- **Visible flicker** every 2 s as the sort runs. Tune
  `AUTOSORT_POLL_INTERVAL_MS` in `autosort/autosort_module.c` if annoying.
- **Fighting the user.** Drag-reorder a window and the next tick reverts it.
  Mitigation pointer: check the hot-tracking state in
  `vendor/7tt-dll/com_func_hook.c:1696` and skip the tick.
- **Antivirus.** `CreateRemoteThread` injection is a common AV trigger.
  Sign the binaries if distributing.

## Sort cadence and behavior (unchanged from upstream)

- **Interval:** 2000 ms (`AUTOSORT_POLL_INTERVAL_MS`). Tune freely.
- **Scope:** every group whose `button_group_type` is 1 or 3 (the two
  "grouped windows" types). Single-window apps and pinned-only apps are
  skipped automatically.
- **Sort key:** `StrCmpLogicalW` on the window title — the same locale-aware
  comparison Explorer uses for File Explorer's "name" column (digits sort
  numerically, etc.). Inherited from 7TT's existing routine.
- **Multi-monitor:** primary taskbar only. Secondary monitors not yet
  covered — iterate via `EV_TASK_SW_MULTI_TASK_LIST_REF` in
  `vendor/7tt-dll/explorer_vars.c` to add.

## Threading correctness

- Polling thread is created by `CreateThread` in `AutosortInit()`. Lives in
  `explorer.exe`.
- The thread does only two things per tick: read globals (`hTaskbarWnd`,
  `lpTaskListLongPtr`, `uTweakerMsg`) and `SendMessage`. Globals are
  written once during `Init()` and never mutated after.
- `SendMessage` blocks until the wnd_proc on the taskbar UI thread returns.
  So the actual button-group walk + sort runs serialized on Explorer's UI
  thread — the only safe place to touch its data.
- Shutdown: `AutosortShutdown()` is called from 7TT's `WaitThread` (runs
  when the injector exits or its clean event fires). Signals the stop event
  and joins with a 5 s timeout.

## Optional next moves (not done)

| Improvement | Where to add | Effort |
|---|---|---|
| Skip ticks while user is dragging/flyout-open | `AutosortAllGroupsOnTaskbarThread` — check hot-tracking state | 1 hour |
| Cover secondary monitors | Iterate `EV_TASK_SW_MULTI_TASK_LIST_REF` | 2 hours |
| Configurable interval / disable per-app | INI via 7TT's `portable_settings.h` | half day |
| Sort by alternate keys (process name, etc.) | Replace `InternalGetWindowText` call inside `SortButtonGroupItems` — small fork | half day |
| Run on user logon | Task Scheduler entry (no installer needed) | half day |

## License

GPLv3. See `LICENSE` (copied from 7TT). Original 7+ Taskbar Tweaker by
RaMMicHaeL — <https://tweaker.ramensoftware.com/>.
