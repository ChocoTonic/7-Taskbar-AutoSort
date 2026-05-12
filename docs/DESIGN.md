# Design

## Goal

Periodically (every ~2 s) walk every taskbar app group, and if the group
contains multiple windows, reorder them alphabetically by title. Works
on Windows 7, 8, 10 (classic `explorer.exe` taskbar).

## Two-binary shape

```
inject.exe                 standalone, runs once per user session
  └─ remote LoadLibraryW ──▶ autosort.dll (inside explorer.exe)
                              ├─ DllMain spawns polling thread
                              └─ polling thread:
                                  every 2s:
                                    find Shell_TrayWnd → … → MSTaskListWClass
                                    for each button_group of type 1|3:
                                        sort items by window title
```

The split mirrors how 7+ Taskbar Tweaker (and most "Explorer mod" tools)
work. The injector is a tiny utility; all logic lives in the DLL.

## Why polling, not hooks

We considered hooking Explorer's window-creation path so the sort
triggers immediately on each new window. Decided against it because:

- Polling is *radically* simpler — no MinHook, no API patching, no
  per-Windows-version offset tables for hookable functions.
- For a 2 s cadence, the visible difference vs. event-driven is small.
- Fewer moving parts → fewer ways to crash Explorer.

If we later need to *suspend* sorting during user interaction (dragging,
flyout open), we can either poll for that state too or selectively add a
single hook on `OnButtonGroupHotTracking`. Deferred until we hit the
problem.

## Window discovery

The taskbar window chain (all documented class names):

```
Shell_TrayWnd               (top-level, owned by explorer.exe)
└─ ReBarWindow32
   └─ MSTaskSwWClass
      └─ MSTaskListWClass   ← this is what we want
```

`FindWindowEx` walks this chain. Implemented in
`autosort\explorer_taskbar.c::FindTaskListWnd`.

On multi-monitor setups, secondary monitors get `Shell_SecondaryTrayWnd`.
v1 only handles the primary; multi-monitor is a v2 nicety.

## Reaching Explorer internals (the unsolved part)

`MSTaskListWClass` is a normal HWND, but its window-procedure backing is
a C++ object (`CTaskListWnd` / `CTaskBand`). The button-groups array
hangs off that object at a build-specific offset.

7TT solves this in `dll\explorer_vars.c` with a table of offsets per
Explorer build/architecture, and macros like
`EV_MM_TASKLIST_BUTTON_GROUPS_HDPA(lpMMTaskListLongPtr)` that expand to
the right `*(LONG_PTR**)(p + offset)` for the running OS.

We have three options to handle this standalone:

1. **Port the small slice** (~200 lines of `explorer_vars.[ch]` and
   adjacent helpers). Fast. Forces project to GPLv3.
2. **Reverse-engineer fresh.** Slow (1–2 weeks of debugger work) but
   keeps MIT.
3. **Use public Windows APIs only.** Doesn't exist for this — there's
   no documented API to enumerate taskbar buttons within a group, let
   alone reorder them.

Decision deferred. Scaffold uses **stubs** so the build/run loop works
and the integration can be filled in by either path. See `NOTES.md` Gap
1–3.

## Sort algorithm

Given a button group containing N task items:

1. Read each item's `HWND`.
2. `GetWindowTextW(hwnd, buf, len)` → window title.
3. Sort with `CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, ...)`.
4. Apply the new order to the DPA via `DPA_Sort` with a comparator, OR
   manually rewrite the DPA pointer array in-place.
5. Trigger a taskbar refresh (likely `InvalidateRect` on the tasklist
   window suffices; 7TT calls into a dedicated routine).

## Threading

- Polling thread is created in `DllMain`'s `DLL_PROCESS_ATTACH` (use
  `_beginthreadex` to be safe with the MSVC runtime).
- The thread waits on a manual-reset stop event with
  `WaitForSingleObject(stopEvent, POLL_INTERVAL_MS)`. On `WAIT_OBJECT_0`
  it exits cleanly.
- `DLL_PROCESS_DETACH` (which fires on remote `FreeLibrary` from
  `inject.exe /uninject`) sets the stop event and joins the thread with
  a timeout. We never block detach for long.

⚠️ Touching Explorer's data structures from a non-Explorer thread is
unsafe — those structures are owned by the taskbar UI thread. The
final implementation must either: (a) marshal the sort onto the
taskbar thread via `PostMessage`, or (b) use a known-safe API/lock.
7TT does (a) — see `SendMessage(hTaskbarWnd, uTweakerMsg, ..., MSG_DLL_CALLFUNC_PARAM)`.
This scaffold's polling thread runs on its own thread and *does not yet*
marshal — safe only because the sort call itself is a stub.

## Telemetry / debugging

A simple `AutosortLog` writes wide-string lines to `%TEMP%\autosort.log`.
The DLL has no UI; this is how we confirm it's alive.

For more involved debugging:

- `WinDbg` attached to `explorer.exe` (the VM makes this safe).
- The DLL is built with `Optimization=Disabled` and PDBs in Debug.
