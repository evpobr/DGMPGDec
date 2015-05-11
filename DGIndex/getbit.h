/* 
 *  Mutated into DGIndex. Modifications Copyright (C) 2004, Donald Graft
 * 
 *	Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *	
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#pragma once

#ifdef GETBIT_GLOBAL
#define GXTN
#else
#define GXTN extern
#endif

#define SUB_SUB						0x20
#define SUB_AC3						0x80
#define SUB_DTS						0x88
#define SUB_PCM						0xA0

/* extension start code IDs */
#define SEQUENCE_EXTENSION_ID					1
#define SEQUENCE_DISPLAY_EXTENSION_ID			2
#define QUANT_MATRIX_EXTENSION_ID				3
#define COPYRIGHT_EXTENSION_ID					4
#define PICTURE_DISPLAY_EXTENSION_ID			7
#define PICTURE_CODING_EXTENSION_ID				8

#define ZIG_ZAG									0
#define MB_WEIGHT								32
#define MB_CLASS4								64

#define I_TYPE			1
#define P_TYPE			2
#define B_TYPE			3
#define D_TYPE			4

#define MACROBLOCK_INTRA				1
#define MACROBLOCK_PATTERN				2
#define MACROBLOCK_MOTION_BACKWARD		4
#define MACROBLOCK_MOTION_FORWARD		8
#define MACROBLOCK_QUANT				16

#define TOP_FIELD		1
#define BOTTOM_FIELD	2
#define FRAME_PICTURE	3

#define MC_FIELD		1
#define MC_FRAME		2
#define MC_16X8			2
#define MC_DMV			3

#define MV_FIELD		0
#define MV_FRAME		1

#define CHROMA420		1
#define CHROMA422		2
#define CHROMA444		3

#define ELEMENTARY_STREAM 0
#define MPEG1_PROGRAM_STREAM 1
#define MPEG2_PROGRAM_STREAM 2

void Initialize_Buffer();
void Fill_Buffer();
void Next_Packet();
void Flush_Buffer_All(unsigned int N);
unsigned int Get_Bits_All(unsigned int N);
void Next_File(void);

GXTN unsigned char *Rdbfr, *Rdptr, *Rdmax;
GXTN unsigned int BitsLeft, CurrentBfr, NextBfr, Val, Read;
GXTN __int64 CurrentPackHeaderPosition;

void next_start_code();

__forceinline static unsigned int Show_Bits(unsigned int N)
{
	if (N <= BitsLeft)
	{
		return (CurrentBfr << (32 - BitsLeft)) >> (32 - N);
	}
	else
	{
		N -= BitsLeft;
		return (((CurrentBfr << (32 - BitsLeft)) >> (32 - BitsLeft)) << N) + (NextBfr >> (32 - N));
	}
}

__forceinline static unsigned int Get_Bits(unsigned int N)
{
	if (N < BitsLeft)
	{
		Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - N);
		BitsLeft -= N;
		return Val;
	}
	else
		return Get_Bits_All(N);
}

__forceinline static void Flush_Buffer(unsigned int N)
{
	if (N < BitsLeft)
		BitsLeft -= N;
	else
		Flush_Buffer_All(N);	
}

int _donread(int fd, void *buffer, unsigned int count);

