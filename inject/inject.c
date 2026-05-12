// inject.c — taskbar-autosort injector.
//
// Windows app entry point. Orchestrates:
// - Settings initialization (registry)
// - Auto-update check on startup (if interval has elapsed)
// - DLL extraction from resources
// - Explorer injection
// - System tray icon with context menu (Check Updates, Settings, Exit)

#include <windows.h>
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

    AllocConsole();
    FILE *pFile = NULL;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    wprintf(L"[DEBUG] Starting 7-Taskbar-AutoSort\n");

    int opts[OPTS_COUNT];
    ZeroMemory(opts, sizeof(opts));

    wprintf(L"[DEBUG] Initializing settings\n");
    SettingsInit();
    wprintf(L"[DEBUG] Settings initialized\n");

    wprintf(L"[DEBUG] Checking for updates\n");
    time_t lastCheck = SettingsGetLastCheckTime();
    int interval = SettingsGetUpdateInterval();
    time_t now = time(NULL);

    if (now - lastCheck >= (time_t)(interval * 86400))
    {
        wprintf(L"[DEBUG] Update interval elapsed, checking for updates\n");
        WCHAR szNewVersion[64] = {0};
        SettingsSetLastCheckTime(now);

        if (UpdateCheckAvailable(szNewVersion, 64))
        {
            wprintf(L"[DEBUG] Update found: %s\n", szNewVersion);
            WCHAR szPrompt[256];
            swprintf_s(szPrompt, 256, L"Update available: v%s\n\nWould you like to update now?", szNewVersion);
            if (MessageBoxW(NULL, szPrompt, L"Update Available", MB_YESNO | MB_ICONINFORMATION) == IDYES)
            {
                UpdateApply(szNewVersion);
                return 0;
            }
        }
        else {
            wprintf(L"[DEBUG] No update available\n");
        }
    }

    wprintf(L"[DEBUG] Extracting DLL\n");
    if (!ExtractDll()) {
        wprintf(L"[DEBUG] ERROR: Failed to extract DLL\n");
        MessageBoxW(NULL, L"Failed to extract DLL", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    wprintf(L"[DEBUG] DLL extracted, injecting into explorer\n");
    DWORD err = ExplorerInject(
        NULL,
        WM_APP,
        GetUserDefaultUILanguage(),
        opts,
        NULL
    );

    if (err != 0) {
        wprintf(L"[DEBUG] ERROR: ExplorerInject failed with code %lu\n", err);
        WCHAR szError[256];
        swprintf_s(szError, 256, L"ExplorerInject failed with code %lu", err);
        MessageBoxW(NULL, szError, L"Injection Error", MB_OK | MB_ICONERROR);
        return (int)err;
    }

    wprintf(L"[DEBUG] Creating tray icon\n");
    if (!TrayInit(hInst)) {
        wprintf(L"[DEBUG] ERROR: Failed to create tray icon\n");
        MessageBoxW(NULL, L"Failed to create system tray icon", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    wprintf(L"[DEBUG] Tray icon created, entering message loop\n");

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
