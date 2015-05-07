#pragma once

#include <tchar.h>
#include <stdio.h>

void Normalize(FILE *WaveIn, int WaveInPos, TCHAR *filename, FILE *WaveOut, int WaveOutPos, int size);
