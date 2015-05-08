/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#pragma once

#ifdef GLOBAL
#define XTN
#else
#define XTN extern
#endif

// Messages to the window procedure.
#define CLI_RIP_MESSAGE				(WM_APP)
#define D2V_DONE_MESSAGE			(WM_APP + 1)
#define CLI_PREVIEW_DONE_MESSAGE	(WM_APP + 2)
#define PROGRESS_MESSAGE			(WM_APP + 3)

/* code definition */
#define PICTURE_START_CODE			0x100
#define SLICE_START_CODE_MIN		0x101
#define SLICE_START_CODE_MAX		0x1AF
#define USER_DATA_START_CODE		0x1B2
#define SEQUENCE_HEADER_CODE		0x1B3
#define EXTENSION_START_CODE		0x1B5
#define SEQUENCE_END_CODE			0x1B7
#define GROUP_START_CODE			0x1B8

#define SYSTEM_END_CODE				0x1B9
#define PACK_START_CODE				0x1BA
#define SYSTEM_START_CODE			0x1BB

#define VIDEO_ELEMENTARY_STREAM		0x1E0

#define PRIVATE_STREAM_1			0x1BD
#define BLURAY_STREAM_1			    0x1FD
#define PRIVATE_STREAM_2			0x1BF
#define AUDIO_ELEMENTARY_STREAM_0	0x1C0
#define AUDIO_ELEMENTARY_STREAM_1	0x1C1
#define AUDIO_ELEMENTARY_STREAM_2	0x1C2
#define AUDIO_ELEMENTARY_STREAM_3	0x1C3
#define AUDIO_ELEMENTARY_STREAM_4	0x1C4
#define AUDIO_ELEMENTARY_STREAM_5	0x1C5
#define AUDIO_ELEMENTARY_STREAM_6	0x1C6
#define AUDIO_ELEMENTARY_STREAM_7	0x1C7

#define SUB_SUB						0x20
#define SUB_AC3						0x80
#define SUB_DTS						0x88
#define SUB_PCM						0xA0

#define ELEMENTARY_STREAM 0
#define MPEG1_PROGRAM_STREAM 1
#define MPEG2_PROGRAM_STREAM 2

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

#define SECTOR_SIZE				2048
#define BUFFER_SIZE				2048
#define MAX_FILE_NUMBER			512
#define MAX_PICTURES_PER_GOP	500
#define MAX_GOPS				1000000

#define IDCT_MMX		1
#define IDCT_SSEMMX		2
#define IDCT_SSE2MMX	3
#define	IDCT_FPU		4
#define IDCT_REF		5
#define IDCT_SKAL		6
#define IDCT_SIMPLE		7

#define LOCATE_INIT			0
#define LOCATE_FORWARD		1
#define LOCATE_BACKWARD		-1
#define LOCATE_SCROLL		2
#define LOCATE_RIP			3
#define LOCATE_PLAY			4
#define LOCATE_DEMUX_AUDIO	5

#define CHANNEL				8

#define TRACK_1			0
#define TRACK_2			1
#define TRACK_3			2
#define TRACK_4			3
#define TRACK_5			4
#define TRACK_6			5
#define TRACK_7			6
#define TRACK_8			7

#define FORMAT_AC3			1
#define FORMAT_MPA			2
#define FORMAT_LPCM			3
#define FORMAT_LPCM_M2TS	4
#define FORMAT_DTS			5
#define FORMAT_AAC			6

#define AUDIO_NONE			0
#define AUDIO_DEMUX			1
#define AUDIO_DEMUXALL		2
#define AUDIO_DECODE		3

#define DRC_NONE		0
#define DRC_LIGHT		1
#define DRC_NORMAL		2
#define DRC_HEAVY		3

#define FO_NONE			0
#define FO_FILM			1
#define FO_RAW			2

#define SRC_NONE		0
#define SRC_LOW			1
#define SRC_MID			2
#define SRC_HIGH		3
#define SRC_UHIGH		4

#define TRACK_PITCH		30000

#define DG_MAX_PATH 2048

typedef struct {
	int			gop_start;
	int			type;
	int			file;
	__int64		lba;
	__int64		position;
	bool		pf;
	bool		trf;
    int         picture_structure;
}	D2VData;
XTN D2VData d2v_backward, d2v_forward, d2v_current;

XTN __int64 gop_positions[MAX_GOPS];
XTN int gop_positions_ndx;

typedef struct {
	TCHAR					filename[DG_MAX_PATH];
	FILE					*file;
    bool                    selected_for_demux;
	bool					rip;
    int                     type;
	// The different types use subsets of the following variables.
	unsigned int			layer;
	unsigned int			mode;
	unsigned int			sample;
	unsigned int			rate;
	int						size;
	int						delay;
    unsigned char			format;
	unsigned short			format_m2ts;
} AudioStream;
XTN AudioStream audio[256];

XTN int Sound_Max;

typedef struct {
	__int64					run;
	__int64					start;
	__int64					end;
	__int64					trackleft;
	__int64					trackright;
	int						locate;
	int						startfile;
	__int64					startloc;
	int						endfile;
	__int64					endloc;
	int						file;
	__int64					lba;
	int						leftfile;
	__int64					leftlba;
	int						rightfile;
	__int64					rightlba;
} Process;
XTN Process process;

typedef struct {
	unsigned int			op;
	unsigned int			mi;
	unsigned int			ed;
} Timing;
XTN Timing timing;

typedef struct{
	bool					mmx;
	bool					_3dnow;
	bool					ssemmx;
	bool					ssefpu;
	bool					sse2;
} Cpu;
XTN Cpu cpu;

/* decoder operation control flags */
XTN bool Check_Flag;
XTN bool D2V_Flag;
XTN bool AudioOnly_Flag;
XTN unsigned int AudioPktCount;
XTN bool Display_Flag;
XTN int Fault_Flag;
XTN int CurrentFile;
XTN int NumLoadedFiles;
XTN int FO_Flag;
XTN int iDCT_Flag;
XTN bool Info_Flag;
XTN bool Pause_Flag;
XTN bool Scale_Flag;
XTN bool Start_Flag;
XTN bool Stop_Flag;
XTN bool HadAudioPTS;
XTN int SystemStream_Flag;
#define ELEMENTARY_STREAM 0
#define PROGRAM_STREAM 1
#define TRANSPORT_STREAM 2
#define PVA_STREAM 3
XTN int program_stream_type;
XTN __int64 PackHeaderPosition;

XTN int LeadingBFrames;
XTN int ForceOpenGops;
XTN TCHAR AVSTemplatePath[DG_MAX_PATH];
XTN TCHAR BMPPathString[DG_MAX_PATH];
XTN int FullPathInFiles;
XTN int LoopPlayback;
XTN int FusionAudio;
XTN int UseMPAExtensions;
XTN int NotifyWhenDone;

XTN bool Luminance_Flag;
XTN bool Cropping_Flag;
XTN int Clip_Width, Clip_Height; 

XTN int Method_Flag;
XTN TCHAR Track_List[255];
XTN TCHAR Delay_Track[255];
XTN int DRC_Flag;
XTN bool DSDown_Flag;
XTN bool Decision_Flag;
XTN int SRC_Flag;
XTN bool Norm_Flag;
XTN int Norm_Ratio;
XTN double PreScale_Ratio;

/* DirectDraw & GDI resources */
XTN HMENU hMenu;
XTN HDC hDC;

/* Global Value */
XTN int CLIActive;
XTN char CLIPreview;
XTN char ExitOnEnd;
XTN FILE *D2VFile;
XTN TCHAR D2VFilePath[DG_MAX_PATH];
XTN TCHAR AudioFilePath[DG_MAX_PATH];
XTN unsigned int LowestAudioId;
XTN int VOB_ID, CELL_ID;
XTN FILE *MuxFile;
XTN int HadAddDialog;
XTN int hadRGoption;
#define D2V_FILE_VERSION 16

#define MAX_LOADSTRING	1024

XTN TCHAR g_szMessage[MAX_LOADSTRING];

void DGShowError(UINT nTextID, UINT nCaptionID);
void DGShowError(UINT nTextID);
void DGShowWarning(UINT nTextID, UINT nCaptionID);
void DGShowWarning(UINT nTextID);
BOOL DGSetDlgItemText(HWND hDlg, int nIDDlgItem, UINT nStringID);

void LoadSettings(LPCTSTR pszIniPath);
void SaveSettings(LPCTSTR pszIniPath);

#ifdef DG_MIN_CRT

#define DGStrLength lstrlen

#else

#define DGStrLength _tcslen

#endif


XTN int WindowMode;
XTN HWND hWnd, hDlg, hTrack;
XTN HWND hwndSelect;
XTN TCHAR szInput[MAX_FILE_NUMBER*DG_MAX_PATH], szOutput[DG_MAX_PATH], szBuffer[DG_MAX_PATH], szSave[DG_MAX_PATH];

XTN unsigned char *backward_reference_frame[3], *forward_reference_frame[3];
XTN unsigned char *auxframe[3], *current_frame[3];
XTN unsigned char *u422, *v422, *u444, *v444, *rgb24, *rgb24small, *yuy2, *lum;
XTN __int64 RGB_Scale, RGB_Offset, RGB_CRV, RGB_CBU, RGB_CGX;
XTN int LumGamma, LumOffset;

XTN	unsigned int elapsed, remain;
XTN int playback, frame_repeats, field_repeats, Old_Playback;
XTN int PlaybackSpeed, OldPlaybackSpeed;
XTN bool RightArrowHit;
#define SPEED_SINGLE_STEP	0
#define SPEED_SUPER_SLOW	1
#define SPEED_SLOW			2
#define SPEED_NORMAL		3
#define SPEED_FAST			4
#define SPEED_MAXIMUM		5

XTN int HDDisplay;
#define HD_DISPLAY_FULL_SIZED       0
#define HD_DISPLAY_SHRINK_BY_HALF	1
#define HD_DISPLAY_TOP_LEFT     	2
#define HD_DISPLAY_TOP_RIGHT		3
#define HD_DISPLAY_BOTTOM_LEFT		4
#define HD_DISPLAY_BOTTOM_RIGHT		5

XTN unsigned int Frame_Number;
XTN int Coded_Picture_Width, Coded_Picture_Height;
XTN int block_count, Second_Field;
XTN int horizontal_size, vertical_size, mb_width, mb_height;

XTN double frame_rate, Frame_Rate;
XTN unsigned int fr_num, fr_den;
XTN int FILM_Purity, VIDEO_Purity, Bitrate_Monitor;
XTN double Bitrate_Average;
XTN double max_rate;

XTN int Clip_Left, Clip_Right, Clip_Top, Clip_Bottom;

XTN int Infile[MAX_FILE_NUMBER];
XTN TCHAR *Infilename[MAX_FILE_NUMBER];
XTN __int64 Infilelength[MAX_FILE_NUMBER];
XTN __int64	Infiletotal;

XTN int intra_quantizer_matrix[64];
XTN int intra_quantizer_matrix_log[64];
XTN int non_intra_quantizer_matrix[64];
XTN int non_intra_quantizer_matrix_log[64];
XTN int chroma_intra_quantizer_matrix[64];
XTN int chroma_intra_quantizer_matrix_log[64];
XTN int chroma_non_intra_quantizer_matrix[64];
XTN int chroma_non_intra_quantizer_matrix_log[64];
XTN int full_pel_forward_vector;
XTN int full_pel_backward_vector;
XTN int forward_f_code;
XTN int backward_f_code;

XTN int q_scale_type;
XTN int alternate_scan;
XTN int quantizer_scale;

XTN short *block[8];

/* ISO/IEC 13818-2 section 6.2.2.1:  sequence_header() */
XTN int aspect_ratio_information;

/* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
XTN int progressive_sequence;
XTN int chroma_format;

/* sequence_display_extension() */
XTN int display_horizontal_size;
XTN int display_vertical_size;

/* ISO/IEC 13818-2 section 6.2.2.6:  group_of_pictures_header() */
XTN int closed_gop;

/* ISO/IEC 13818-2 section 6.2.3: picture_header() */
XTN int temporal_reference;
XTN int picture_coding_type;
XTN int progressive_frame;
XTN int PTSAdjustDone;

XTN int matrix_coefficients;
XTN bool default_matrix_coefficients;

/* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header */
XTN int f_code[2][2];
XTN int picture_structure;
XTN int frame_pred_frame_dct;
XTN int concealment_motion_vectors;
XTN int intra_dc_precision;
XTN int top_field_first;
XTN int repeat_first_field;
XTN int intra_vlc_format;

#ifdef UNICODE

#define strverscmp StrCmpLogicalW

#else

XTN int strverscmp(const TCHAR *s1, const TCHAR *s2);

#endif

/* getbit.c */
XTN void UpdateInfo(void);

/* gethdr.c */
XTN int Get_Hdr(int);
XTN void sequence_header(void);
XTN int slice_header(void);
XTN bool GOPSeen;

/* getpic.c */
XTN void Decode_Picture(void);
XTN void WriteD2VLine(int);

/* gui.cpp */
#define MISC_KILL 0
#define END_OF_DATA_KILL 1
XTN void ThreadKill(int);
XTN bool crop1088_warned, crop1088;
XTN int LogQuants_Flag;
XTN FILE *Quants;
XTN int LogTimestamps_Flag;
XTN int StartLogging_Flag;
XTN FILE *Timestamps;

/* gui */

/* idct */
extern "C" void __fastcall MMX_IDCT(short *block);
extern "C" void __fastcall SSEMMX_IDCT(short *block);
extern "C" void __fastcall SSE2MMX_IDCT(short *block);
XTN void Initialize_FPU_IDCT(void);
XTN void FPU_IDCT(short *block);
XTN void __fastcall REF_IDCT(short *block);
extern "C" void __fastcall Skl_IDct16_Sparse_SSE(short *block);
extern "C" void __fastcall simple_idct_mmx(short *block);

/* motion.c */
XTN void motion_vectors(int PMV[2][2][2], int dmvector[2], int motion_vertical_field_select[2][2], 
	int s, int motion_vector_count, int mv_format, int h_r_size, int v_r_size, int dmv, int mvscale);
XTN void Dual_Prime_Arithmetic(int DMV[][2], int *dmvector, int mvx, int mvy);

/* mpeg2dec.c */
XTN DWORD WINAPI MPEG2Dec(LPVOID n);
XTN int initial_parse(TCHAR *input_file, int *mpeg_type_p, int *is_pgrogram_stream_p);
XTN void setRGBValues();
#define IS_NOT_MPEG 0
#define IS_MPEG1 1
#define IS_MPEG2 2
XTN int mpeg_type;
XTN int is_program_stream;

/* store.c */
XTN void Write_Frame(unsigned char *src[], D2VData d2v, DWORD frame);
XTN int DetectVideoType(int frame, int rff);
XTN void ShowFrame(bool move);

static TCHAR *AspectRatio[] = {
	_T(""), _T("1:1"), _T("4:3"), _T("16:9"), _T("2.21:1")
};

static TCHAR *AspectRatioMPEG1[] = {
	_T(""), _T("1:1"), _T("0.6735"), _T("16:9,625"), _T("0.7615"), _T("0.8055"), _T("16:9,525"), _T("0.8935"), _T("4:3,625"), _T("0.9815"), _T("1.0255"),
	_T("1.0695"), _T("4:3,525"), _T("1.575"), _T("1.2015")
};

XTN int TransportPacketSize;
XTN int MPEG2_Transport_VideoPID;
XTN int MPEG2_Transport_AudioPID;
XTN int MPEG2_Transport_PCRPID;
XTN int MPEG2_Transport_AudioType;
#define PID_DETECT_RAW 0
#define PID_DETECT_PATPMT 1
#define PID_DETECT_PSIP 2
XTN int Pid_Detect_Method;
// XTN PATParser pat_parser;

struct DCTtab {
	char run, level, len;
};

struct VLCtab {
	char val, len;
};

#define ERROR_VALUE	(-1)

/* zig-zag and alternate scan patterns */
extern unsigned char scan[2][64];

/* non-linear quantization coefficient table */
extern unsigned char Non_Linear_quantizer_scale[32];
