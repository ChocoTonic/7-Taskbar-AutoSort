#pragma once

BOOL TrayInit(HINSTANCE hInst);
void TrayDestroy(void);
void TraySetTooltip(const WCHAR *pText);
HWND TrayGetWindow(void);
