// autosort_module.c — periodically sort the windows inside every taskbar app
// group alphabetically by title.
//
// Runs as a thread inside explorer.exe. The thread polls on a fixed interval;
// on each tick it asks the taskbar UI thread (via 7TT's MSG_DLL_CALLFUNC_PARAM
// SendMessage shim) to walk every grouped button group and call the existing
// SortButtonGroupItems routine.
//
// All Explorer-internals work is reused from 7TT verbatim — EV_*, DO2, the sort
// itself, the marshal-to-taskbar-thread mechanism. We add only the timer.

#include "stdafx.h"
#include "autosort_module.h"
#include "functions.h"     // DO2, GetButtonWnd, etc.
#include "explorer_vars.h" // EV_MM_TASKLIST_BUTTON_GROUPS_HDPA
#include "wnd_proc.h"      // MSG_DLL_CALLFUNC_PARAM, CALLFUCN_PARAM

// 7TT globals defined in dll.c.
extern HWND hTaskbarWnd;
extern LONG_PTR lpTaskListLongPtr;
extern UINT uTweakerMsg;

#define AUTOSORT_POLL_INTERVAL_MS 2000

static HANDLE g_hStopEvent = NULL;
static HANDLE g_hPollThread = NULL;

// Runs on the taskbar UI thread (via the MSG_DLL_CALLFUNC_PARAM dispatch in
// wnd_proc.c). Safe to read Explorer's HDPA and call SortButtonGroupItems
// directly here.
static LONG_PTR AutosortAllGroupsOnTaskbarThread(LONG_PTR lpMMTaskListLongPtr)
{
    HDPA *pphdpa = EV_MM_TASKLIST_BUTTON_GROUPS_HDPA(lpMMTaskListLongPtr);
    if (!pphdpa) return 0;

    LONG_PTR *plp = (LONG_PTR *)*pphdpa;
    if (!plp) return 0;

    int button_groups_count = (int)plp[0];
    LONG_PTR **button_groups = (LONG_PTR **)plp[1];

    for (int i = 0; i < button_groups_count; i++)
    {
        if (!button_groups[i]) continue;
        int type = (int)button_groups[i][DO2(6, 8)];
        if (type == 1 || type == 3) SortButtonGroupItems(button_groups[i]);
    }

    return 0;
}

static DWORD WINAPI AutosortPollThreadProc(LPVOID p)
{
    (void)p;

    for (;;)
    {
        DWORD wait = WaitForSingleObject(g_hStopEvent, AUTOSORT_POLL_INTERVAL_MS);
        if (wait == WAIT_OBJECT_0) break;

        // Bail if 7TT's bootstrap hasn't populated the globals yet, or if
        // the taskbar window has gone away (e.g., explorer restart in progress).
        if (!hTaskbarWnd || !lpTaskListLongPtr || !uTweakerMsg) continue;
        if (!IsWindow(hTaskbarWnd)) continue;

        CALLFUCN_PARAM call;
        call.pFunction = AutosortAllGroupsOnTaskbarThread;
        call.pParam = (void *)lpTaskListLongPtr;
        SendMessage(hTaskbarWnd, uTweakerMsg, (WPARAM)&call, MSG_DLL_CALLFUNC_PARAM);
    }

    return 0;
}

void AutosortInit(void)
{
    if (g_hStopEvent) return; // already initialized

    g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hStopEvent) return;

    g_hPollThread = CreateThread(NULL, 0, AutosortPollThreadProc, NULL, 0, NULL);
    if (!g_hPollThread)
    {
        CloseHandle(g_hStopEvent);
        g_hStopEvent = NULL;
    }
}

void AutosortShutdown(void)
{
    if (g_hStopEvent) SetEvent(g_hStopEvent);
    if (g_hPollThread)
    {
        WaitForSingleObject(g_hPollThread, 5000);
        CloseHandle(g_hPollThread);
        g_hPollThread = NULL;
    }
    if (g_hStopEvent)
    {
        CloseHandle(g_hStopEvent);
        g_hStopEvent = NULL;
    }
}
