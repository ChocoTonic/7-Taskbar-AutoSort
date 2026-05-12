#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

    BOOL ParseVersionFromJson(const char *pJson, WCHAR *pOut, int cchMax);

#ifdef __cplusplus
}
#endif
