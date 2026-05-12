#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "autosort_version.h"
#include "explorer_inject.h"
#include "json_util.h"
#include "update.h"
#include "version_compare.h"

#pragma comment(lib, "winhttp.lib")

static BOOL FetchLatestVersion(WCHAR *pVersionOut, int cchMax)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pResponse = NULL;
    BOOL bSuccess = FALSE;

    hSession = WinHttpOpen(L"7-Taskbar-AutoSort/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS,
                           0);
    if (!hSession) goto cleanup;

    hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) goto cleanup;

    hRequest = WinHttpOpenRequest(hConnect,
                                  L"GET",
                                  L"/repos/ChocoTonic/7-Taskbar-AutoSort/releases/latest",
                                  L"HTTP/1.1",
                                  WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES,
                                  WINHTTP_FLAG_SECURE);
    if (!hRequest) goto cleanup;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0)) goto cleanup;

    if (!WinHttpReceiveResponse(hRequest, NULL)) goto cleanup;

    pResponse = (LPSTR)malloc(8192);
    if (!pResponse) goto cleanup;

    while (WinHttpReadData(hRequest, (LPVOID)(pResponse + dwDownloaded), 8192 - dwDownloaded, &dwSize))
    {
        if (dwSize == 0) break;
        dwDownloaded += dwSize;
        if (dwDownloaded > 8000) break;
    }

    if (dwDownloaded > 0)
    {
        pResponse[dwDownloaded] = '\0';
        bSuccess = ParseVersionFromJson(pResponse, pVersionOut, cchMax);
    }

cleanup:
    if (pResponse) free(pResponse);
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return bSuccess;
}

BOOL UpdateCheckAvailable(WCHAR *pNewVersion, int cchMax)
{
    WCHAR szLatestVersion[64] = {0};

    if (!FetchLatestVersion(szLatestVersion, 64)) return FALSE;

    wcscpy_s(pNewVersion, cchMax, AUTOSORT_VERSION_STR);
    if (VersionCompare(AUTOSORT_VERSION_STR, szLatestVersion))
    {
        wcscpy_s(pNewVersion, cchMax, szLatestVersion);
        return TRUE;
    }

    return FALSE;
}

static BOOL DownloadFile(const WCHAR *pUrl, const WCHAR *pFilePath)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    HANDLE hFile = NULL;
    DWORD dwWritten = 0;
    BOOL bSuccess = FALSE;
    BYTE buffer[4096];

    hFile = CreateFileW(pFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) goto cleanup;

    hSession = WinHttpOpen(L"7-Taskbar-AutoSort/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS,
                           0);
    if (!hSession) goto cleanup;

    hConnect = WinHttpConnect(hSession, L"github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) goto cleanup;

    hRequest = WinHttpOpenRequest(
        hConnect, L"GET", pUrl, L"HTTP/1.1", WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) goto cleanup;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0)) goto cleanup;

    if (!WinHttpReceiveResponse(hRequest, NULL)) goto cleanup;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &dwSize))
    {
        if (dwSize == 0) break;
        if (!WriteFile(hFile, buffer, dwSize, &dwWritten, NULL) || dwWritten != dwSize) goto cleanup;
    }

    bSuccess = TRUE;

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

    return bSuccess;
}

void UpdateApply(const WCHAR *pNewVersion)
{
    WCHAR szExePath[MAX_PATH];
    WCHAR szNewPath[MAX_PATH];
    WCHAR szOldPath[MAX_PATH];
    WCHAR szDownloadUrl[MAX_PATH];

    GetModuleFileNameW(NULL, szExePath, MAX_PATH);

    wcscpy_s(szNewPath, MAX_PATH, szExePath);
    wcscat_s(szNewPath, MAX_PATH, L".new");

    wcscpy_s(szOldPath, MAX_PATH, szExePath);
    wcscat_s(szOldPath, MAX_PATH, L".old");

    swprintf_s(szDownloadUrl,
               MAX_PATH,
               L"/ChocoTonic/7-Taskbar-AutoSort/releases/download/v%s/7-Taskbar-AutoSort.exe",
               pNewVersion);

    ExplorerCleanup();

    if (DownloadFile(szDownloadUrl, szNewPath))
    {
        DeleteFileW(szOldPath);

        MoveFileExW(szExePath, szOldPath, MOVEFILE_REPLACE_EXISTING);
        MoveFileExW(szNewPath, szExePath, MOVEFILE_REPLACE_EXISTING);

        MoveFileExW(szOldPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

        STARTUPINFOW si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);

        if (CreateProcessW(szExePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    ExitProcess(0);
}

BOOL UpdatePromptIfAvailable(HWND hParent)
{
    WCHAR szNewVersion[64] = {0};
    if (!UpdateCheckAvailable(szNewVersion, 64)) return FALSE;

    WCHAR szPrompt[256];
    swprintf_s(szPrompt, 256, L"Update available: v%s\n\nWould you like to update now?", szNewVersion);

    if (MessageBoxW(hParent, szPrompt, L"Update Available", MB_YESNO | MB_ICONINFORMATION) == IDYES)
    {
        UpdateApply(szNewVersion);
    }
    return TRUE;
}
