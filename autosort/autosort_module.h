// autosort_module.h — auto-sort polling thread API.
// Linked into the vendored 7TT DLL build; called from the (modified) dll.c.
#pragma once

#include <windows.h>

// Spawn the polling thread. Call after WndProcInit succeeds.
void AutosortInit(void);

// Signal stop and join the polling thread (bounded wait). Call in the WaitThread
// cleanup path, before WndProcExit, so the polling thread won't try to send
// messages through a tearing-down wnd_proc.
void AutosortShutdown(void);
