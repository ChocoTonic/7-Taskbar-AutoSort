#include <windows.h>
#include "cJSON.h"
#include "json_util.h"

BOOL ParseVersionFromJson(const char *pJson, WCHAR *pOut, int cchMax)
{
    if (!pJson || !pOut || cchMax <= 0) return FALSE;

    cJSON *pRoot = cJSON_Parse(pJson);
    if (!pRoot) return FALSE;

    BOOL bSuccess = FALSE;
    const cJSON *pTag = cJSON_GetObjectItemCaseSensitive(pRoot, "tag_name");
    if (cJSON_IsString(pTag) && pTag->valuestring)
    {
        const char *pVersion = pTag->valuestring;
        if (pVersion[0] == 'v') pVersion++;
        MultiByteToWideChar(CP_UTF8, 0, pVersion, -1, pOut, cchMax);
        bSuccess = TRUE;
    }

    cJSON_Delete(pRoot);
    return bSuccess;
}
