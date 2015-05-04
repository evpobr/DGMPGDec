/*
 *  Avisynth 2.5 API for MPEG2Dec3
 *
 *  Changes to fix frame dropping and random frame access are
 *  Copyright (C) 2003-2008 Donald A. Graft
 *
 *	Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *	based of the intial MPEG2Dec Avisytnh API Copyright (C) Mathias Born - May 2001
 *
 *  This file is part of DGMPGDec, a free MPEG-2 decoder
 *	
 *  DGMPGDec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DGMPGDec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "postprocess.h"
#include "AvisynthAPI.h"
#include "utilities.h"
#include <string.h>

#define VERSION "DGDecode 1.5.8"

MPEG2Source::MPEG2Source(const char* d2v, int cpu, int idct, int iPP, int moderate_h, int moderate_v, bool showQ, bool fastMC, const char* _cpu2, int _info, int _upConv, bool _i420, int iCC, IScriptEnvironment* env)
{
	int status;

	CheckCPU();

	#ifdef PROFILING
		init_first(&tim);
	#endif

	/* override */
	m_decoder.refinit = false;
	m_decoder.fpuinit = false;
	m_decoder.luminit = false;

	if (iPP != -1 && iPP != 0 && iPP != 1)
		env->ThrowError("MPEG2Source: iPP must be set to -1, 0, or 1!");

	if (iCC != -1 && iCC != 0 && iCC != 1)
		env->ThrowError("MPEG2Source: iCC must be set to -1, 0, or 1!");

	if (_upConv != 0 && _upConv != 1 && _upConv != 2)
		env->ThrowError("MPEG2Source: upConv must be set to 0, 1, or 2!");

	ovr_idct = idct;
	m_decoder.iPP = iPP;
	m_decoder.iCC = iCC;
	m_decoder.info = _info;
	m_decoder.showQ = showQ;
	m_decoder.fastMC = fastMC;
	m_decoder.upConv = _upConv;
	m_decoder.i420 = _i420;
	m_decoder.moderate_h = moderate_h;
	m_decoder.moderate_v = moderate_v;
	m_decoder.maxquant = m_decoder.minquant = m_decoder.avgquant = 0;

	if (ovr_idct > 7) 
	{
		env->ThrowError("MPEG2Source : iDCT invalid (1:MMX,2:SSEMMX,3:SSE2,4:FPU,5:REF,6:Skal's,7:Simple)");
	}

	const char *path = d2v;

	FILE *f;
	if (fopen_s(&f, path,"r"))
		env->ThrowError("MPEG2Source : unable to load D2V file \"%s\" ",path);
	
	fclose(f);

	status = m_decoder.Open(path);
    if (status != 0 && m_decoder.VF_File)
    {
        fclose(m_decoder.VF_File);
        m_decoder.VF_File = NULL;
    }
	if (status == 1)
		env->ThrowError("MPEG2Source: Invalid D2V file, it's empty!");
	else if (status == 2)
		env->ThrowError("MPEG2Source: DGIndex/DGDecode mismatch. You are picking up\n"
		                "a version of DGDecode, possibly from your plugins directory,\n"
						"that does not match the version of DGIndex used to make the D2V\n"
						"file. Search your hard disk for all copies of DGDecode.dll\n"
						"and delete or rename all of them except for the one that\n"
						"has the same version number as the DGIndex.exe that was used\n"
						"to make the D2V file.");
	else if (status == 3)
		env->ThrowError("MPEG2Source: Could not open one of the input files.");
	else if (status == 4)
		env->ThrowError("MPEG2Source: Could not find a sequence header in the input stream.");
	else if (status == 5)
		env->ThrowError("MPEG2Source: The input file is not a D2V project file.");
	else if (status == 6)
		env->ThrowError("MPEG2Source: Force Film mode not supported for frame repeats.");

	memset(&vi, 0, sizeof(vi));
	vi.width = m_decoder.Clip_Width;
	vi.height = m_decoder.Clip_Height;
	if (m_decoder.chroma_format == 1 && _upConv == 0)
	{
		if (m_decoder.i420 == true)
			vi.pixel_type = VideoInfo::CS_I420;
		else
			vi.pixel_type = VideoInfo::CS_YV12;
	}
	else if (m_decoder.chroma_format == 1 && _upConv == 1) vi.pixel_type = VideoInfo::CS_YUY2;
	else if (m_decoder.chroma_format == 1 && _upConv == 2) vi.pixel_type = VideoInfo::CS_BGR24;
	else if (m_decoder.chroma_format == 2 && _upConv != 2) vi.pixel_type = VideoInfo::CS_YUY2;
	else if (m_decoder.chroma_format == 2 && _upConv == 2) vi.pixel_type = VideoInfo::CS_BGR24;
	else env->ThrowError("MPEG2Source:  currently unsupported input color format (4:4:4)");
	vi.fps_numerator = m_decoder.VF_FrameRate_Num;
	vi.fps_denominator = m_decoder.VF_FrameRate_Den;
	vi.num_frames = m_decoder.VF_FrameLimit;
	vi.SetFieldBased(false);

	switch (cpu) {
		case 0 : _PP_MODE = 0; break;
		case 1 : _PP_MODE = PP_DEBLOCK_Y_H; break;
		case 2 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
		case 3 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
		case 4 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
		case 5 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
		case 6 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
		default : env->ThrowError("MPEG2Source : cpu level invalid (0 to 6)");
	}

	const char* cpu2 = _cpu2;
	if (strlen(cpu2)==6) {
		_PP_MODE = 0;
		if (cpu2[0]=='x' || cpu2[0] == 'X') { _PP_MODE |= PP_DEBLOCK_Y_H; }
		if (cpu2[1]=='x' || cpu2[1] == 'X') { _PP_MODE |= PP_DEBLOCK_Y_V; }
		if (cpu2[2]=='x' || cpu2[2] == 'X') { _PP_MODE |= PP_DEBLOCK_C_H; }
		if (cpu2[3]=='x' || cpu2[3] == 'X') { _PP_MODE |= PP_DEBLOCK_C_V; }
		if (cpu2[4]=='x' || cpu2[4] == 'X') { _PP_MODE |= PP_DERING_Y; }
		if (cpu2[5]=='x' || cpu2[5] == 'X') { _PP_MODE |= PP_DERING_C; }
	}

	if ( ovr_idct != m_decoder.IDCT_Flag && ovr_idct > 0 )
	{
		dprintf("Overiding iDCT With: %d", ovr_idct);
		override(ovr_idct);
	}

	out = NULL;
	out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);
	if (out == NULL) env->ThrowError("MPEG2Source:  malloc failure (yv12 pic out)!");

	m_decoder.AVSenv = env;
	m_decoder.pp_mode = _PP_MODE;

	bufY = bufU = bufV = NULL;
	if (m_decoder.chroma_format != 1 || (m_decoder.chroma_format == 1 && _upConv > 0))
	{
		bufY = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,32);
		bufU = (unsigned char*)aligned_malloc(m_decoder.Chroma_Width*vi.height+2048,32);
		bufV =  (unsigned char*)aligned_malloc(m_decoder.Chroma_Width*vi.height+2048,32);
		if (bufY == NULL || bufU == NULL || bufV == NULL)
			env->ThrowError("MPEG2Source:  malloc failure (bufY, bufU, bufV)!");
	}

	u444 = v444 = NULL;
	if (_upConv == 2)
	{
		u444 = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,32);
		v444 = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,32);
		if (u444 == NULL || v444 == NULL)
			env->ThrowError("MPEG2Source:  malloc failure (u444, v444)!");
	}
}

MPEG2Source::~MPEG2Source()
{
	m_decoder.Close();
	if (out != NULL) { aligned_free(out); out = NULL; }
	if (bufY != NULL) { aligned_free(bufY); bufY = NULL; }
	if (bufU != NULL) { aligned_free(bufU); bufU = NULL; }
	if (bufV != NULL) { aligned_free(bufV); bufV = NULL; }
	if (u444 != NULL) { aligned_free(u444); u444 = NULL; }
	if (v444 != NULL) { aligned_free(v444); v444 = NULL; }
}

bool __stdcall MPEG2Source::GetParity(int)
{
	return m_decoder.Field_Order == 1;
}

void MPEG2Source::override(int ovr_idct) 
{
	if (ovr_idct > 0)
		m_decoder.IDCT_Flag = ovr_idct;

	switch (m_decoder.IDCT_Flag)
	{
		case IDCT_MMX:
			m_decoder.idctFunc = MMX_IDCT;
			break;

		case IDCT_SSEMMX:
			m_decoder.idctFunc = SSEMMX_IDCT;
			if (!cpu.ssemmx)
			{
				m_decoder.IDCT_Flag = IDCT_MMX;
				m_decoder.idctFunc = MMX_IDCT;
			}
			break;

		case IDCT_FPU:
			if (!m_decoder.fpuinit) 
			{
				Initialize_FPU_IDCT();
				m_decoder.fpuinit = true;
			}
			m_decoder.idctFunc = FPU_IDCT;
			break;

		case IDCT_REF:
			m_decoder.refinit = true;
			m_decoder.idctFunc = REF_IDCT;
			break;

		case IDCT_SSE2MMX:
			m_decoder.idctFunc = SSE2MMX_IDCT;
			if (!cpu.sse2mmx)
			{
				m_decoder.IDCT_Flag = IDCT_SSEMMX;
				m_decoder.idctFunc = SSEMMX_IDCT;
				if (!cpu.ssemmx)
				{
					m_decoder.IDCT_Flag = IDCT_MMX;
					m_decoder.idctFunc = MMX_IDCT;
				}
			}
			break;
		
		case IDCT_SKALSSE:
			m_decoder.idctFunc = Skl_IDct16_Sparse_SSE; //Skl_IDct16_SSE;
			if (!cpu.ssemmx)
			{
				m_decoder.IDCT_Flag = IDCT_MMX;
				m_decoder.idctFunc = MMX_IDCT;
			}
			break;

		case IDCT_SIMPLEIDCT:
			m_decoder.idctFunc = simple_idct_mmx;
			if (!cpu.ssemmx)
			{
				m_decoder.IDCT_Flag = IDCT_MMX;
				m_decoder.idctFunc = MMX_IDCT;
			}
			break;
	}
}

PVideoFrame __stdcall MPEG2Source::GetFrame(int n, IScriptEnvironment* env)
{
	int gop, pct;
	char Matrix_s[40];
	unsigned int raw;
	unsigned int hint;

	if (m_decoder.info != 0 || m_decoder.upConv == 2)
	{
		raw = max(m_decoder.FrameList[n].bottom, m_decoder.FrameList[n].top);
		if (raw < (int)m_decoder.BadStartingFrames) raw = m_decoder.BadStartingFrames;

		for (gop = 0; gop < (int) m_decoder.VF_GOPLimit - 1; gop++)
		{
			if (raw >= (int)m_decoder.GOPList[gop]->number && raw < (int)m_decoder.GOPList[gop+1]->number)
			{
				break;
			}
		}
	}

    PVideoFrame frame = env->NewVideoFrame(vi);

	if (m_decoder.chroma_format == 1 && m_decoder.upConv == 0) // YV12
	{
		out->y = frame->GetWritePtr(PLANAR_Y);
		out->u = frame->GetWritePtr(PLANAR_U);
		out->v = frame->GetWritePtr(PLANAR_V);
		out->ypitch = frame->GetPitch(PLANAR_Y);
		out->uvpitch = frame->GetPitch(PLANAR_U);
		out->ywidth = frame->GetRowSize(PLANAR_Y);
		out->uvwidth = frame->GetRowSize(PLANAR_U);
		out->yheight = frame->GetHeight(PLANAR_Y);
		out->uvheight = frame->GetHeight(PLANAR_V);
	}
	else // its 4:2:2, pass our own buffers
	{
		out->y = bufY;
		out->u = bufU;
		out->v = bufV;
		out->ypitch = vi.width;
		out->uvpitch = m_decoder.Chroma_Width;
		out->ywidth = vi.width;
		out->uvwidth = m_decoder.Chroma_Width;
		out->yheight = vi.height;
		out->uvheight = vi.height;
	}

#ifdef PROFILING
	init_timers(&tim);
	start_timer2(&tim.overall);
#endif

	m_decoder.Decode(n, out);

	if ( m_decoder.Luminance_Flag )
		m_decoder.LuminanceFilter(out->y, out->ywidth, out->yheight, out->ypitch);

#ifdef PROFILING
	stop_timer2(&tim.overall);
	tim.sum += tim.overall;
	tim.div++;
	timer_debug(&tim);
#endif

	__asm emms;

	if ((m_decoder.chroma_format == 2 && m_decoder.upConv != 2) ||
		(m_decoder.chroma_format == 1 && m_decoder.upConv == 1)) // convert 4:2:2 (planar) to YUY2 (packed)
	{
		conv422toYUV422(out->y,out->u,out->v,frame->GetWritePtr(),out->ypitch,out->uvpitch,
			frame->GetPitch(),vi.width,vi.height);
	}

	if (m_decoder.upConv == 2) // convert 4:2:2 (planar) to 4:4:4 (planar) and then to RGB24
	{
		conv422to444(out->u,u444,out->uvpitch,vi.width,vi.width,vi.height);
		conv422to444(out->v,v444,out->uvpitch,vi.width,vi.width,vi.height);
		PVideoFrame frame2 = env->NewVideoFrame(vi);
		conv444toRGB24(out->y,u444,v444,frame2->GetWritePtr(),out->ypitch,vi.width,
			frame2->GetPitch(),vi.width,vi.height,m_decoder.GOPList[gop]->matrix,m_decoder.pc_scale);
		env->BitBlt(frame->GetWritePtr(), frame->GetPitch(), 
			frame2->GetReadPtr() + (vi.height-1) * frame2->GetPitch(), -frame2->GetPitch(), 
			frame->GetRowSize(), frame->GetHeight());
	}

	if (m_decoder.info != 0)
	{
		pct = m_decoder.FrameList[raw].pct == I_TYPE ? 'I' : m_decoder.FrameList[raw].pct == B_TYPE ? 'B' : 'P';

		switch (m_decoder.GOPList[gop]->matrix)
		{
		case 0:
			strcpy_s(Matrix_s, "Forbidden");
			break;
		case 1:
			strcpy_s(Matrix_s, "ITU-R BT.709");
			break;
		case 2:
			strcpy_s(Matrix_s, "Unspecified Video");
			break;
		case 3:
			strcpy_s(Matrix_s, "Reserved");
			break;
		case 4:
			strcpy_s(Matrix_s, "FCC");
			break;
		case 5:
			strcpy_s(Matrix_s, "ITU-R BT.470-2 System B, G");
			break;
		case 6:
			strcpy_s(Matrix_s, "SMPTE 170M");
			break;
		case 7:
			strcpy_s(Matrix_s, "SMPTE 240M (1987)");
			break;
		default:
			strcpy_s(Matrix_s, "Reserved");
			break;
		}
	}

	if (m_decoder.info == 1)
	{
		char msg1[1024];
		sprintf_s(msg1,"%s (c) 2007 Donald A. Graft (et al)\n" \
					 "---------------------------------------\n" \
					 "Source:        %s\n" \
					 "Frame Rate:    %3.6f fps (%u/%u) %s\n" \
			         "Field Order:   %s\n" \
			         "Picture Size:  %d x %d\n" \
 					 "Aspect Ratio:  %s\n"  \
					 "Progr Seq:     %s\n" \
					 "GOP Number:    %d (%d)  GOP Pos = %I64d\n" \
					 "Closed GOP:    %s\n" \
					 "Display Frame: %d\n" \
					 "Encoded Frame: %d (top) %d (bottom)\n" \
 					 "Frame Type:    %c (%d)\n" \
					 "Progr Frame:   %s\n" \
 					 "Colorimetry:   %s (%d)\n" \
 					 "Quants:        %d/%d/%d (avg/min/max)\n",
		VERSION,
 		m_decoder.Infilename[m_decoder.GOPList[gop]->file],
		double(m_decoder.VF_FrameRate_Num) / double(m_decoder.VF_FrameRate_Den),
			m_decoder.VF_FrameRate_Num,
			m_decoder.VF_FrameRate_Den,
			(m_decoder.VF_FrameRate == 25000 || m_decoder.VF_FrameRate == 50000) ? "(PAL)" :
			m_decoder.VF_FrameRate == 29970 ? "(NTSC)" : "",
		m_decoder.Field_Order == 1 ? "Top Field First" : "Bottom Field First",
		m_decoder.Coded_Picture_Width, m_decoder.Coded_Picture_Height,
		m_decoder.Aspect_Ratio,
		m_decoder.GOPList[gop]->progressive ? "True" : "False",
		gop, m_decoder.GOPList[gop]->number, m_decoder.GOPList[gop]->position,
		m_decoder.GOPList[gop]->closed ? "True" : "False",
		n,
		m_decoder.FrameList[n].top, m_decoder.FrameList[n].bottom,
		pct, raw,
		m_decoder.FrameList[raw].pf ? "True" : "False",
		Matrix_s, m_decoder.GOPList[gop]->matrix,
		m_decoder.avgquant, m_decoder.minquant, m_decoder.maxquant);
		ApplyMessage(&frame, vi, msg1, 150, 0xdfffbf, 0x0, 0x0, env);
	}
	else if (m_decoder.info == 2)
	{
		dprintf("DGDecode: DGDecode %s (c) 2005 Donald A. Graft\n", VERSION);
		dprintf("DGDecode: Source:            %s\n", m_decoder.Infilename[m_decoder.GOPList[gop]->file]);
		dprintf("DGDecode: Frame Rate:        %3.6f fps (%u/%u) %s\n",
			double(m_decoder.VF_FrameRate_Num) / double(m_decoder.VF_FrameRate_Den),
			m_decoder.VF_FrameRate_Num, m_decoder.VF_FrameRate_Den,
			(m_decoder.VF_FrameRate == 25000 || m_decoder.VF_FrameRate == 50000) ? "(PAL)" : m_decoder.VF_FrameRate == 29970 ? "(NTSC)" : "");
		dprintf("DGDecode: Field Order:       %s\n", m_decoder.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
		dprintf("DGDecode: Picture Size:      %d x %d\n", m_decoder.Coded_Picture_Width, m_decoder.Coded_Picture_Height);
		dprintf("DGDecode: Aspect Ratio:      %s\n", m_decoder.Aspect_Ratio);
		dprintf("DGDecode: Progressive Seq:   %s\n", m_decoder.GOPList[gop]->progressive ? "True" : "False");
		dprintf("DGDecode: GOP Number:        %d (%d)  GOP Pos = %I64d\n", gop, m_decoder.GOPList[gop]->number, m_decoder.GOPList[gop]->position);
		dprintf("DGDecode: Closed GOP:        %s\n", m_decoder.GOPList[gop]->closed ? "True" : "False");
		dprintf("DGDecode: Display Frame:     %d\n", n);
		dprintf("DGDecode: Encoded Frame:     %d (top) %d (bottom)\n", m_decoder.FrameList[n].top, m_decoder.FrameList[n].bottom);
		dprintf("DGDecode: Frame Type:        %c (%d)\n", pct, raw);
		dprintf("DGDecode: Progressive Frame: %s\n", m_decoder.FrameList[raw].pf ? "True" : "False");
		dprintf("DGDecode: Colorimetry:       %s (%d)\n", Matrix_s, m_decoder.GOPList[gop]->matrix);
		dprintf("DGDecode: Quants:            %d/%d/%d (avg/min/max)\n", m_decoder.avgquant, m_decoder.minquant, m_decoder.maxquant);
	}
	else if (m_decoder.info == 3)
	{
		hint = 0;
		if (m_decoder.FrameList[raw].pf == 1) hint |= PROGRESSIVE;
		hint |= ((m_decoder.GOPList[gop]->matrix & 7) << COLORIMETRY_SHIFT);
		PutHintingData(frame->GetWritePtr(PLANAR_Y), hint);
	}

	return frame;
}

int MPEG2Source::getMatrix(int n)
{
	int raw = max(m_decoder.FrameList[n].bottom, m_decoder.FrameList[n].top);
	if (raw < (int)m_decoder.BadStartingFrames) raw = m_decoder.BadStartingFrames;

	for (int gop = 0; gop < (int) m_decoder.VF_GOPLimit - 1; gop++)
	{
		if (raw >= (int)m_decoder.GOPList[gop]->number && 
			raw < (int)m_decoder.GOPList[gop+1]->number) 
			return m_decoder.GOPList[gop]->matrix;
	}
	return 1;
}

/*

  MPEG2Dec's colorspace convertions Copyright (C) Chia-chen Kuo - April 2001

*/

// modified to be pitch != width friendly
// tritical - May 16, 2005

// lots of bug fixes and new isse 422->444 routine
// tritical - August 18, 2005

static const __int64 mmmask_0001 = 0x0001000100010001;
static const __int64 mmmask_0002 = 0x0002000200020002;
static const __int64 mmmask_0003 = 0x0003000300030003;
static const __int64 mmmask_0004 = 0x0004000400040004;
static const __int64 mmmask_0005 = 0x0005000500050005;
static const __int64 mmmask_0007 = 0x0007000700070007;
static const __int64 mmmask_0016 = 0x0010001000100010;
static const __int64 mmmask_0128 = 0x0080008000800080;
static const __int64 lastmask    = 0xFF00000000000000;
static const __int64 mmmask_0101 = 0x0101010101010101;

void conv420to422(const unsigned char *src, unsigned char *dst, int frame_type, int src_pitch,
				  int dst_pitch, int width, int height)
{
	int src_pitch2 = src_pitch<<1;
	int dst_pitch2 = dst_pitch<<1;
	int dst_pitch4 = dst_pitch<<2;
	int INTERLACED_HEIGHT = (height>>2) - 2;
	int PROGRESSIVE_HEIGHT = (height>>1) - 2;
	int HALF_WIDTH = width>>1; // chroma width

	if (frame_type)
	{
		if (cpu.ssemmx)
		{
			conv420to422P_iSSE(src, dst, src_pitch, dst_pitch, width, height);
			return;
		}

		__asm
		{
			mov			eax, [src] // eax = src
			mov			ebx, [dst] // ebx = dst
			mov			ecx, ebx   // ecx = dst
			add			ecx, dst_pitch // ecx = dst + dst_pitch
			xor			esi, esi
			movq		mm3, [mmmask_0003]
			pxor		mm0, mm0
			movq		mm4, [mmmask_0002]

			mov			edx, eax // edx = src
			add			edx, src_pitch // edx = src + src_pitch
			mov			edi, HALF_WIDTH

convyuv422topp:
			movd		mm1, [eax+esi]
			movd		mm2, [edx+esi]
			movd		[ebx+esi], mm1
			punpcklbw	mm1, mm0
			pmullw		mm1, mm3
			paddusw		mm1, mm4
			punpcklbw	mm2, mm0
			paddusw		mm2, mm1
			psrlw		mm2, 0x02
			packuswb	mm2, mm0

			add			esi, 0x04
			cmp			esi, edi
			movd		[ecx+esi-4], mm2
			jl			convyuv422topp

			add			eax, src_pitch
			add			ebx, dst_pitch2
			add			ecx, dst_pitch2
			xor			esi, esi

convyuv422p:
			movd		mm1, [eax+esi]

			punpcklbw	mm1, mm0
			mov			edx, eax // edx = src

			pmullw		mm1, mm3
			sub			edx, src_pitch

			movd		mm5, [edx+esi]
			add			edx, src_pitch2
			movd		mm2, [edx+esi]

			punpcklbw	mm5, mm0
			punpcklbw	mm2, mm0
			paddusw		mm5, mm1
			paddusw		mm2, mm1
			paddusw		mm5, mm4
			paddusw		mm2, mm4
			psrlw		mm5, 0x02
			psrlw		mm2, 0x02
			packuswb	mm5, mm0
			packuswb	mm2, mm0

			add			esi, 0x04
			cmp			esi, edi
			movd		[ebx+esi-4], mm5
			movd		[ecx+esi-4], mm2

			jl			convyuv422p

			add			eax, src_pitch
			add			ebx, dst_pitch2
			add			ecx, dst_pitch2
			xor			esi, esi
			dec			PROGRESSIVE_HEIGHT
			jnz			convyuv422p

			mov			edx, eax
			sub			edx, src_pitch

convyuv422bottomp:
			movd		mm1, [eax+esi]
			movd		mm5, [edx+esi]
			punpcklbw	mm5, mm0
			movd		[ecx+esi], mm1

			punpcklbw	mm1, mm0
			pmullw		mm1, mm3
			paddusw		mm5, mm1
			paddusw		mm5, mm4
			psrlw		mm5, 0x02
			packuswb	mm5, mm0

			add			esi, 0x04
			cmp			esi, edi
			movd		[ebx+esi-4], mm5
			jl			convyuv422bottomp

			emms
		}
	}
	else
	{
		__asm
		{
			mov			eax, [src] // eax = src
			mov			ecx, [dst] // ecx = dst
			xor			esi, esi
			pxor		mm0, mm0
			movq		mm3, [mmmask_0003]
			movq		mm4, [mmmask_0004]
			movq		mm5, [mmmask_0005]
			mov			edi, HALF_WIDTH

convyuv422topi:
			movd		mm1, [eax+esi]
			mov			ebx, eax
			add			ebx, src_pitch2
			movd		mm2, [ebx+esi]
			movd		[ecx+esi], mm1
			sub			ebx, src_pitch

			punpcklbw	mm1, mm0
			movq		mm6, [ebx+esi]
			pmullw		mm1, mm5
			add			ebx, src_pitch2
			punpcklbw	mm2, mm0
			movq		mm7, [ebx+esi]
			pmullw		mm2, mm3
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			mov			edx, ecx
			add			edx, dst_pitch2
			movd		[edx+esi], mm2
			sub			edx, dst_pitch

			movd		[edx+esi], mm6
			punpcklbw	mm6, mm0
			pmullw		mm6, [mmmask_0007]
			punpcklbw	mm7, mm0
			paddusw		mm7, mm6
			paddusw		mm7, mm4
			psrlw		mm7, 0x03
			packuswb	mm7, mm0

			add			edx, dst_pitch2
			add			esi, 0x04
			cmp			esi, edi
			movd		[edx+esi-4], mm7

			jl			convyuv422topi

			add			eax, src_pitch2
			add			ecx, dst_pitch4
			xor			esi, esi

convyuv422i:
			movd		mm1, [eax+esi]
			punpcklbw	mm1, mm0
			movq		mm6, mm1
			mov			ebx, eax
			sub			ebx, src_pitch2
			movd		mm3, [ebx+esi]
			pmullw		mm1, [mmmask_0007]
			punpcklbw	mm3, mm0
			paddusw		mm3, mm1
			paddusw		mm3, mm4
			psrlw		mm3, 0x03
			packuswb	mm3, mm0

			add			ebx, src_pitch
			movq		mm1, [ebx+esi]
			add			ebx, src_pitch2
			movd		[ecx+esi], mm3

			movq		mm3, [mmmask_0003]
			movd		mm2, [ebx+esi]

			punpcklbw	mm1, mm0
			pmullw		mm1, mm3
			punpcklbw	mm2, mm0
			movq		mm7, mm2
			pmullw		mm2, mm5
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			pmullw		mm6, mm5
			mov			edx, ecx
			add			edx, dst_pitch
			movd		[edx+esi], mm2

			add			ebx, src_pitch
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			pmullw		mm2, mm3
			paddusw		mm2, mm6
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			pmullw		mm7, [mmmask_0007]
			add			edx, dst_pitch
			add			ebx, src_pitch
 			movd		[edx+esi], mm2

			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			paddusw		mm2, mm7
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, dst_pitch
			add			esi, 0x04
			cmp			esi, edi
			movd		[edx+esi-4], mm2

			jl			convyuv422i
			add			eax, src_pitch2
			add			ecx, dst_pitch4
			xor			esi, esi
			dec			INTERLACED_HEIGHT
			jnz			convyuv422i

convyuv422bottomi:
			movd		mm1, [eax+esi]
			movq		mm6, mm1
			punpcklbw	mm1, mm0
			mov			ebx, eax
			sub			ebx, src_pitch2
			movd		mm3, [ebx+esi]
			punpcklbw	mm3, mm0
			pmullw		mm1, [mmmask_0007]
			paddusw		mm3, mm1
			paddusw		mm3, mm4
			psrlw		mm3, 0x03
			packuswb	mm3, mm0

			add			ebx, src_pitch
			movq		mm1, [ebx+esi]
			punpcklbw	mm1, mm0
			movd		[ecx+esi], mm3

			pmullw		mm1, [mmmask_0003]
			add			ebx, src_pitch2
			movd		mm2, [ebx+esi]
			movq		mm7, mm2
			punpcklbw	mm2, mm0
			pmullw		mm2, mm5
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			mov			edx, ecx
			add			edx, dst_pitch
			movd		[edx+esi], mm2
			add			edx, dst_pitch
 			movd		[edx+esi], mm6

			add			edx, dst_pitch
			add			esi, 0x04
			cmp			esi, edi
			movd		[edx+esi-4], mm7

			jl			convyuv422bottomi

			emms
		}
	}
}

void conv420to422P_iSSE(const unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch,
						int width, int height)
{
	int src_pitch2 = src_pitch<<1;
	int dst_pitch2 = dst_pitch<<1;
	int PROGRESSIVE_HEIGHT = (height>>1) - 2;
	int HALF_WIDTH = width>>1; // chroma width

	__asm
	{
		mov			eax, [src] // eax = src
		mov			ebx, [dst] // ebx = dst
		mov			ecx, ebx   // ecx = dst
		add			ecx, dst_pitch // ecx = dst + dst_pitch
		mov			edx, eax // edx = src
		add			edx, src_pitch // edx = src + src_pitch
		xor			esi, esi
		mov			edi, HALF_WIDTH
		movq		mm6, mmmask_0101
		movq		mm7, mm6

convyuv422topp:
		movq		mm0, [eax+esi]
		movq		mm1, [edx+esi]
		movq		[ebx+esi], mm0
		psubusb		mm1, mm7
		pavgb		mm1, mm0
		pavgb		mm1, mm0
		movq		[ecx+esi], mm1
		add			esi, 0x08
		cmp			esi, edi
		jl			convyuv422topp
		add			eax, src_pitch
		add			ebx, dst_pitch2
		add			ecx, dst_pitch2
		xor			esi, esi

convyuv422p:
		movq		mm0, [eax+esi]
		mov			edx, eax // edx = src
		movq		mm1, mm0
		sub			edx, src_pitch
		movq		mm2, [edx+esi]
		add			edx, src_pitch2
		movq		mm3, [edx+esi]
		psubusb		mm2, mm6
		psubusb		mm3, mm7
		pavgb		mm2, mm0
		pavgb		mm3, mm1
		pavgb		mm2, mm0
		pavgb		mm3, mm1
		movq		[ebx+esi], mm2
		movq		[ecx+esi], mm3
		add			esi, 0x08
		cmp			esi, edi
		jl			convyuv422p
		add			eax, src_pitch
		add			ebx, dst_pitch2
		add			ecx, dst_pitch2
		xor			esi, esi
		dec			PROGRESSIVE_HEIGHT
		jnz			convyuv422p
		mov			edx, eax
		sub			edx, src_pitch

convyuv422bottomp:
		movq		mm0, [eax+esi]
		movq		mm1, [edx+esi]
		movq		[ecx+esi], mm0
		psubusb		mm1, mm7
		pavgb		mm1, mm0
		pavgb		mm1, mm0
		movq		[ebx+esi], mm1
		add			esi, 0x08
		cmp			esi, edi
		jl			convyuv422bottomp
		emms
	}
}

void conv422to444(const unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, 
			int width, int height)
{
	if (cpu.ssemmx)
	{
		conv422to444_iSSE(src, dst, src_pitch, dst_pitch, width, height);
		return;
	}

	int HALF_WIDTH_D8 = (width>>1)-8;
	
	__asm
	{
		mov			eax, [src]	// eax = src
		mov			ebx, [dst]	// ebx = dst
		mov			edi, height	// edi = height
		mov			ecx, HALF_WIDTH_D8	// ecx = (width>>1)-8
		movq		mm1, mmmask_0001	// mm1 = 1's
		pxor		mm0, mm0	// mm0 = 0's

convyuv444init:
		movq		mm7, [eax]  // mm7 = hgfedcba
		xor			esi, esi	// esi = 0

convyuv444:
		movq		mm2, mm7    // mm2 = hgfedcba
		movq		mm7, [eax+esi+8] // mm7 = ponmlkji
		movq		mm3, mm2    // mm3 = hgfedcba
		movq		mm4, mm7    // mm4 = ponmlkji

		psrlq		mm3, 8      // mm3 = 0hgfedcb
		psllq		mm4, 56     // mm4 = i0000000
		por			mm3, mm4    // mm3 = ihgfedcb 

		movq		mm4, mm2    // mm4 = hgfedcba
		movq		mm5, mm3    // mm5 = ihgfedcb

		punpcklbw	mm4, mm0    // 0d0c0b0a
		punpcklbw	mm5, mm0    // 0e0d0c0b

		movq		mm6, mm4    // mm6 = 0d0c0b0a
		paddusw		mm4, mm1
		paddusw		mm4, mm5
		psrlw		mm4, 1		// average mm4/mm5 (d/e,c/d,b/c,a/b)	
		psllq		mm4, 8		// mm4 = z0z0z0z0
		por			mm4, mm6	// mm4 = zdzczbza

		punpckhbw	mm2, mm0	// 0h0g0f0e
		punpckhbw	mm3, mm0	// 0i0h0g0f

		movq		mm6, mm2	// mm6 = 0h0g0f0e
		paddusw		mm2, mm1
		paddusw		mm2, mm3
		psrlw		mm2, 1		// average mm2/mm3 (h/i,g/h,f/g,e/f)
		psllq		mm2, 8		// mm2 = z0z0z0z0
		por			mm2, mm6	// mm2 = zhzgzfze

		movq		[ebx+esi*2], mm4	// store zdzczbza
		movq		[ebx+esi*2+8], mm2	// store zhzgzfze

		add			esi, 8
		cmp			esi, ecx
		jl			convyuv444	// loop back if not to last 8 pixels

		movq		mm2, mm7	// mm2 = ponmlkji

		punpcklbw	mm2, mm0	// mm2 = 0l0k0j0i
		punpckhbw	mm7, mm0	// mm7 = 0p0o0n0m

		movq		mm3, mm2	// mm3 = 0l0k0j0i
		movq		mm4, mm7	// mm4 = 0p0o0n0m

		psrlq		mm2, 16		// mm2 = 000l0k0j
		psllq		mm4, 48		// mm4 = 0m000000
		por			mm2, mm4	// mm2 = 0m0l0k0j

		paddusw		mm2, mm1
		paddusw		mm2, mm3
		psrlw		mm2, 1		// average mm2/mm3 (m/l,l/k,k/j,j/i)	
		psllq		mm2, 8		// mm2 = z0z0z0z0
		por			mm2, mm3	// mm2 = zlzkzjzi

		movq		mm6, mm7	// mm6 = 0p0o0n0m
		movq		mm4, mm7	// mm4 = 0p0o0n0m

		psrlq		mm6, 48		// mm6 = 0000000p
		psrlq		mm4, 16		// mm4 = 000p0o0n
		psllq		mm6, 48		// mm6 = 0p000000
		por			mm4,mm6		// mm4 = 0p0p0o0n

		paddusw		mm4, mm1
		paddusw		mm4, mm7
		psrlw		mm4, 1		// average mm4/mm7 (p/p,p/o,o/n,n/m)	
		psllq		mm4, 8		// mm4 = z0z0z0z0
		por			mm4, mm7	// mm4 = zpzoznzm

		movq		[ebx+esi*2], mm2	// store mm2
		movq		[ebx+esi*2+8], mm4	// store mm4	

		add			eax, src_pitch
		add			ebx, dst_pitch
		dec			edi
		jnz			convyuv444init

		emms
	}
}

void conv422to444_iSSE(const unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, 
			int width, int height)
{
	int HALF_WIDTH_D8 = (width>>1)-8;
	
	__asm
	{
		mov			eax, [src]	// eax = src
		mov			ebx, [dst]	// ebx = dst
		mov			edi, height	// edi = height
		mov			ecx, HALF_WIDTH_D8	// ecx = (width>>1)-8
		mov			edx, src_pitch
		xor			esi, esi	// esi = 0
		movq		mm7, lastmask

xloop:
		movq		mm0, [eax+esi]  // mm7 = hgfedcba
		movq		mm1, [eax+esi+1]// mm1 = ihgfedcb
		pavgb		mm1, mm0
		movq		mm2, mm0
		punpcklbw	mm0,mm1
		punpckhbw	mm2,mm1

		movq		[ebx+esi*2], mm0	// store mm0
		movq		[ebx+esi*2+8], mm2	// store mm2

		add			esi, 8
		cmp			esi, ecx
		jl			xloop	// loop back if not to last 8 pixels

		movq		mm0, [eax+esi]  // mm7 = hgfedcba
		movq		mm1, mm0		// mm1 = hgfedcba
		movq		mm2, mm0		// mm2 = hgfedcba
		psrlq		mm1, 8			// mm1 = 0hgfedcb
		pand		mm2, mm7		// mm2 = h0000000
		por			mm1, mm2		// mm1 = hhgfedcb
		pavgb		mm1, mm0
		movq		mm2, mm0
		punpcklbw	mm0,mm1
		punpckhbw	mm2,mm1

		movq		[ebx+esi*2], mm0	// store mm0
		movq		[ebx+esi*2+8], mm2	// store mm2

		add			eax, edx
		add			ebx, dst_pitch
		xor			esi, esi
		dec			edi
		jnz			xloop
		emms
	}
}

void conv444toRGB24(const unsigned char *py, const unsigned char *pu, const unsigned char *pv, 
				unsigned char *dst, int src_pitchY, int src_pitchUV, int dst_pitch, int width, 
				int height, int matrix, int pc_scale)
{
	__int64 RGB_Offset, RGB_Scale, RGB_CBU, RGB_CRV, RGB_CGX;
	int dst_modulo = dst_pitch-(3*width);

	if (pc_scale)
	{
		RGB_Scale = 0x1000254310002543;
		RGB_Offset = 0x0010001000100010;
		if (matrix == 7) // SMPTE 240M (1987)
		{
			RGB_CBU = 0x0000428500004285;
			RGB_CGX = 0xF7BFEEA3F7BFEEA3;
			RGB_CRV = 0x0000396900003969;
		}
		else if (matrix == 6 || matrix == 5) // SMPTE 170M/ITU-R BT.470-2 -- BT.601
		{
			RGB_CBU = 0x0000408D0000408D;
			RGB_CGX = 0xF377E5FCF377E5FC;
			RGB_CRV = 0x0000331300003313;
		}
		else if (matrix == 4) // FCC
		{
			RGB_CBU = 0x000040D8000040D8;
			RGB_CGX = 0xF3E9E611F3E9E611;
			RGB_CRV = 0x0000330000003300;
		}
		else // ITU-R Rec.709 (1990) -- BT.709
		{
			RGB_CBU = 0x0000439A0000439A;
			RGB_CGX = 0xF92CEEF1F92CEEF1;
			RGB_CRV = 0x0000395F0000395F;
		}
	}
	else
	{
		RGB_Scale = 0x1000200010002000;
		RGB_Offset = 0x0000000000000000;
		if (matrix == 7) // SMPTE 240M (1987)
		{
			RGB_CBU = 0x00003A6F00003A6F;
			RGB_CGX = 0xF8C0F0BFF8C0F0BF;
			RGB_CRV = 0x0000326E0000326E;
		}
		else if (matrix == 6 || matrix == 5) // SMPTE 170M/ITU-R BT.470-2 -- BT.601
		{
			RGB_CBU = 0x000038B4000038B4;
			RGB_CGX = 0xF4FDE926F4FDE926;
			RGB_CRV = 0x00002CDD00002CDD;
		}
		else if (matrix == 4) // FCC
		{
			RGB_CBU = 0x000038F6000038F6;
			RGB_CGX = 0xF561E938F561E938;
			RGB_CRV = 0x00002CCD00002CCD;
		}
		else // ITU-R Rec.709 (1990) -- BT.709
		{
			RGB_CBU = 0x00003B6200003B62;
			RGB_CGX = 0xFA00F104FA00F104;
			RGB_CRV = 0x0000326600003266;
		}
	}

	__asm
	{
		mov			eax, [py]  // eax = py
		mov			ebx, [pu]  // ebx = pu
		mov			ecx, [pv]  // ecx = pv
		mov			edx, [dst] // edx = dst
		mov			edi, width // edi = width
		xor			esi, esi
		pxor		mm0, mm0

convRGB24:
		movd		mm1, [eax+esi]
		movd		mm3, [ebx+esi]
		punpcklbw	mm1, mm0
		punpcklbw	mm3, mm0
		movd		mm5, [ecx+esi]
		punpcklbw	mm5, mm0
		movq		mm7, [mmmask_0128]
		psubw		mm3, mm7
		psubw		mm5, mm7

		psubw		mm1, RGB_Offset
		movq		mm2, mm1
		movq		mm7, [mmmask_0001]
		punpcklwd	mm1, mm7
		punpckhwd	mm2, mm7
		movq		mm7, RGB_Scale
		pmaddwd		mm1, mm7
		pmaddwd		mm2, mm7

		movq		mm4, mm3
		punpcklwd	mm3, mm0
		punpckhwd	mm4, mm0
		movq		mm7, RGB_CBU
		pmaddwd		mm3, mm7
		pmaddwd		mm4, mm7
		paddd		mm3, mm1
		paddd		mm4, mm2
		psrad		mm3, 13
		psrad		mm4, 13
		packuswb	mm3, mm0
		packuswb	mm4, mm0

		movq		mm6, mm5
		punpcklwd	mm5, mm0
		punpckhwd	mm6, mm0
		movq		mm7, RGB_CRV
		pmaddwd		mm5, mm7
		pmaddwd		mm6, mm7
		paddd		mm5, mm1
		paddd		mm6, mm2
		psrad		mm5, 13
		psrad		mm6, 13
		packuswb	mm5, mm0
		packuswb	mm6, mm0

		punpcklbw	mm3, mm5
		punpcklbw	mm4, mm6
		movq		mm5, mm3
		movq		mm6, mm4
		psrlq		mm5, 16
		psrlq		mm6, 16
		por			mm3, mm5
		por			mm4, mm6

		movd		mm5, [ebx+esi]
		movd		mm6, [ecx+esi]
		punpcklbw	mm5, mm0
		punpcklbw	mm6, mm0
		movq		mm7, [mmmask_0128]
		psubw		mm5, mm7
		psubw		mm6, mm7

		movq		mm7, mm6
		punpcklwd	mm6, mm5
		punpckhwd	mm7, mm5		
		movq		mm5, RGB_CGX
		pmaddwd		mm6, mm5
		pmaddwd		mm7, mm5
		paddd		mm6, mm1
		paddd		mm7, mm2

		psrad		mm6, 13
		psrad		mm7, 13
		packuswb	mm6, mm0
		packuswb	mm7, mm0

		punpcklbw	mm3, mm6
		punpcklbw	mm4, mm7

		movq		mm1, mm3
		movq		mm5, mm4
		movq		mm6, mm4

		psrlq		mm1, 32
		psllq		mm1, 24
		por			mm1, mm3

		psrlq		mm3, 40
		psllq		mm6, 16
		por			mm3, mm6
		movd		[edx], mm1

		psrld		mm4, 16
		psrlq		mm5, 24
		por			mm5, mm4
		movd		[edx+4], mm3

		add			edx, 0x0c
		add			esi, 0x04
		cmp			esi, edi
		movd		[edx-4], mm5

		jl			convRGB24

		add			eax, src_pitchY
		add			ebx, src_pitchUV
		add			ecx, src_pitchUV
		add			edx, dst_modulo
		xor			esi, esi
		dec			height
		jnz			convRGB24

		emms
	}
}

void conv422toYUV422(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, 
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	int widthdiv2 = width >> 1;
	__asm
	{
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov edi,[dst]
		mov ecx,widthdiv2
yloop:
		xor eax,eax
		align 16
xloop:
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		punpcklbw mm0,mm1      ;VYUYVYUY
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4],mm0   ;store
		movq [edi+eax*4+8],mm3 ;store
		add eax,4
		cmp eax,ecx
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
		emms
	}
}

void MPEG2Source::conv422toYUV422(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, 
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	int widthdiv2 = width >> 1;
	__asm
	{
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov edi,[dst]
		mov ecx,widthdiv2
yloop:
		xor eax,eax
		align 16
xloop:
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		punpcklbw mm0,mm1      ;VYUYVYUY
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4],mm0   ;store
		movq [edi+eax*4+8],mm3 ;store
		add eax,4
		cmp eax,ecx
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
		emms
	}
}

/*
	End of MPEG2Dec's colorspace convertions 
*/

BlindPP::BlindPP(AVSValue args, IScriptEnvironment* env)  : GenericVideoFilter(args[0].AsClip()) 
{
	int quant = args[1].AsInt(2);
	if (vi.width%16!=0)
		env->ThrowError("BlindPP : Need mod16 width");
	if (vi.height%16!=0)
		env->ThrowError("BlindPP : Need mod16 height");
	if (!vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("BlindPP : Only YV12 and YUY2 colorspace supported");

	QP = NULL;
	QP = new int[vi.width*vi.height/256];
	if (QP == NULL)
		env->ThrowError("BlindPP:  malloc failure!");
	for (int i=0;i<vi.width*vi.height/256;i++)
		QP[i] = quant;

	switch (args[2].AsInt(6)) {
		case 0 : PP_MODE = 0; break;
		case 1 : PP_MODE = PP_DEBLOCK_Y_H; break;
		case 2 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
		case 3 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
		case 4 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
		case 5 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
		case 6 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
		default : env->ThrowError("BlindPP : cpu level invalid (0 to 6)");
	}

	const char* cpu2 = args[3].AsString("");
	if (strlen(cpu2)==6) {
		PP_MODE = 0;
		if (cpu2[0]=='x' || cpu2[0]=='X') { PP_MODE |= PP_DEBLOCK_Y_H; }
		if (cpu2[1]=='x' || cpu2[1]=='X') { PP_MODE |= PP_DEBLOCK_Y_V; }
		if (cpu2[2]=='x' || cpu2[2]=='X') { PP_MODE |= PP_DEBLOCK_C_H; }
		if (cpu2[3]=='x' || cpu2[3]=='X') { PP_MODE |= PP_DEBLOCK_C_V; }
		if (cpu2[4]=='x' || cpu2[4]=='X') { PP_MODE |= PP_DERING_Y; }
		if (cpu2[5]=='x' || cpu2[5]=='X') { PP_MODE |= PP_DERING_C; }
	}

	iPP = args[4].AsBool(false);
	moderate_h = args[5].AsInt(20);
	moderate_v = args[6].AsInt(40);

	if (vi.IsYUY2()) 
	{
		out = create_YV12PICT(vi.height,vi.width,2);
		PP_MODE |= PP_DONT_COPY;
	}
	else out = NULL;
}

PVideoFrame __stdcall BlindPP::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame dstf = env->NewVideoFrame(vi);
	PVideoFrame cf = child->GetFrame(n,env);

	if (vi.IsYV12())
	{
		uc* src[3];
		uc* dst[3];
		src[0] = (uc*) cf->GetReadPtr(PLANAR_Y);
		src[1] = (uc*) cf->GetReadPtr(PLANAR_U);
		src[2] = (uc*) cf->GetReadPtr(PLANAR_V);
		dst[0] = dstf->GetWritePtr(PLANAR_Y);
		dst[1] = dstf->GetWritePtr(PLANAR_U);
		dst[2] = dstf->GetWritePtr(PLANAR_V);
        postprocess(src, cf->GetPitch(PLANAR_Y), cf->GetPitch(PLANAR_U),
                    dst, dstf->GetPitch(PLANAR_Y), dstf->GetPitch(PLANAR_U),
                    vi.width, vi.height, QP, vi.width/16, 
                    PP_MODE, moderate_h, moderate_v, false, iPP);
	}
	else
	{
		uc* dst[3];
		convYUV422to422(cf->GetReadPtr(),out->y,out->u,out->v,cf->GetPitch(),out->ypitch,
			out->uvpitch,vi.width,vi.height); // 4:2:2 packed to 4:2:2 planar
		dst[0] = out->y;
		dst[1] = out->u;
		dst[2] = out->v;
        postprocess(dst, out->ypitch, out->uvpitch,
                    dst, out->ypitch, out->uvpitch,
                    vi.width, vi.height, QP, vi.width/16, PP_MODE, 
                    moderate_h, moderate_v, true, iPP);
		conv422toYUV422(out->y,out->u,out->v,dstf->GetWritePtr(),out->ypitch,out->uvpitch,
			dstf->GetPitch(),vi.width,vi.height);  // 4:2:2 planar to 4:2:2 packed
	}

	return dstf;
}

BlindPP::~BlindPP() 
{ 
	if (QP != NULL) delete[] QP;
	if (out != NULL) destroy_YV12PICT(out);
}

void BlindPP::convYUV422to422(const unsigned char *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
       int pitch1, int pitch2y, int pitch2uv, int width, int height)
{
	int widthdiv2 = width>>1;
	__int64 Ymask = 0x00FF00FF00FF00FFi64;
	__asm
	{
		mov edi,[src]
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov ecx,widthdiv2
		movq mm5,Ymask
	yloop:
		xor eax,eax
		align 16
	xloop:
		movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
		movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
		movq mm2,mm0           ; VYUYVYUY - 1
		movq mm3,mm1           ; VYUYVYUY - 2
		pand mm0,mm5           ; 0Y0Y0Y0Y - 1
		psrlw mm2,8 	       ; 0V0U0V0U - 1
		pand mm1,mm5           ; 0Y0Y0Y0Y - 2
		psrlw mm3,8            ; 0V0U0V0U - 2
		packuswb mm0,mm1       ; YYYYYYYY
		packuswb mm2,mm3       ; VUVUVUVU
		movq mm4,mm2           ; VUVUVUVU
		pand mm2,mm5           ; 0U0U0U0U
		psrlw mm4,8            ; 0V0V0V0V
		packuswb mm2,mm2       ; xxxxUUUU
		packuswb mm4,mm4       ; xxxxVVVV
		movq [ebx+eax*2],mm0   ; store y
		movd [edx+eax],mm2     ; store u
		movd [esi+eax],mm4     ; store v
		add eax,4
		cmp eax,ecx
		jl xloop
		add edi,pitch1
		add ebx,pitch2y
		add edx,pitch2uv
		add esi,pitch2uv
		dec height
		jnz yloop
		emms
	}
}

Deblock::Deblock(PClip _child, int q, int aOff, int bOff, bool _mmx, bool _isse,
                 IScriptEnvironment* env) :
GenericVideoFilter(_child)
{
    nQuant = sat(q, 0, 51);
    nAOffset = sat(aOff, -nQuant, 51 - nQuant);
    nBOffset = sat(bOff, -nQuant, 51 - nQuant);
    nWidth = vi.width;
    nHeight = vi.height;

    if ( !vi.IsYV12() )
        env->ThrowError("Deblock : need YV12 input");
    if (( nWidth & 7 ) || ( nHeight & 7 ))
        env->ThrowError("Deblock : width and height must be mod 8");
}

Deblock::~Deblock()
{
}

const int alphas[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 4, 4,
    5, 6, 7, 8, 9, 10,
    12, 13, 15, 17, 20,
    22, 25, 28, 32, 36,
    40, 45, 50, 56, 63, 
    71, 80, 90, 101, 113,
    127, 144, 162, 182,
    203, 226, 255, 255
};

const int betas[52] = {
    0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 2,
    2, 3, 3, 3, 3, 4,
    4, 4, 6, 6, 
    7, 7, 8, 8, 9, 9,
    10, 10, 11, 11, 12,
    12, 13, 13, 14, 14, 
    15, 15, 16, 16, 17, 
    17, 18, 18
};

const int cs[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 
    1, 2, 2, 2, 2, 3, 
    3, 3, 4, 4, 5, 5,
    6, 7, 8, 8, 10, 
    11, 12, 13, 15, 17
};

void Deblock::DeblockPicture(unsigned char *srcp, int srcPitch, int w, int h,
                             int q, int aOff, int bOff)
{
    int indexa, indexb;
    for ( int j = 0; j < h; j += 4 )
    {
        for ( int i = 0; i < w; i += 4 )
        {
            indexa = sat(q + aOff, 0, 51);
            indexb = sat(q + bOff, 0, 51);
            if ( j > 0 )
                DeblockHorEdge(srcp + i, srcPitch, indexa, indexb);
            if ( i > 0 )
                DeblockVerEdge(srcp + i, srcPitch, indexa, indexb);
        }
        srcp += 4 * srcPitch;
    }
}

void Deblock::DeblockHorEdge(unsigned char *srcp, int srcPitch, int ia, int ib)
{
    int alpha = alphas[ia];
    int beta = betas[ib];
    int c, c0 = cs[ia];
    unsigned char *sq0 = srcp;
    unsigned char *sq1 = srcp + srcPitch;
    unsigned char *sq2 = srcp + 2 * srcPitch;
    unsigned char *sp0 = srcp - srcPitch;
    unsigned char *sp1 = srcp - 2 * srcPitch;
    unsigned char *sp2 = srcp - 3 * srcPitch;
    int delta, ap, aq, deltap1, deltaq1;

    for ( int i = 0; i < 4; i ++ )
    {
        if (( abs(sp0[i] - sq0[i]) < alpha ) && ( abs(sp1[i] - sp0[i]) < beta ) && ( abs(sq0[i] - sq1[i]) < beta ))
        {
            ap = abs(sp2[i] - sp0[i]);
            aq = abs(sq2[i] - sq0[i]);
            c = c0;
            if ( aq < beta ) c++;
            if ( ap < beta ) c++;
            delta = sat((((sq0[i] - sp0[i]) << 2) + (sp1[i] - sq1[i]) + 4) >> 3, -c, c);
            deltap1 = sat((sp2[i] + ((sp0[i] + sq0[i] + 1) >> 1) - (sp1[i] << 1)) >> 1, -c0, c0);
            deltaq1 = sat((sq2[i] + ((sp0[i] + sq0[i] + 1) >> 1) - (sq1[i] << 1)) >> 1, -c0, c0);
            sp0[i] = (unsigned char)sat(sp0[i] + delta, 0, 255);
            sq0[i] = (unsigned char)sat(sq0[i] - delta, 0, 255);
            if ( ap < beta )
                sp1[i] = (unsigned char)(sp1[i] + deltap1);
            if ( aq < beta )
                sq1[i] = (unsigned char)(sq1[i] + deltaq1);
        }
    }
}

void Deblock::DeblockVerEdge(unsigned char *srcp, int srcPitch, int ia, int ib)
{
    int alpha = alphas[ia];
    int beta = betas[ib];
    int c, c0 = cs[ia];
    unsigned char *s = srcp;

    int delta, ap, aq, deltap1, deltaq1;

    for ( int i = 0; i < 4; i ++ )
    {
        if (( abs(s[0] - s[-1]) < alpha ) && ( abs(s[1] - s[0]) < beta ) && ( abs(s[-1] - s[-2]) < beta ))
        {
            ap = abs(s[2] - s[0]);
            aq = abs(s[-3] - s[-1]);
            c = c0;
            if ( aq < beta ) c++;
            if ( ap < beta ) c++;
            delta = sat((((s[0] - s[-1]) << 2) + (s[-2] - s[1]) + 4) >> 3, -c, c);
            deltaq1 = sat((s[2] + ((s[0] + s[-1] + 1) >> 1) - (s[1] << 1)) >> 1, -c0, c0);
            deltap1 = sat((s[-3] + ((s[0] + s[-1] + 1) >> 1) - (s[-2] << 1)) >> 1, -c0, c0);
            s[0] = (unsigned char)sat(s[0] - delta, 0, 255);
            s[-1] = (unsigned char)sat(s[-1] + delta, 0, 255);
            if ( ap < beta )
                s[1] = (unsigned char)(s[1] + deltaq1);
            if ( aq < beta )
                s[-2] = (unsigned char)(s[-2] + deltap1);
        }
        s += srcPitch;
    }
}



PVideoFrame __stdcall Deblock::GetFrame(int n, IScriptEnvironment *env)
{
    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);

    DeblockPicture(YWPLAN(src), YPITCH(src), nWidth, nHeight,
                   nQuant, nAOffset, nBOffset);
    DeblockPicture(UWPLAN(src), UPITCH(src), nWidth / 2, nHeight / 2,
                   nQuant, nAOffset, nBOffset);
    DeblockPicture(VWPLAN(src), VPITCH(src), nWidth / 2, nHeight / 2,
                   nQuant, nAOffset, nBOffset);

    return src;
}

#include "lumayv12.cpp"

AVSValue __cdecl Create_MPEG2Source(AVSValue args, void*, IScriptEnvironment* env) 
{
//	char path[1024];
	char buf[80], *p;

	char d2v[255];
	int cpu = 0;
	int idct = -1;

	// check if iPP is set, if it is use the set value... if not
	// we set iPP to -1 which automatically switches between field
	// and progressive pp based on pf flag -- same for iCC
	int iPP = args[3].IsBool() ? (args[3].AsBool() ? 1 : 0) : -1;
	int iCC = args[12].IsBool() ? (args[12].AsBool() ? 1 : 0) : -1;

	int moderate_h = 20;
	int moderate_v = 40;
	bool showQ = false;
	bool fastMC = false;
	char cpu2[255];
	int info = 0;
	int upConv = 0;
	bool i420 = false;

	/* Based on D.Graft Msharpen default files code */
	/* Load user defaults if they exist. */ 
#define LOADINT(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
	p = buf; \
	while(*p++ != '='); \
	var = atoi(p); \
}

#define LOADSTR(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
	p = buf; \
	while(*p++ != '='); \
	char* dst = var; \
	int i=0; \
	if (*p++ == '"') while('"' != *p++) i++; \
	var[i] = 0; \
	strncpy(var,(p-i-1),i); \
}

#define LOADBOOL(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
	p = buf; \
	while(*p++ != '='); \
	if (*p == 't') var = true; \
	else var = false; \
}
	try
	{
		FILE *f;

		if (!fopen_s(&f, "DGDecode.def", "r"))
		{
			while(fgets(buf, 80, f) != 0)
			{
				LOADSTR(d2v,"d2v=",4)
				LOADINT(cpu,"cpu=",4)
				LOADINT(idct,"idct=",5)
				LOADBOOL(iPP,"iPP=",4)
				LOADINT(moderate_h,"moderate_h=",11)
				LOADINT(moderate_v,"moderate_v=",11)
				LOADBOOL(showQ,"showQ=",6)
				LOADBOOL(fastMC,"fastMC=",7)
				LOADSTR(cpu2,"cpu2=",5)
				LOADINT(info,"info=",5);
				LOADINT(upConv,"upConv=",7);
				LOADBOOL(i420,"i420=",5);
				LOADBOOL(iCC,"iCC=",4);
			}
		}
	}
	catch (...)
	{
		// plugin directory not set
		// probably using an older version avisynth
	}

	// check for uninitialised strings
	if (strlen(d2v)>=255) d2v[0]=0;
	if (strlen(cpu2)>=255) cpu2[0]=0;

	MPEG2Source *dec = new MPEG2Source( args[0].AsString(d2v),
										args[1].AsInt(cpu),
										args[2].AsInt(idct),
										iPP,
										args[4].AsInt(moderate_h),
										args[5].AsInt(moderate_v),
										args[6].AsBool(showQ),
										args[7].AsBool(fastMC),
										args[8].AsString(cpu2),
										args[9].AsInt(info),
										args[10].AsInt(upConv),
										args[11].AsBool(i420),
										iCC,
										env );
		// Only bother invoking crop if we have to.
		if (dec->m_decoder.Clip_Top    || 
			dec->m_decoder.Clip_Bottom || 
			dec->m_decoder.Clip_Left   || 
			dec->m_decoder.Clip_Right ||
			// This is cheap but it works. The intent is to allow the
			// display size to be different from the encoded size, while
			// not requiring massive revisions to the code. So we detect the
			// difference and crop it off.
			dec->m_decoder.vertical_size != dec->m_decoder.Clip_Height ||
			dec->m_decoder.horizontal_size != dec->m_decoder.Clip_Width ||
			dec->m_decoder.vertical_size == 1088)
		{
			int vertical;
			// Special case for 1088 to 1080 as directed by DGIndex.
			if (dec->m_decoder.vertical_size == 1088 && dec->m_decoder.D2V_Height == 1080)
				vertical = 1080;
			else
				vertical = dec->m_decoder.vertical_size;
			AVSValue CropArgs[5] =
			{
				dec,
				dec->m_decoder.Clip_Left, 
				dec->m_decoder.Clip_Top, 
				-(dec->m_decoder.Clip_Right + (dec->m_decoder.Clip_Width - dec->m_decoder.horizontal_size)),
				-(dec->m_decoder.Clip_Bottom + (dec->m_decoder.Clip_Height - vertical))
			};

			return env->Invoke("crop", AVSValue(CropArgs,5));
		}

	return dec;
}

AVSValue __cdecl Create_LumaYV12(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new LumaYV12(args[0].AsClip(),
						args[1].AsInt(0),					//lumoff
						args[2].AsFloat(1.00),				//lumgain
						env);
}

AVSValue __cdecl Create_BlindPP(AVSValue args, void* user_data, IScriptEnvironment* env) 
{	
	return new BlindPP(args,env);	
}

AVSValue __cdecl Create_Deblock(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Deblock(
		args[0].AsClip(),
      args[1].AsInt(25),
      args[2].AsInt(0),
      args[3].AsInt(0),
		args[4].AsBool(true),
      args[5].AsBool(true),
    	env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("MPEG2Source", "[d2v]s[cpu]i[idct]i[iPP]b[moderate_h]i[moderate_v]i[showQ]b[fastMC]b[cpu2]s[info]i[upConv]i[i420]b[iCC]b", Create_MPEG2Source, 0);
	env->AddFunction("LumaYV12","c[lumoff]i[lumgain]f",Create_LumaYV12,0);
    env->AddFunction("BlindPP", "c[quant]i[cpu]i[cpu2]s[iPP]b[moderate_h]i[moderate_v]i", Create_BlindPP, 0);
    env->AddFunction("Deblock", "c[quant]i[aOffset]i[bOffset]i[mmx]b[isse]b", Create_Deblock, 0);
    return 0;
}

// Code for backward compatibility and Gordian Knot support
// (running DGDecode functions without Avisynth).
// Also used by DGVFapi.

class NoAVSAccess
{
public:
	MPEG2Source* vf;
	VideoInfo pvi;
	unsigned char *buffer, *picture;
	unsigned char *u422, *u444, *v422, *v444;
	int pitch, vfapi_progressive, ident;
	NoAVSAccess *prv, *nxt;
	NoAVSAccess::NoAVSAccess()
	{
		buffer = picture = u422 = v422 = u444 = v444 = NULL;
		pitch = vfapi_progressive = ident = -1;
		vf = NULL;
		nxt = prv = NULL;
	}
	void NoAVSAccess::cleanUp()
	{
		if (buffer) { free(buffer); buffer = NULL; }
		if (picture) { free(picture); picture = NULL; }
		if (u422) { free(u422); u422 = NULL; }
		if (v422) { free(v422); v422 = NULL; }
		if (u444) { free(u444); u444 = NULL; }
		if (v444) { free(v444); v444 = NULL; }
		if (vf) { delete vf; vf = NULL; }
	}
	NoAVSAccess::~NoAVSAccess()
	{
		if (buffer) free(buffer);
		if (picture) free(picture);
		if (u422) free(u422);
		if (v422) free(v422);
		if (u444) free(u444);
		if (v444) free(v444);
		if (vf) delete vf;
	}
};

class NoAVSLinkedList
{
public:
	NoAVSAccess *LLB, *LLE;
	NoAVSLinkedList::NoAVSLinkedList() : LLB(NULL), LLE(NULL) {};
	NoAVSLinkedList::~NoAVSLinkedList()
	{
		for (NoAVSAccess *i=LLB; i;)
		{
			NoAVSAccess *j = i->nxt;
			delete i;
			i = j;
		}
		LLB = LLE = NULL;
	}
	void NoAVSLinkedList::remapPointers(NoAVSAccess *i)
	{
		if (i->prv)
		{
			if (i->nxt) i->prv->nxt = i->nxt;
			else i->prv->nxt = NULL;
		}
		else if (LLB == i) LLB = i->nxt;
		if (i->nxt)
		{
			if (i->prv) i->nxt->prv = i->prv;
			else i->nxt->prv = NULL;
		}
		else if (LLE == i) LLE = i->prv;
	}
};

NoAVSAccess g_A;
NoAVSLinkedList g_LL;

MPEG2Source::MPEG2Source(const char* d2v, int _upConv)
{
	CheckCPU();

	m_decoder.refinit = false;
	m_decoder.fpuinit = false;
	m_decoder.luminit = false;
	ovr_idct = -1;
	m_decoder.IDCT_Flag = IDCT_MMX;
	m_decoder.idctFunc = MMX_IDCT;
	m_decoder.info = 0;
	m_decoder.showQ = false;
	m_decoder.fastMC = false;
	m_decoder.upConv = _upConv == 1 ? 1 : 0;
	m_decoder.i420 = false;
	m_decoder.Clip_Width = -1;  // used for checking correct opening
	m_decoder.pp_mode = 0;
	m_decoder.moderate_h = 20;
	m_decoder.moderate_v = 40;
	m_decoder.iPP = -1;
	m_decoder.iCC = -1;

	const char *path = d2v;

	FILE *f;
	if ((f = fopen(path,"r"))==NULL)
	{
		dprintf("MPEG2Source : unable to load file \"%s\" ",path);
		return;
	}
	
	fclose(f);

	if (m_decoder.Open(path))
	{
		dprintf("MPEG2Source: couldn't open file or obsolete D2V file");
		return;
	}

	memset(&vi, 0, sizeof(vi));
	vi.width = m_decoder.Clip_Width;
	vi.height = m_decoder.Clip_Height;
	if (m_decoder.chroma_format == 1 && _upConv == 0) vi.pixel_type = VideoInfo::CS_YV12;
	else if (m_decoder.chroma_format == 2 || _upConv > 0) vi.pixel_type = VideoInfo::CS_YUY2;
	else
	{
		dprintf("MPEG2Source: unsupported color format (4:4:4)");
		return;
	}
	vi.fps_numerator = m_decoder.VF_FrameRate_Num;
	vi.fps_denominator = m_decoder.VF_FrameRate_Den;
	vi.num_frames = m_decoder.VF_FrameLimit;
	vi.SetFieldBased(false);

	bufY = bufU = bufV = NULL;
	u444 = v444 = NULL;

	out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);
}

void MPEG2Source::GetFrame(int n, unsigned char *buffer, int pitch)
{
	out->y = buffer;
	out->u = out->y + (pitch * m_decoder.Coded_Picture_Height);
	if (vi.IsYV12()) out->v = out->u + ((pitch * m_decoder.Chroma_Height)/2);
	else out->v = out->u + ((pitch * m_decoder.Coded_Picture_Height)/2);
	out->ypitch = pitch;
	out->uvpitch = pitch/2;
	out->ywidth = m_decoder.Coded_Picture_Width;
	out->yheight = m_decoder.Coded_Picture_Height;
	out->uvwidth = m_decoder.Chroma_Width;
	if (vi.IsYV12()) out->uvheight = m_decoder.Chroma_Height;
	else out->uvheight = m_decoder.Coded_Picture_Height;

	m_decoder.Decode(n, out);

	__asm emms;
}

/* 
** These are the older legacy functions.  Only one instance is supported.
** They now use g_A, which is just a class wrapper for the old globals.
*/

extern "C" __declspec(dllexport) VideoInfo* __cdecl openMPEG2Source(char* file)
{
	g_A.cleanUp();

	char *p;

	// If the D2V filename has _P just before the extension, force
	// progressive upsampling.
	p = file + strlen(file);
	while (*p != '.' && p != file) p--;
	if (p == file) return NULL;
	p -= 2;
	if (p[0] == '_' && p[1] == 'P') g_A.vfapi_progressive = 1;
	else
	{
		// If the D2V filename has _A just before the extension, force
		// automatic upsampling based on pf flag (getFrame() will return yuy2
		// even if input is yv12). Otherwise, default to interlaced upsampling.
		p = file + strlen(file);
		while (*p != '.' && p != file) p--;
		if (p == file) return NULL;
		p -= 2;
		if (p[0] == '_' && p[1] == 'A') g_A.vfapi_progressive = -1;
		else g_A.vfapi_progressive = 0;
	}

	g_A.vf = new MPEG2Source(file, g_A.vfapi_progressive == -1 ? 1 : 0);
	if (!g_A.vf || g_A.vf->m_decoder.Clip_Width < 0) return NULL;

	g_A.pvi = g_A.vf->GetVideoInfo();

	g_A.buffer = (unsigned char*)malloc(g_A.pvi.height * g_A.pvi.width * 2);
	g_A.picture = (unsigned char*)malloc(g_A.pvi.height * g_A.pvi.width * 3);

	g_A.u422 = (unsigned char*)malloc((g_A.pvi.height * g_A.pvi.width)>>1);
	g_A.u444 = (unsigned char*)malloc(g_A.pvi.height * g_A.pvi.width);

	g_A.v422 = (unsigned char*)malloc((g_A.pvi.height * g_A.pvi.width)>>1);
	g_A.v444 = (unsigned char*)malloc(g_A.pvi.height * g_A.pvi.width);

	g_A.pitch = g_A.pvi.width;
	
	return &g_A.pvi;
}

extern "C" __declspec(dllexport) unsigned char* __cdecl getFrame(int frame)
{
	if (g_A.vf->GetVideoInfo().pixel_type == VideoInfo::CS_YV12)
	{
		g_A.vf->GetFrame(frame, g_A.buffer, g_A.pitch);
	}
	else
	{
		g_A.vf->GetFrame(frame, g_A.picture, g_A.pitch);
		conv422toYUV422(g_A.picture, g_A.picture+(g_A.pitch*g_A.vf->m_decoder.Clip_Height),
			g_A.picture+(g_A.pitch*g_A.vf->m_decoder.Clip_Height)+((g_A.pitch*g_A.vf->m_decoder.Clip_Height)/2),
			g_A.buffer,g_A.pitch,g_A.pitch/2,g_A.pitch*2,g_A.vf->m_decoder.Clip_Width,
			g_A.vf->m_decoder.Clip_Height);
	}

	return g_A.buffer;
}

extern "C" __declspec(dllexport) unsigned char* __cdecl getRGBFrame(int frame)
{
	g_A.vf->GetFrame(frame, g_A.buffer, g_A.pitch);

	if (g_A.vf->GetVideoInfo().pixel_type == VideoInfo::CS_YV12)
	{
		unsigned char *y = g_A.buffer;
		unsigned char *u = y + (g_A.pitch * g_A.pvi.height);
		unsigned char *v = u + ((g_A.pitch * g_A.pvi.height)/4);
		conv420to422(u, g_A.u422, g_A.vfapi_progressive, g_A.pvi.width>>1, g_A.pvi.width>>1, 
			g_A.pvi.width, g_A.pvi.height);
		conv422to444(g_A.u422, g_A.u444, g_A.pvi.width>>1, g_A.pvi.width, g_A.pvi.width, g_A.pvi.height);
		conv420to422(v, g_A.v422, g_A.vfapi_progressive, g_A.pvi.width>>1, g_A.pvi.width>>1, 
			g_A.pvi.width, g_A.pvi.height);
		conv422to444(g_A.v422, g_A.v444, g_A.pvi.width>>1, g_A.pvi.width, g_A.pvi.width, g_A.pvi.height);
		conv444toRGB24(y, g_A.u444, g_A.v444, g_A.picture, g_A.pvi.width, g_A.pvi.width, 
			g_A.pvi.width * 3, g_A.pvi.width, g_A.pvi.height, g_A.vf->getMatrix(frame), 
			g_A.vf->m_decoder.pc_scale);
	}
	else
	{
		unsigned char *y = g_A.buffer;
		unsigned char *u = y + (g_A.pitch * g_A.pvi.height);
		unsigned char *v = u + ((g_A.pitch * g_A.pvi.height)/2);
		conv422to444(u, g_A.u444, g_A.pvi.width>>1, g_A.pvi.width, g_A.pvi.width, g_A.pvi.height);
		conv422to444(v, g_A.v444, g_A.pvi.width>>1, g_A.pvi.width, g_A.pvi.width, g_A.pvi.height);
		conv444toRGB24(y, g_A.u444, g_A.v444, g_A.picture, g_A.pvi.width, g_A.pvi.width, 
			g_A.pvi.width * 3, g_A.pvi.width, g_A.pvi.height,  g_A.vf->getMatrix(frame), 
			g_A.vf->m_decoder.pc_scale);
	}

	return g_A.picture;
}

extern "C" __declspec(dllexport) void __cdecl closeVideo(void)
{
	g_A.cleanUp();
}

/* 
** These are the new functions with multiple instance support. 
** They use g_LL, a linked list of NoAVSAccess objects. "ident"
** is used to identify the different instances and must be passed
** by the caller.
*/

extern "C" __declspec(dllexport) VideoInfo* __cdecl openMPEG2SourceMI(char* file, int ident)
{
	NoAVSAccess *i, *j;

	if (!g_LL.LLB) 
	{
		g_LL.LLB = new NoAVSAccess();
		g_LL.LLB->ident = ident;
		g_LL.LLE = g_LL.LLB;
	}
	else
	{
		for (i=g_LL.LLB; i; i=i->nxt)
		{
			if (i->ident == ident) 
			{
				dprintf("MPEG2Source:  an instance with that ident already exists!"); 
				return &i->pvi;
			}
		}
		i = new NoAVSAccess();
		i->ident = ident;
		g_LL.LLE->nxt = i;
		i->prv = g_LL.LLE;
		g_LL.LLE = i;
	}

	j = g_LL.LLE;

	char *p;

	// If the D2V filename has _P just before the extension, force
	// progressive upsampling.
	p = file + strlen(file);
	while (*p != '.' && p != file) p--;
	if (p == file) 
	{
		g_LL.remapPointers(j);
		delete j;
		return NULL;
	}
	p -= 2;
	if (p[0] == '_' && p[1] == 'P') j->vfapi_progressive = 1;
	else
	{
		// If the D2V filename has _A just before the extension, force
		// automatic upsampling. Otherwise, force interlaced.
		p = file + strlen(file);
		while (*p != '.' && p != file) p--;
		if (p == file) 
		{
			g_LL.remapPointers(j);
			delete j;
			return NULL;
		}
		p -= 2;
		if (p[0] == '_' && p[1] == 'A') j->vfapi_progressive = -1;
		else j->vfapi_progressive = 0;
	}

	j->vf = new MPEG2Source(file, j->vfapi_progressive == -1 ? 1 : 0);
	if (!j->vf || j->vf->m_decoder.Clip_Width < 0) 
	{
		g_LL.remapPointers(j);
		delete j;
		return NULL;
	}

	j->pvi = j->vf->GetVideoInfo();

	j->buffer = (unsigned char*)malloc(j->pvi.height * j->pvi.width * 2);
	j->picture = (unsigned char*)malloc(j->pvi.height * j->pvi.width * 3);

	j->u422 = (unsigned char*)malloc((j->pvi.height * j->pvi.width)>>1);
	j->u444 = (unsigned char*)malloc(j->pvi.height * j->pvi.width);

	j->v422 = (unsigned char*)malloc((j->pvi.height * j->pvi.width)>>1);
	j->v444 = (unsigned char*)malloc(j->pvi.height * j->pvi.width);

	j->pitch = j->pvi.width;
	
	return &j->pvi;
}

extern "C" __declspec(dllexport) unsigned char* __cdecl getFrameMI(int frame, int ident)
{
	NoAVSAccess *i;

	for (i=g_LL.LLB; i; i=i->nxt)
	{
		if (i->ident == ident) break;
	}
	if (!i) 
	{
		dprintf("MPEG2Source:  no matching instance found (getFrameMI)!");
		return NULL;
	}

	if (i->vf->GetVideoInfo().pixel_type == VideoInfo::CS_YV12)
	{
		i->vf->GetFrame(frame, i->buffer, i->pitch);
	}
	else
	{
		i->vf->GetFrame(frame, i->picture, i->pitch);
		conv422toYUV422(i->picture, i->picture+(i->pitch*i->vf->m_decoder.Clip_Height),
			i->picture+(i->pitch*i->vf->m_decoder.Clip_Height)+((i->pitch*i->vf->m_decoder.Clip_Height)/2),
			i->buffer,i->pitch,i->pitch/2,i->pitch*2,i->vf->m_decoder.Clip_Width,
			i->vf->m_decoder.Clip_Height);
	}

	return i->buffer;
}

extern "C" __declspec(dllexport) unsigned char* __cdecl getRGBFrameMI(int frame, int ident)
{
	NoAVSAccess *i;

	for (i=g_LL.LLB; i; i=i->nxt)
	{
		if (i->ident == ident) break;
	}
	if (!i) 
	{
		dprintf("MPEG2Source:  no matching instance found (getRGBFrameMI)!");
		return NULL;
	}

	i->vf->GetFrame(frame, i->buffer, i->pitch);

	if (i->vf->GetVideoInfo().pixel_type == VideoInfo::CS_YV12)
	{
		unsigned char *y = i->buffer;
		unsigned char *u = y + (i->pitch * i->pvi.height);
		unsigned char *v = u + ((i->pitch * i->pvi.height)/4);
		conv420to422(u, i->u422, i->vfapi_progressive, i->pvi.width>>1, i->pvi.width>>1, 
			i->pvi.width, i->pvi.height);
		conv422to444(i->u422, i->u444, i->pvi.width>>1, i->pvi.width, i->pvi.width, i->pvi.height);
		conv420to422(v, i->v422, i->vfapi_progressive, i->pvi.width>>1, i->pvi.width>>1, 
			i->pvi.width, i->pvi.height);
		conv422to444(i->v422, i->v444, i->pvi.width>>1, i->pvi.width, i->pvi.width, i->pvi.height);
		conv444toRGB24(y, i->u444, i->v444, i->picture, i->pvi.width, i->pvi.width, 
			i->pvi.width * 3, i->pvi.width, i->pvi.height, i->vf->getMatrix(frame), 
			i->vf->m_decoder.pc_scale);
	}
	else
	{
		unsigned char *y = i->buffer;
		unsigned char *u = y + (i->pitch * i->pvi.height);
		unsigned char *v = u + ((i->pitch * i->pvi.height)/2);
		conv422to444(u, i->u444, i->pvi.width>>1, i->pvi.width, i->pvi.width, i->pvi.height);
		conv422to444(v, i->v444, i->pvi.width>>1, i->pvi.width, i->pvi.width, i->pvi.height);
		conv444toRGB24(y, i->u444, i->v444, i->picture, i->pvi.width, i->pvi.width, 
			i->pvi.width * 3, i->pvi.width, i->pvi.height, i->vf->getMatrix(frame), 
			i->vf->m_decoder.pc_scale);
	}

	return i->picture;
}

extern "C" __declspec(dllexport) void __cdecl closeVideoMI(int ident)
{
	NoAVSAccess *i;

	for (i=g_LL.LLB; i; i=i->nxt)
	{
		if (i->ident == ident)
		{
			g_LL.remapPointers(i);
			delete i;
			return;
		}
	}
}

/*
** Standard call versions.
*/
extern "C" __declspec(dllexport) VideoInfo* _stdcall openMPEG2Source_SC(char* file)
{
	return openMPEG2Source(file);
}

extern "C" __declspec(dllexport) unsigned char* _stdcall getFrame_SC(int frame)
{
	return getFrame(frame);
}

extern "C" __declspec(dllexport) unsigned char* _stdcall getRGBFrame_SC(int frame)
{
	return getRGBFrame(frame);
}

extern "C" __declspec(dllexport) void _stdcall closeVideo_SC(void)
{
	closeVideo();
}

/*
** Standard call versions for multi-instance.
*/
extern "C" __declspec(dllexport) VideoInfo* _stdcall openMPEG2Source_SCMI(char* file, int ident)
{
	return openMPEG2SourceMI(file, ident);
}

extern "C" __declspec(dllexport) unsigned char* _stdcall getFrame_SCMI(int frame, int ident)
{
	return getFrameMI(frame, ident);
}

extern "C" __declspec(dllexport) unsigned char* _stdcall getRGBFrame_SCMI(int frame, int ident)
{
	return getRGBFrameMI(frame, ident);
}

extern "C" __declspec(dllexport) void _stdcall closeVideo_SCMI(int ident)
{
	closeVideoMI(ident);
}