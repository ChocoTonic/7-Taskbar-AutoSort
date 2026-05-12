// inject.c — taskbar-autosort injector.
//
// Embeds autosort.dll as a resource, extracts it to the same directory as
// the exe on startup, then calls ExplorerInject() (vendored from 7+ Taskbar
// Tweaker's exe/explorer_inject.c) to load it into explorer.exe.
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

static BOOL ExtractDll(void)
{
    wchar_t szExePath[MAX_PATH];
    wchar_t szDllPath[MAX_PATH];
    HRSRC hRsrc;
    HGLOBAL hGlobal;
    void *pData;
    DWORD dwSize;
    HANDLE hFile;
    DWORD dwWritten;

    // Get the executable path and strip the filename
    int i = GetModuleFileName(NULL, szExePath, MAX_PATH);
    if (i == 0) {
        fwprintf(stderr, L"GetModuleFileName failed\n");
        return FALSE;
    }
    while (i-- && szExePath[i] != L'\\');
    lstrcpy(&szExePath[i + 1], L"autosort.dll");
    lstrcpy(szDllPath, szExePath);

    // Find the embedded resource
    hRsrc = FindResource(NULL, MAKEINTRESOURCE(101), RT_RCDATA);
    if (!hRsrc) {
        fwprintf(stderr, L"FindResource failed: %lu\n", GetLastError());
        return FALSE;
    }

    hGlobal = LoadResource(NULL, hRsrc);
    if (!hGlobal) {
        fwprintf(stderr, L"LoadResource failed: %lu\n", GetLastError());
        return FALSE;
    }

    pData = LockResource(hGlobal);
    dwSize = SizeofResource(NULL, hRsrc);
    if (!pData || dwSize == 0) {
        fwprintf(stderr, L"LockResource/SizeofResource failed\n");
        return FALSE;
    }

    // Delete old DLL if it exists
    DeleteFile(szDllPath);

    // Write DLL to disk
    hFile = CreateFile(szDllPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        // If CREATE_NEW fails, try to open and overwrite
        hFile = CreateFile(szDllPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            fwprintf(stderr, L"CreateFile failed: %lu\n", GetLastError());
            return FALSE;
        }
    }

    if (!WriteFile(hFile, pData, dwSize, &dwWritten, NULL) || dwWritten != dwSize) {
        fwprintf(stderr, L"WriteFile failed: %lu\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

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

    // Extract embedded DLL to same directory as exe
    if (!ExtractDll()) {
        fwprintf(stderr, L"Failed to extract DLL\n");
        return 1;
    }

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
