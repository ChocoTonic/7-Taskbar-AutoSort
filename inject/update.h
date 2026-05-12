#pragma once
#include "json_util.h"

BOOL  UpdateCheckAvailable(WCHAR *pNewVersion, int cchMax);
void  UpdateApply(const WCHAR *pNewVersion);
BOOL  UpdatePromptIfAvailable(HWND hParent);
