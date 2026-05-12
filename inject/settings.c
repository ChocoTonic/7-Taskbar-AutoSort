#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "portable_settings.h"
#include "settings.h"

static const int DEFAULT_INTERVAL = 7;
static const WCHAR SECTION_NAME[] = L"UpdateCheck";
static const WCHAR KEY_INTERVAL[] = L"IntervalDays";
static const WCHAR KEY_LAST_CHECK[] = L"LastCheckTime";

void SettingsInit(void)
{
    PSInit(PS_REGISTRY, L"7-Taskbar-AutoSort");
}

int SettingsGetUpdateInterval(void)
{
    int interval = DEFAULT_INTERVAL;
    PSGetSingleInt(SECTION_NAME, KEY_INTERVAL, &interval);
    if (interval < 1) interval = 1;
    if (interval > 365) interval = 365;
    return interval;
}

void SettingsSetUpdateInterval(int days)
{
    if (days < 1) days = 1;
    if (days > 365) days = 365;
    PSSetSingleInt(SECTION_NAME, KEY_INTERVAL, days);
}

time_t SettingsGetLastCheckTime(void)
{
    WCHAR szVal[32] = {0};
    UINT sz = ARRAYSIZE(szVal);
    if (PSGetSingleString(SECTION_NAME, KEY_LAST_CHECK, szVal, &sz) != ERROR_SUCCESS) return 0;
    return (time_t)_wtoi64(szVal);
}

void SettingsSetLastCheckTime(time_t t)
{
    WCHAR szVal[32];
    swprintf_s(szVal, ARRAYSIZE(szVal), L"%lld", (long long)t);
    PSSetSingleString(SECTION_NAME, KEY_LAST_CHECK, szVal);
}
