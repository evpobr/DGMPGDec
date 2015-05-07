#pragma once

#include <stdio.h>

void InitialSRC(void);
void Wavefs44(FILE *file, int size, unsigned char *buffer);
void EndSRC(FILE *file);
void StartWAV(FILE *file, unsigned char format);
void CloseWAV(FILE *file, int size);
void DownWAV(FILE *file);
bool CheckWAV(void);
