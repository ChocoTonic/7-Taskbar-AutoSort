#pragma once

BOOL TrayInit(HINSTANCE hInst);
void TrayDestroy(void);
void TraySetTooltip(const WCHAR *pText);
void TrayShowBalloon(const WCHAR *pTitle, const WCHAR *pMsg);
HWND TrayGetWindow(void);
