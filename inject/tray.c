#include <windows.h>
#include <shellapi.h>
#include "resource.h"
#include "tray.h"
#include "update.h"
#include "settings.h"

#define WM_TRAY_CALLBACK (WM_USER + 1)
#define TRAY_CLASS_NAME L"7TaskbarAutoSort_Tray"

static HWND g_hTrayWnd = NULL;
static NOTIFYICONDATA g_nid = {0};

static INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        int interval = SettingsGetUpdateInterval();
        WCHAR szBuffer[16];
        swprintf_s(szBuffer, 16, L"%d", interval);
        SetDlgItemTextW(hDlg, IDC_DAYS, szBuffer);
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR szBuffer[16] = {0};
            GetDlgItemTextW(hDlg, IDC_DAYS, szBuffer, 16);
            int days = _wtoi(szBuffer);
            SettingsSetUpdateInterval(days);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_CHECK_UPDATE, L"Check for Updates");
    AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, L"Settings");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Exit");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

static void CheckForUpdates(HWND hWnd)
{
    WCHAR szNewVersion[64] = {0};

    if (UpdateCheckAvailable(szNewVersion, 64))
    {
        WCHAR szPrompt[256];
        swprintf_s(szPrompt, 256, L"Update available: v%s\n\nWould you like to update now?", szNewVersion);
        if (MessageBoxW(hWnd, szPrompt, L"Update Available", MB_YESNO | MB_ICONINFORMATION) == IDYES)
        {
            UpdateApply(szNewVersion);
        }
    }
    else
    {
        MessageBoxW(hWnd, L"You are running the latest version.", L"Up to Date", MB_OK | MB_ICONINFORMATION);
    }
}

static LRESULT CALLBACK TrayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TRAY_CALLBACK:
        if (lParam == WM_RBUTTONUP)
            ShowContextMenu(hWnd);
        else if (lParam == WM_LBUTTONDBLCLK)
            DialogBoxW(NULL, MAKEINTRESOURCEW(IDD_SETTINGS), hWnd, SettingsDialogProc);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_CHECK_UPDATE)
        {
            CheckForUpdates(hWnd);
            return 0;
        }
        else if (LOWORD(wParam) == IDM_SETTINGS)
        {
            DialogBoxW(NULL, MAKEINTRESOURCEW(IDD_SETTINGS), hWnd, SettingsDialogProc);
            return 0;
        }
        else if (LOWORD(wParam) == IDM_EXIT)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

BOOL TrayInit(HINSTANCE hInst)
{
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = TRAY_CLASS_NAME;

    if (!RegisterClassW(&wc))
        return FALSE;

    g_hTrayWnd = CreateWindowExW(0, TRAY_CLASS_NAME, L"7-Taskbar-AutoSort",
                                 WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                                 HWND_MESSAGE, NULL, hInst, NULL);
    if (!g_hTrayWnd)
        return FALSE;

    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hTrayWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    g_nid.uCallbackMessage = WM_TRAY_CALLBACK;
    g_nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, sizeof(g_nid.szTip) / sizeof(WCHAR), L"7-Taskbar-AutoSort");

    if (!Shell_NotifyIconW(NIM_ADD, &g_nid))
    {
        DestroyWindow(g_hTrayWnd);
        g_hTrayWnd = NULL;
        return FALSE;
    }

    return TRUE;
}

void TrayDestroy(void)
{
    if (g_hTrayWnd)
    {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        DestroyWindow(g_hTrayWnd);
        g_hTrayWnd = NULL;
    }
}

void TraySetTooltip(const WCHAR *pText)
{
    if (g_hTrayWnd && pText)
    {
        wcscpy_s(g_nid.szTip, sizeof(g_nid.szTip) / sizeof(WCHAR), pText);
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    }
}

HWND TrayGetWindow(void)
{
    return g_hTrayWnd;
}
