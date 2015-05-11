#pragma once

#include <Windows.h>
#include "DGIndex_h.h"

#define MAX_WINDOW_WIDTH 800
#define MAX_WINDOW_HEIGHT 600

// Messages to the window procedure.
#define CLI_RIP_MESSAGE				(WM_APP)
#define D2V_DONE_MESSAGE			(WM_APP + 1)
#define CLI_PREVIEW_DONE_MESSAGE	(WM_APP + 2)
#define PROGRESS_MESSAGE			(WM_APP + 3)

void UpdateWindowText();
void UpdateMRUList();
void AddMRUList(_In_reads_z_(DG_MAX_PATH) LPCTSTR);
void DeleteMRUList(_In_ int);
void ResizeWindow(_In_ int width, _In_ int height);

void PrepareOpenFileFilter(_Inout_updates_z_(DG_MAX_PATH) LPTSTR pszFilter);

void Recovery(void);
void RefreshWindow(bool);
void CheckFlag(void);
int parse_cli(LPTSTR lpCmdLine, LPTSTR ucCmdLine);

