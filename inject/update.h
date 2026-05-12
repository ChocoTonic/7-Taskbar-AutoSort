#pragma once

BOOL  UpdateCheckAvailable(WCHAR *pNewVersion, int cchMax);
void  UpdateApply(const WCHAR *pNewVersion);
