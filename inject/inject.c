#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "explorer_inject.h"
#include "library_load_errors.h"
#include "options_def.h"
#include "tray.h"
#include "update.h"
#include "settings.h"

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

    int i = GetModuleFileNameW(NULL, szExePath, MAX_PATH);
    if (i == 0) return FALSE;

    while (i-- && szExePath[i] != L'\\')
        ;
    lstrcpyW(&szExePath[i + 1], L"autosort.dll");
    lstrcpyW(szDllPath, szExePath);

    hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(101), RT_RCDATA);
    if (!hRsrc) return FALSE;

    hGlobal = LoadResource(NULL, hRsrc);
    if (!hGlobal) return FALSE;

    pData = LockResource(hGlobal);
    dwSize = SizeofResource(NULL, hRsrc);
    if (!pData || dwSize == 0) return FALSE;

    DeleteFileW(szDllPath);

    hFile = CreateFileW(szDllPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    if (!WriteFile(hFile, pData, dwSize, &dwWritten, NULL) || dwWritten != dwSize)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR pCmdLine, int nShowCmd)
{
    (void)hPrevInst;
    (void)pCmdLine;
    (void)nShowCmd;

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"7TaskbarAutoSort_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(NULL,
                    L"7-Taskbar-AutoSort is already running.\n\nCheck the system tray (notification area).",
                    L"7-Taskbar-AutoSort",
                    MB_OK | MB_ICONINFORMATION);
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    int opts[OPTS_COUNT];
    ZeroMemory(opts, sizeof(opts));

    SettingsInit();

    time_t lastCheck = SettingsGetLastCheckTime();
    int interval = SettingsGetUpdateInterval();
    time_t now = time(NULL);

    if (now - lastCheck >= (time_t)(interval * 86400))
    {
        SettingsSetLastCheckTime(now);
        UpdatePromptIfAvailable(NULL);
    }

    if (!ExtractDll())
    {
        MessageBoxW(
            NULL,
            L"Failed to extract autosort.dll to the application directory.\n\nCheck that the folder is writable.",
            L"7-Taskbar-AutoSort",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    DWORD err = ExplorerInject(NULL, WM_APP, GetUserDefaultUILanguage(), opts, NULL);
    if (err != 0)
    {
        WCHAR szMsg[384];
        const WCHAR *pReason = L"Unknown error.";
        switch (err)
        {
        case EXE_ERR_NO_DLL:
            pReason = L"autosort.dll not found next to the EXE.";
            break;
        case EXE_ERR_NO_TASKBAR:
            pReason = L"Taskbar window not found. Is Explorer running?";
            break;
        case EXE_ERR_NO_TASKLIST:
            pReason = L"Task list window not found (unsupported shell).";
            break;
        case EXE_ERR_HIDDEN_TASKLIST:
            pReason = L"Windows 11 taskbar is not supported.";
            break;
        case EXE_ERR_OPEN_PROCESS:
            pReason = L"Could not open Explorer process. Try running as Administrator.";
            break;
        case INJ_ERR_BEFORE_RUN:
            pReason = L"Injected code failed to start (DEP or antivirus may be blocking it).";
            break;
        case INJ_ERR_LOADLIBRARY:
            pReason = L"Explorer could not load autosort.dll (missing dependency or bad DLL).";
            break;
        case EXE_ERR_VIRTUAL_ALLOC:
            pReason = L"VirtualAllocEx failed — Exploit Protection may be blocking injection.";
            break;
        case EXE_ERR_CREATE_REMOTE_THREAD:
            pReason = L"CreateRemoteThread failed. Try running as Administrator.";
            break;
        }
        swprintf_s(szMsg, ARRAYSIZE(szMsg), L"Could not inject into Explorer (error %lu).\n\n%s", err, pReason);
        MessageBoxW(NULL, szMsg, L"7-Taskbar-AutoSort", MB_OK | MB_ICONERROR);
        return (int)err;
    }

    if (!TrayInit(hInst))
    {
        ExplorerCleanup();
        MessageBoxW(NULL, L"Failed to create the system tray icon.", L"7-Taskbar-AutoSort", MB_OK | MB_ICONERROR);
        return 1;
    }

    TrayShowBalloon(L"7-Taskbar-AutoSort", L"Running. Right-click the tray icon for options.");

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ExplorerCleanup();
    TrayDestroy();
    return 0;
}
