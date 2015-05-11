#include "stdafx.h"
#include "global.h"
#include "resource.h"
#include "Application.h"
#include "DGIndex.h"

extern DWORD threadId;
extern HANDLE hProcess, hThread;

STDMETHODIMP CApplication::QueryInterface(REFIID riid, void **ppvObject)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CApplication::AddRef()
{
	return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CApplication::Release(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP CApplication::LoadProject(LPCTSTR pszFileName)
{
	size_t i;
	int m, n;
	unsigned int num, den;
	TCHAR line[2048];
	int D2Vformat;

	D2VFile = _tfopen(szInput, _T("r"));

	// Validate the D2V file.
	_fgetts(line, 2048, D2VFile);
	if (_tcsncmp(line, _T("DGIndexProjectFile"), 18) != 0)
	{
		DGShowError(IDS_ERROR_FILE_IS_NOT_D2V);
		fclose(D2VFile);
		return E_FAIL;
	}
	_stscanf(line, _T("DGIndexProjectFile%d"), &D2Vformat);
	if (D2Vformat != D2V_FILE_VERSION)
	{
		DGShowError(IDS_ERROR_OBSOLETE_D2V);
		fclose(D2VFile);
		return E_FAIL;
	}

	// Close any opened files.
	while (NumLoadedFiles)
	{
		NumLoadedFiles--;
		_close(Infile[NumLoadedFiles]);
		Infile[NumLoadedFiles] = NULL;
	}

	_ftscanf(D2VFile, _T("%d\n"), &NumLoadedFiles);

	i = NumLoadedFiles;
	while (i)
	{
		_fgetts(Infilename[NumLoadedFiles - i], DG_MAX_PATH - 1, D2VFile);
		// Strip newline.
		Infilename[NumLoadedFiles - i][DGStrLength(Infilename[NumLoadedFiles - i]) - 1] = 0;
		if ((Infile[NumLoadedFiles - i] = _topen(Infilename[NumLoadedFiles - i], _O_RDONLY | _O_BINARY | _O_SEQUENTIAL)) == -1)
		{
			while (i<NumLoadedFiles)
			{
				_close(Infile[NumLoadedFiles - i - 1]);
				Infile[NumLoadedFiles - i - 1] = NULL;
				i++;
			}

			NumLoadedFiles = 0;
			break;
		}

		i--;
	}

	Recovery();

	_ftscanf(D2VFile, _T("\nStream_Type=%d\n"), &SystemStream_Flag);
	if (SystemStream_Flag == TRANSPORT_STREAM)
	{
		_ftscanf(D2VFile, _T("MPEG2_Transport_PID=%x,%x,%x\n"),
			&MPEG2_Transport_VideoPID, &MPEG2_Transport_AudioPID, &MPEG2_Transport_PCRPID);
		_ftscanf(D2VFile, _T("Transport_Packet_Size=%d\n"), &TransportPacketSize);
	}
	// Don't use the read value. It will be detected.
	SystemStream_Flag = ELEMENTARY_STREAM;
	_ftscanf(D2VFile, _T("MPEG_Type=%d\n"), &mpeg_type);
	_ftscanf(D2VFile, _T("iDCT_Algorithm=%d\n"), &iDCT_Flag);
	_ftscanf(D2VFile, _T("YUVRGB_Scale=%d\n"), &Scale_Flag);
	setRGBValues();
	_ftscanf(D2VFile, _T("Luminance_Filter=%d,%d\n"), &LumGamma, &LumOffset);

	if (LumGamma || LumOffset)
	{
		CheckMenuItem(hMenu, IDM_LUMINANCE, MF_CHECKED);
		Luminance_Flag = true;
	}
	else
	{
		CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);
		Luminance_Flag = false;
	}

	_ftscanf(D2VFile, _T("Clipping=%d,%d,%d,%d\n"),
		&Clip_Left, &Clip_Right, &Clip_Top, &Clip_Bottom);

	if (Clip_Top || Clip_Bottom || Clip_Left || Clip_Right)
	{
		CheckMenuItem(hMenu, IDM_CROPPING, MF_CHECKED);
		Cropping_Flag = true;
	}
	else
	{
		CheckMenuItem(hMenu, IDM_CROPPING, MF_UNCHECKED);
		Cropping_Flag = false;
	}

	_ftscanf(D2VFile, _T("Aspect_Ratio=%d:%d\n"), &m, &n);
	_ftscanf(D2VFile, _T("Picture_Size=%dx%d\n"), &m, &n);
	_ftscanf(D2VFile, _T("Field_Operation=%d\n"), &FO_Flag);
	_ftscanf(D2VFile, _T("Frame_Rate=%d (%u/%u)\n"), &m, &num, &den);

	CheckFlag();

	if (NumLoadedFiles)
	{
		_ftscanf(D2VFile, _T("Location=%d,%I64x,%d,%I64x\n"),
			&process.leftfile, &process.leftlba, &process.rightfile, &process.rightlba);

		process.startfile = process.leftfile;
		process.startloc = process.leftlba * SECTOR_SIZE;
		process.end = Infiletotal - SECTOR_SIZE;
		process.endfile = process.rightfile;
		process.endloc = process.rightlba* SECTOR_SIZE;
		process.run = 0;
		process.start = process.startloc;
		process.trackleft = (process.startloc*TRACK_PITCH / Infiletotal);
		process.trackright = (process.endloc*TRACK_PITCH / Infiletotal);
		process.locate = LOCATE_INIT;
		InvalidateRect(hwndSelect, NULL, TRUE);

		if (!threadId || WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0)
			hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CApplication::Quit()
{
	return E_NOTIMPL;
}