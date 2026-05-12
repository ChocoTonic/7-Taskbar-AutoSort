// inject.c — taskbar-autosort injector.
//
// Windows app entry point. Orchestrates:
// - Settings initialization (registry)
// - Auto-update check on startup (if interval has elapsed)
// - DLL extraction from resources
// - Explorer injection
// - System tray icon with context menu (Check Updates, Settings, Exit)

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "explorer_inject.h"
#include "options_def.h"
#include "library_load_errors.h"
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

    while (i-- && szExePath[i] != L'\\');
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
    (void)hPrevInst; (void)pCmdLine; (void)nShowCmd;

    MessageBoxW(NULL, L"wWinMain entry point reached", L"7-Taskbar-AutoSort", MB_OK);

    AllocConsole();
    FILE *pFile = NULL;
    freopen_s(&pFile, "CONOUT$", "w", stdout);

    FILE *pDebugLog = NULL;
    fopen_s(&pDebugLog, "C:\\autosort_inject.log", "w");
    if (pDebugLog) {
        fprintf(pDebugLog, "=== 7-Taskbar-AutoSort startup ===\n");
        fflush(pDebugLog);
    } else {
        MessageBoxW(NULL, L"Failed to open debug log", L"Error", MB_OK);
    }

    wprintf(L"[DEBUG] Starting 7-Taskbar-AutoSort\n");
    MessageBoxW(NULL, L"AllocConsole successful", L"7-Taskbar-AutoSort", MB_OK);

    int opts[OPTS_COUNT];
    ZeroMemory(opts, sizeof(opts));

    wprintf(L"[DEBUG] Initializing settings\n");
    if (pDebugLog) { fprintf(pDebugLog, "[1/5] Initializing settings...\n"); fflush(pDebugLog); }
    SettingsInit();
    MessageBoxW(NULL, L"Step 1/5: Settings initialized", L"Progress", MB_OK);
    wprintf(L"[DEBUG] Settings initialized\n");
    if (pDebugLog) { fprintf(pDebugLog, "[1/5] OK - Settings initialized\n"); fflush(pDebugLog); }

    wprintf(L"[DEBUG] Checking for updates\n");
    if (pDebugLog) { fprintf(pDebugLog, "[2/5] Checking for updates...\n"); fflush(pDebugLog); }
    time_t lastCheck = SettingsGetLastCheckTime();
    int interval = SettingsGetUpdateInterval();
    time_t now = time(NULL);

    if (now - lastCheck >= (time_t)(interval * 86400))
    {
        wprintf(L"[DEBUG] Update interval elapsed, checking for updates\n");
        if (pDebugLog) { fprintf(pDebugLog, "     Update interval elapsed, fetching GitHub...\n"); fflush(pDebugLog); }
        WCHAR szNewVersion[64] = {0};
        SettingsSetLastCheckTime(now);

        if (UpdateCheckAvailable(szNewVersion, 64))
        {
            wprintf(L"[DEBUG] Update found: %s\n", szNewVersion);
            if (pDebugLog) { fprintf(pDebugLog, "     Update found: %S\n", szNewVersion); fflush(pDebugLog); }
            WCHAR szPrompt[256];
            swprintf_s(szPrompt, 256, L"Update available: v%s\n\nWould you like to update now?", szNewVersion);
            if (MessageBoxW(NULL, szPrompt, L"Update Available", MB_YESNO | MB_ICONINFORMATION) == IDYES)
            {
                if (pDebugLog) { fprintf(pDebugLog, "     User chose to update, calling UpdateApply...\n"); fflush(pDebugLog); }
                UpdateApply(szNewVersion);
                return 0;
            }
        }
        else {
            wprintf(L"[DEBUG] No update available\n");
            if (pDebugLog) { fprintf(pDebugLog, "     No update available\n"); fflush(pDebugLog); }
        }
    }
    MessageBoxW(NULL, L"Step 2/5: Update check complete", L"Progress", MB_OK);
    if (pDebugLog) { fprintf(pDebugLog, "[2/5] OK - Update check complete\n"); fflush(pDebugLog); }

    wprintf(L"[DEBUG] Extracting DLL\n");
    if (pDebugLog) { fprintf(pDebugLog, "[3/5] Extracting DLL from resources...\n"); fflush(pDebugLog); }
    if (!ExtractDll()) {
        wprintf(L"[DEBUG] ERROR: Failed to extract DLL\n");
        if (pDebugLog) { fprintf(pDebugLog, "[3/5] ERROR: Failed to extract DLL\n"); fflush(pDebugLog); }
        MessageBoxW(NULL, L"Failed to extract DLL", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(NULL, L"Step 3/5: DLL extracted", L"Progress", MB_OK);
    if (pDebugLog) { fprintf(pDebugLog, "[3/5] OK - DLL extracted\n"); fflush(pDebugLog); }

    wprintf(L"[DEBUG] DLL extracted, injecting into explorer\n");
    if (pDebugLog) { fprintf(pDebugLog, "[4/5] Injecting DLL into explorer.exe...\n"); fflush(pDebugLog); }
    DWORD err = ExplorerInject(
        NULL,
        WM_APP,
        GetUserDefaultUILanguage(),
        opts,
        NULL
    );

    if (err != 0) {
        wprintf(L"[DEBUG] ERROR: ExplorerInject failed with code %lu\n", err);
        if (pDebugLog) { fprintf(pDebugLog, "[4/5] ERROR: ExplorerInject failed with code %lu\n", err); fflush(pDebugLog); }
        WCHAR szError[256];
        swprintf_s(szError, 256, L"ExplorerInject failed with code %lu", err);
        MessageBoxW(NULL, szError, L"Injection Error", MB_OK | MB_ICONERROR);
        return (int)err;
    }
    MessageBoxW(NULL, L"Step 4/5: DLL injected into explorer", L"Progress", MB_OK);
    if (pDebugLog) { fprintf(pDebugLog, "[4/5] OK - DLL injected into explorer\n"); fflush(pDebugLog); }

    wprintf(L"[DEBUG] Creating tray icon\n");
    if (pDebugLog) { fprintf(pDebugLog, "[5/5] Creating system tray icon...\n"); fflush(pDebugLog); }
    if (!TrayInit(hInst)) {
        wprintf(L"[DEBUG] ERROR: Failed to create tray icon\n");
        if (pDebugLog) { fprintf(pDebugLog, "[5/5] ERROR: Failed to create tray icon\n"); fflush(pDebugLog); }
        MessageBoxW(NULL, L"Failed to create system tray icon", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(NULL, L"Step 5/5: Tray icon created - entering message loop", L"Progress", MB_OK);
    if (pDebugLog) { fprintf(pDebugLog, "[5/5] OK - Tray icon created\n\n"); fflush(pDebugLog); }

    wprintf(L"[DEBUG] Tray icon created, entering message loop\n");
    if (pDebugLog) { fprintf(pDebugLog, "=== Startup complete, entering message loop ===\n"); fflush(pDebugLog); }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (pDebugLog) { fprintf(pDebugLog, "=== Message loop exited, shutting down ===\n"); fflush(pDebugLog); }

    ExplorerCleanup();
    TrayDestroy();

    if (pDebugLog) {
        fprintf(pDebugLog, "=== Shutdown complete ===\n");
        fflush(pDebugLog);
        fclose(pDebugLog);
    }
    return 0;
}
