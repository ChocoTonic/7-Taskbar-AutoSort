// inject.c — taskbar-autosort injector.
//
// Calls ExplorerInject() (vendored from 7+ Taskbar Tweaker's exe/explorer_inject.c)
// to load autosort.dll into explorer.exe with a valid INJECT_INIT_STRUCT, then
// stays alive holding the keep-loaded handle until the user signals stop.
//
// The DLL's WaitThread inside explorer waits on (a) this process's handle and
// (b) a clean event. If this process exits, the DLL unloads itself. So we must
// stay running. Ctrl-C (or closing the console) triggers a graceful uninject.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "explorer_inject.h"
#include "options_def.h"
#include "library_load_errors.h"

static HANDLE g_hQuit;

static BOOL WINAPI CtrlHandler(DWORD type)
{
    (void)type;
    if (g_hQuit) SetEvent(g_hQuit);
    return TRUE;
}

int wmain(int argc, wchar_t **argv)
{
    (void)argc; (void)argv;

    int opts[OPTS_COUNT];
    ZeroMemory(opts, sizeof(opts));

    g_hQuit = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!g_hQuit) { fwprintf(stderr, L"CreateEvent failed: %lu\n", GetLastError()); return 1; }

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    DWORD err = ExplorerInject(
        NULL,                          // hTweakerWnd: no UI window
        WM_APP,                        // uEjectedMsg: posted to NULL, ignored
        GetUserDefaultUILanguage(),
        opts,
        NULL                           // pIniFile: use registry defaults
    );

    if (err != 0)
    {
        fwprintf(stderr, L"ExplorerInject failed (code %lu)\n", err);
        return (int)err;
    }

    wprintf(L"autosort.dll injected. Press Ctrl-C to uninject and exit.\n");

    WaitForSingleObject(g_hQuit, INFINITE);

    ExplorerCleanup();
    wprintf(L"Uninjected.\n");
    return 0;
}
