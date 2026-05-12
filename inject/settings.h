#pragma once

#include <time.h>

void SettingsInit(void);
int SettingsGetUpdateInterval(void);
void SettingsSetUpdateInterval(int days);
time_t SettingsGetLastCheckTime(void);
void SettingsSetLastCheckTime(time_t t);
int SettingsGetPollIntervalSec(void);
void SettingsSetPollIntervalSec(int seconds);
