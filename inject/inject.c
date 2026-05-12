#include <windows.h>
#include <time.h>
#include "explorer_inject.h"
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

    if (!ExtractDll()) return 1;

    DWORD err = ExplorerInject(NULL, WM_APP, GetUserDefaultUILanguage(), opts, NULL);
    if (err != 0) return (int)err;

    if (!TrayInit(hInst)) return 1;

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
