# 7-Taskbar-AutoSort

Periodically sort the windows inside every taskbar app group alphabetically
by title. Targets Windows 7, 8, and 10 (the classic `explorer.exe` taskbar).

Inspired by and forks from **7+ Taskbar Tweaker**
(<https://tweaker.ramensoftware.com/>). The full 7TT source tree is vendored
under `vendor/`. On top of it, this project adds a small polling thread that
periodically re-invokes 7TT's existing `SortButtonGroupItems` routine across
every grouped app on the taskbar. See [NOTES.md](NOTES.md) for the precise
diff against upstream.

> [!WARNING]
> This software injects a DLL into `explorer.exe`. A bug can crash the shell
> and freeze the desktop. **Do not test on your main machine.** Use a
> snapshot-capable VM (Hyper-V / VMware / VirtualBox) and snapshot before
> every test session.

> [!NOTE]
> Windows 11 is not supported. Its taskbar is a different process
> (`Taskbar.View.dll` hosted in XAML), and none of the techniques used here
> apply.

## Download Latest Release

**One-liner to download and run the latest release** (Windows cmd):

```cmd
powershell -c "Invoke-WebRequest -Uri 'https://github.com/ChocoTonic/7-Taskbar-AutoSort/releases/latest/download/7-Taskbar-AutoSort.exe' -OutFile $env:TEMP\7-Taskbar-AutoSort.exe -UseBasicParsing; & $env:TEMP\7-Taskbar-AutoSort.exe"
```

Or with curl:

```cmd
curl -L -o %TEMP%\7-Taskbar-AutoSort.exe https://github.com/ChocoTonic/7-Taskbar-AutoSort/releases/latest/download/7-Taskbar-AutoSort.exe && %TEMP%\7-Taskbar-AutoSort.exe
```

Or get the files manually from [Releases](https://github.com/ChocoTonic/7-Taskbar-AutoSort/releases).

## Build

Prerequisites:

- Windows 10 / 11 host (build host can be Win 11; _runtime_ target is
  Win 7/8/10)
- Visual Studio 2022 (or newer) Build Tools with the "Desktop development
  with C++" workload. Toolset is set to `$(DefaultPlatformToolset)` so any
  installed C++ workload should work — verified with VS 2022 (v143) and
  VS 2026 BuildTools (v180).
- GNU Make (`choco install make`, Git-bash, or MSYS2 make)

From a shell with `MSBuild.exe` on PATH (use "x64 Native Tools Command
Prompt for VS 2022" or run `vcvars64.bat`):

```cmd
make release           :: -> build\Release-x64\{7-Taskbar-AutoSort.exe, autosort.dll}
make debug
make clean
make package           :: zip the Release x64 binaries
make help
```

CI: GitHub Actions builds Release/x64 and Release/Win32 on every push and
uploads both binaries as workflow artifacts. See
[`.github/workflows/build.yml`](.github/workflows/build.yml).

## Run (inside a VM)

1. Copy `7-Taskbar-AutoSort.exe` and `autosort.dll` (must be side-by-side)
   into the VM.
2. Run `7-Taskbar-AutoSort.exe`. It will print `autosort.dll injected. Press
Ctrl-C to uninject and exit.` and stay running.
3. Open multiple windows of any app (Firefox, Notepad, RDP sessions, …).
4. Within ~2 seconds, the windows in each grouped taskbar button should be
   reordered alphabetically by title.
5. To stop: press Ctrl-C in the `7-Taskbar-AutoSort.exe` console. The DLL
   self-unloads cleanly.

If something goes wrong: kill `explorer.exe` from Task Manager, then
File → Run new task → `explorer.exe`. Everything injected goes away with
explorer.

## Layout

```
7-Taskbar-AutoSort/
├── 7-Taskbar-AutoSort.sln       Solution (two projects: injector exe + dll)
├── autosort/                    New code: polling thread that wraps 7TT's sort
├── inject/                      New code: console-app injector entry point
├── vendor/
│   ├── 7tt-dll/                 Vendored 7TT dll/ (incl. modified dll.c)
│   ├── 7tt-include/             Vendored 7TT include/
│   └── 7tt-inject/              Vendored 7TT exe-side injection (incl.
│                                modified explorer_inject.c)
├── docs/DESIGN.md
├── Makefile
├── LICENSE                      GPLv3
├── NOTES.md                     What was done, what's tested, what's pending
└── README.md
```

## How it works (one paragraph)

`7-Taskbar-AutoSort.exe` finds `explorer.exe`, builds a 7TT
`INJECT_INIT_STRUCT`, allocates memory in the remote process, and runs a
small shim there that `LoadLibrary`s `autosort.dll` and calls its exported
`Init()`. Inside explorer, `Init()` performs the full 7TT bootstrap
(version detection, function hooking, taskbar globals) and then calls
`AutosortInit()`, which starts a polling thread. Every 2 seconds the thread
sends `MSG_DLL_CALLFUNC_PARAM` to the taskbar window with a pointer to our
"enumerate-and-sort" function, which 7TT's existing wnd_proc dispatches on
the taskbar UI thread. There it walks the button-groups HDPA and calls
`SortButtonGroupItems` on every grouped entry — the exact routine 7TT's
manual "Sort items" menu has been calling since forever.

## License

GPLv3 (inherited from 7+ Taskbar Tweaker, since most of the source under
`vendor/` is copied from there). See [LICENSE](LICENSE).

Original 7+ Taskbar Tweaker by RaMMicHaeL —
<https://tweaker.ramensoftware.com/>.
