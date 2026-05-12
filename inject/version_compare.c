#include <windows.h>
#include <wchar.h>
#include "version_compare.h"

BOOL VersionCompare(const WCHAR *pCurrent, const WCHAR *pNew)
{
    int curr_major = 0, curr_minor = 0, curr_patch = 0;
    int new_major = 0, new_minor = 0, new_patch = 0;

    if (swscanf_s(pCurrent, L"%d.%d.%d", &curr_major, &curr_minor, &curr_patch) != 3)
        curr_major = curr_minor = curr_patch = 0;
    if (swscanf_s(pNew, L"%d.%d.%d", &new_major, &new_minor, &new_patch) != 3) new_major = new_minor = new_patch = 0;

    if (new_major > curr_major) return TRUE;
    if (new_major < curr_major) return FALSE;
    if (new_minor > curr_minor) return TRUE;
    if (new_minor < curr_minor) return FALSE;
    if (new_patch > curr_patch) return TRUE;
    return FALSE;
}
