#pragma once

#include <Windows.h>

void UpdateWindowText();
void UpdateMRUList();
void AddMRUList(TCHAR *);
void DeleteMRUList(int);
void ResizeWindow(int width, int height);

void Recovery(void);
void RefreshWindow(bool);
void CheckFlag(void);
int parse_cli(LPTSTR lpCmdLine, LPTSTR ucCmdLine);

