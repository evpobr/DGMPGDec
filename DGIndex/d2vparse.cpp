// ParseD2V by Donald A. Graft
// Released under the Gnu Public Licence (GPL)

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "resource.h"

int parse_d2v(HWND hWnd, TCHAR *szInput)
{
	FILE *fp, *wfp;
	TCHAR line[2048], *p;
	int i, fill, val, prev_val = -1, ndx = 0, count = 0, fdom = -1;
	int D2Vformat = 0;
	int vob, cell;
	unsigned int gop_field, field_operation, frame_rate, hour, min;
	double sec;
	TCHAR render[128], temp[20];
	int type;
	
	// Open the D2V file to be parsed.
	fp = _tfopen(szInput, _T("r"));
	if (fp == 0)
	{
		DGShowError(IDS_ERROR_OPEN_D2V_FILE);
		return 0;
	}
	// Mutate the file name to the output text file to receive the parsed data.
	PathRenameExtension(szInput, _T(".parse.txt"));
	// Open the output file.
	wfp = _tfopen(szInput, _T("w"));
	if (wfp == 0)
	{
		DGShowError(IDS_ERROR_CREATE_PARSE_OUTPUT_FILE);
		return 0;
	}

	_ftprintf(wfp, _T("D2V Parse Output\n\n"));
	// Pick up the D2V format number
	_fgetts(line, 2048, fp);
	if (_tcsncmp(line, _T("DGIndexProjectFile"), 18) != 0)
	{
		DGShowError(IDS_ERROR_FILE_IS_NOT_D2V);
		fclose(fp);
		fclose(wfp);
		return 0;
	}
	_stscanf(line, _T("DGIndexProjectFile%d"), &D2Vformat);
	if (D2Vformat != D2V_FILE_VERSION)
	{
		DGShowError(IDS_ERROR_OBSOLETE_D2V);
		fclose(fp);
		fclose(wfp);
		return 0;
	}
	_ftprintf(wfp, _T("Encoded Frame: Display Frames....Flags Byte (* means in 3:2 pattern)\n"));
	_ftprintf(wfp, _T("--------------------------------------------------------------------\n\n"));
	// Get the field operation flag.
	while (_fgetts(line, 2048, fp) != 0)
	{
		if (_tcsncmp(line, _T("Field_Operation"), 8) == 0) break;
	}
	_stscanf(line, _T("Field_Operation=%d"), &field_operation);
	if (field_operation == 1)
		_ftprintf(wfp, _T("[Forced Film, ignoring flags]\n"));
	else if (field_operation == 2)
		_ftprintf(wfp, _T("[Raw Frames, ignoring flags]\n"));
	else
		_ftprintf(wfp, _T("[Field Operation None, using flags]\n"));
	// Get framerate.
	_fgetts(line, 2048, fp);
	_stscanf(line, _T("Frame_Rate=%d"), &frame_rate);
	// Skip past the rest of the D2V header info to the data lines.
	while (_fgetts(line, 2048, fp) != 0)
	{
		if (_tcsncmp(line, _T("Location"), 8) == 0) break;
	}
	while (_fgetts(line, 2048, fp) != 0)
	{
		if (line[0] != _T('\n')) break;
	}
	// Process the data lines.
	do
	{
		// Skip to the TFF/RFF flags entries.
		p = line;
		_stscanf(p, _T("%x"), &gop_field);
		if (gop_field & 0x100)
        {
		    if (gop_field & 0x400)
				_ftprintf(wfp, _T("[GOP: closed]\n"));
            else
				_ftprintf(wfp, _T("[GOP: open]\n"));
        }
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		_stscanf(p, _T("%d"), &vob);
		while (*p++ != _T(' '));
		_stscanf(p, _T("%d"), &cell);
		while (*p++ != _T(' '));
		// Process the flags entries.
		while ((*p >= _T('0') && *p <= _T('9')) || (*p >= _T('a') && *p <= _T('f')))
		{
			_stscanf(p, _T("%x"), &val);
			if (val == 0xff)
			{
				_ftprintf(wfp, _T("[EOF]\n"));
				break;
			}
			// Pick up the frame type.
			type = (val & 0x30) >> 4;
			_stprintf_s(temp, _T("%d [%s]"), ndx, type == 1 ? _T("I") : (type == 2 ? _T("P") : _T("B")));
			// Isolate the TFF/RFF bits.
			val &= 0x3;
			// Determine field dominance.
			if (fdom == -1)
			{
				fdom = (val >> 1) & 1;
				_ftprintf(wfp, _T("[Clip is %s]\n"), fdom ? _T("TFF") : _T("BFF"));
			}

			// Show encoded-to-display mapping.
			if (field_operation == 0)
			{
				if ((count % 2) && (val == 1 || val == 3))
				{
					_stprintf_s(render, _T("%s: %d,%d,%d"), temp, ndx + count/2, ndx + count/2 + 1, ndx + count/2 + 1);
				}
				else if ((count % 2) && !(val == 1 || val == 3))
				{
					_stprintf_s(render, _T("%s: %d,%d"), temp, ndx + count / 2, ndx + count / 2 + 1);
				}
				else if (!(count % 2) && (val == 1 || val == 3))
				{
					_stprintf_s(render, _T("%s: %d,%d,%d"), temp, ndx + count / 2, ndx + count / 2, ndx + count / 2 + 1);
				}
				else if (!(count % 2) && !(val == 1 || val == 3))
				{
					_stprintf_s(render, _T("%s: %d,%d"), temp, ndx + count / 2, ndx + count / 2);
				}
				fill = 32 - DGStrLength(render);
				for (i = 0; i < fill; i++) _tcscat_s(render, _T("."));
				_stprintf_s(temp, _T("%x"), val);
				_ftprintf(wfp, _T("%s%s"), render, temp);
			}
			else
			{
				_stprintf_s(render, _T("%s: %d,%d"), temp, ndx, ndx);
				fill = 32 - DGStrLength(render);
				for (i = 0; i < fill; i++) _tcscat(render, _T("."));
				_stprintf_s(temp, _T("%x"), val);
				_ftprintf(wfp, _T("%s%s"), render, temp);
			}

			// Show vob cell id.
//			printf(" [vob/cell=%d/%d]", vob, cell);

			// Print warnings for 3:2 breaks and field order transitions.
			if ((prev_val >= 0) && ((val == 0 && prev_val == 3) || (val != 0 && val == prev_val + 1)))
			{
				_ftprintf(wfp, _T(" *"));
			}

			if (prev_val >= 0)
			{
				if ((val == 2 && prev_val == 0) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 3 && prev_val == 0) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 0 && prev_val == 1) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 1 && prev_val == 1) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 0 && prev_val == 2) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 1 && prev_val == 2) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 2 && prev_val == 3) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
				else if ((val == 3 && prev_val == 3) || (val == 2 && prev_val == 0))
					_ftprintf(wfp, _T(" [FIELD ORDER TRANSITION!]"));
			}

			_ftprintf(wfp, _T("\n"));

			// Count the encoded frames.
			ndx++;
			// Count the number of pulled-down fields.
			if (val == 1 || val == 3) count++;
			// Move to the next flags entry.
			while (*p != _T(' ') && *p != _T('\n')) p++;
			p++;
			prev_val = val;
		}
	} while ((_fgetts(line, 2048, fp) != 0) &&
		((line[0] >= _T('0') && line[0] <= _T('9')) || (line[0] >= 'a' && line[0] <= 'f')));
	sec = ((float)(ndx + count / 2)) * 1000.0 / frame_rate;
	hour = (int) (sec / 3600);
	sec -= hour * 3600;
	min = (int) (sec / 60);
	sec -= min * 60;
	_ftprintf(wfp, _T("Running time = %d hours, %d minutes, %f seconds\n"), hour, min, sec);

	fclose(fp);
	fclose(wfp);
	return 1;
}

int analyze_sync(HWND hWnd, TCHAR *Input, int audio_id)
{
	FILE *fp, *wfp;
	TCHAR line[2048], *p;
	__int64 vpts, apts;
	int delay, ref;
	double rate, picture_period;
    int leadingBframes;
	TCHAR audio_id_str[10], tmp[20];

	_stprintf_s(audio_id_str, _T(" A%02x"), audio_id);
	fp = _tfopen(Input, _T("r"));
	if (fp == 0)
	{
		DGShowError(IDS_ERROR_OPEN_DUMP_FILE);
		return 0;
	}
	// Check that it is a timestamps dump file
	_fgetts(line, 1024, fp);
	if (_tcsncmp(line, _T("DGIndex Timestamps Dump"), 23) != 0)
	{
		DGShowError(IDS_ERROR_FILE_IN_NOT_TIMESTAMPS_DUMP_FILE);
		fclose(fp);
		return 0;
	}

	// Mutate the file name to the output text file to receive the parsed data.
	p = &szInput[DGStrLength(Input)];
	while (*p != _T('.')) p--;
	p[1] = 0;
	_stprintf_s(tmp, _T("delayT%x.txt"), audio_id);
	_tcscat(p, tmp);
	// Open the output file.
	wfp = _tfopen(szInput, _T("w"));
	if (wfp == 0)
	{
		DGShowError(IDS_ERROR_CREATE_OUTPUT_FILE);
		return 0;
	}
	_ftprintf(wfp, _T("Delay Analysis Output (Audio ID %x)\n\n"), audio_id);

	_fgetts(line, 1024, fp);
	_fgetts(line, 1024, fp);
	p = line;
	while (*p++ != _T('='));
	_stscanf(p, _T("%Lf"), &rate);
	_fgetts(line, 1024, fp);
	p = line;
	while (*p++ != _T('='));
	_stscanf(p, _T("%d"), &leadingBframes);
next_vpts:
	while (_fgetts(line, 1024, fp) != 0)
	{
		if (!_tcsncmp(line, _T("V PTS"), 5))
		{
			p = line;
			while (*p++ != _T('S'));
            vpts = _tstoi64(p);
			while (_fgetts(line, 1024, fp) != 0)
			{
				if (!_tcsncmp(line, _T("Decode picture"), 14))
				{
					p = line + 35;
					_stscanf(p, _T("%d"), &ref);
					while (*p++ != _T('['));
					if (*p != _T('I'))
						goto next_vpts;
					_ftprintf(wfp, _T("%s"), line);
					break;
				}
			}
			picture_period = 1.0 / rate;
			vpts -= (int) (leadingBframes * picture_period * 90000);
			while (_fgetts(line, 1024, fp) != 0)
			{
				if (!_tcsncmp(line, audio_id_str, 4))
				{
					p = line;
					while (*p++ != _T('S'));
                    apts = _tstoi64(p);
					delay = (int) ((apts - vpts) / 90);
					_ftprintf(wfp, _T("delay = %d\n"), delay);
					break;
				}
			}
		}
	}
	fclose(fp);
	fclose(wfp);

	return 1;
}

// Return 1 if a transition was detected in test_only mode and the user wants to correct it, otherwise 0.	
int fix_d2v(HWND hWnd, LPCTSTR Input, int test_only)
{
	FILE *fp, *wfp, *dfp;
	TCHAR line[2048], prev_line[2048], wfile[2048], logfile[2048], *p, *q;
	int val, mval, prev_val, mprev_val, fix;
	bool first, found;
	int D2Vformat = 0;
    unsigned int info;

	fp = _tfopen(Input, _T("r"));
	if (fp == 0)
	{
		DGShowError(IDS_ERROR_OPEN_D2V_FILE);
		return 0;
	}
	// Pick up the D2V format number
	_fgetts(line, 1024, fp);
	if (_tcsncmp(line, _T("DGIndexProjectFile"), 18) != 0)
	{
		DGShowError(IDS_ERROR_FILE_IS_NOT_D2V);
		fclose(fp);
		return 0;
	}
	_stscanf(line, _T("DGIndexProjectFile%d"), &D2Vformat);
	if (D2Vformat != D2V_FILE_VERSION)
	{
		DGShowError(IDS_ERROR_OBSOLETE_D2V);
		fclose(fp);
		return 0;
	}

	if (!test_only)
	{
		_tcscpy_s(wfile, Input);
		_tcscat_s(wfile, _T(".fixed"));
		wfp = _tfopen(wfile, _T("w"));
		if (wfp == 0)
		{
			DGShowError(IDS_ERROR_CREATE_FIXED_D2V_FILE);
			fclose(fp);
			return 0;
		}
		_fputts(line, wfp);
		// Mutate the file name to the output text file to receive processing status information.
		_tcscpy_s(logfile, Input);
		p = &logfile[DGStrLength(logfile)];
		while (*p != _T('.')) p--;
		p[1] = 0;
		_tcscat(p, _T("fix.txt"));
		// Open the output file.
		dfp = _tfopen(logfile, _T("w"));
		if (dfp == 0)
		{
			fclose(fp);
			fclose(wfp);
			DGShowError(IDS_ERROR_CREATE_INFO_OUTPUT_FILE);
			return 0;
		}

		_ftprintf(dfp, _T("D2V Fix Output\n\n"));
	}
	
	while (_fgetts(line, 1024, fp) != 0)
	{
		if (!test_only) _fputts(line, wfp);
		if (_tcsncmp(line, _T("Location"), 8) == 0) break;
	}
	_fgetts(line, 1024, fp);
	if (!test_only) _fputts(line, wfp);
	_fgetts(line, 1024, fp);
	prev_line[0] = 0;
	prev_val = -1;
	mprev_val = -1;
	found = false;
	do
	{
		p = line;
		_stscanf(p, _T("%x"), &info);
        // If it's a progressive sequence then we can't have any transitions.
        if (info & (1 << 9))
            continue;
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		while (*p++ != _T(' '));
		first = true;
		while ((*p >= _T('0') && *p <= _T('9')) || (*p >= _T('a') && *p <= _T('f')))
		{
			fix = -1;
			_stscanf(p, _T("%x"), &val);
			if (val == 0xff) break;
			// Isolate the TFF/RFF bits.
			mval = val & 0x3;
			if (prev_val >= 0) mprev_val = prev_val & 0x3;
			// Detect field order transitions.
			if (mval == 2 && mprev_val == 0)      fix = 1;
			else if (mval == 3 && mprev_val == 0) fix = 1;
			else if (mval == 0 && mprev_val == 1) fix = 0;
			else if (mval == 1 && mprev_val == 1) fix = 0;
			else if (mval == 0 && mprev_val == 2) fix = 3;
			else if (mval == 1 && mprev_val == 2) fix = 3;
			else if (mval == 2 && mprev_val == 3) fix = 2;
			else if (mval == 3 && mprev_val == 3) fix = 2;
			// Correct the field order transition.
			if (fix >= 0)
			{
				found = true;
				if (!test_only) _ftprintf(dfp, _T("Field order transition: %x -> %x\n"), mprev_val, mval);
				if (!test_only) _ftprintf(dfp, prev_line);
				if (!test_only) _ftprintf(dfp, line);
				if (first == false)
				{
					q = p;
					while (*q-- != _T(' '));
				}
				else
				{
					q = prev_line;
					while (*q != _T('\n')) q++;
					while (!((*q >= _T('0') && *q <= _T('9')) || (*q >= _T('a') && *q <= _T('f')))) q--;
				}
				*q = (TCHAR)fix + _T('0');
				if (!test_only) _ftprintf(dfp, _T("corrected...\n"));
				if (!test_only) _ftprintf(dfp, prev_line);
				if (!test_only) _ftprintf(dfp, line);
				if (!test_only) _ftprintf(dfp, _T("\n"));
			}
			while (*p != _T(' ') && *p != _T('\n')) p++;
			p++;
			prev_val = val;
			first = false;
		}
		if (!test_only) _fputts(prev_line, wfp);
		_tcscpy_s(prev_line, line);
		//TODO: may be bug in unicode mode
	} while ((_fgetts(line, 2048, fp) != 0) &&
		((line[0] >= _T('0') && line[0] <= _T('9')) || (line[0] >= _T('a') && line[0] <= _T('f'))));
	if (!test_only) _fputts(prev_line, wfp);
	if (!test_only) _fputts(line, wfp);
	while (_fgetts(line, 2048, fp) != 0)
		if (!test_only) _fputts(line, wfp);
	fclose(fp);
	if (!test_only) fclose(wfp);
	if (test_only)
	{
        if (found == true)
        {
		    if (!CLIActive)
		    {
				if (MessageBox(hWnd, _T("A field order transition was detected.\n")
					_T("It is not possible to decide automatically if this should be corrected.\n")
					_T("Refer to the DGIndex Users Manual for an explanation.\n")
					_T("You can choose to correct it by hitting the Yes button below or\n")
					_T("you can correct it later using the Fix D2V tool.\n\n")
					_T("Correct the field order transition?"),
					_T("Field Order Transition Detected"), MB_YESNO | MB_ICONWARNING) == IDYES)
                    return 1;
                else
                    return 0;
		    }
            else
                return 1;
        }
		return 0;
	}
	if (found == false)
	{
		_ftprintf(dfp, _T("No errors found.\n"));
		fclose(dfp);
		_tunlink(wfile);
		MessageBox(hWnd, _T("No errors found."), _T("Fix D2V"), MB_OK | MB_ICONINFORMATION);
		return 0;
	}
	else
	{
		FILE *bad, *good, *fixed;
		TCHAR c;

		fclose(dfp);
		// Copy the D2V file to *.d2v.bad version.
		good = _tfopen(Input, _T("r"));
		if (good == 0)
			return 0;
		_stprintf_s(line, _T("%s.bad"), Input);
		bad = _tfopen(line, _T("w"));
		if (bad == 0)
		{
			fclose(good);
			return 0;
		}
		while ((c = _fgettc(good)) != EOF) _fputtc(c, bad);
		fclose(good);
		fclose(bad);
		// Copy the *.d2v.fixed version to the D2V file.
		good = _tfopen(Input, _T("w"));
		if (good == 0)
			return 0;
		_stprintf_s(line, _T("%s.fixed"), Input);
		fixed = _tfopen(line, _T("r"));
		while ((c = _fgettc(fixed)) != EOF) _fputtc(c, good);
		fclose(good);
		fclose(fixed);
		// Ditch the *.d2v.fixed version.
		_tunlink(line);
		if (!CLIActive)
		{
			MessageBox(hWnd, _T("Field order corrected. The original version was\nsaved with the extension \".bad\"."), _T("Correct Field Order"), MB_OK | MB_ICONINFORMATION);
			ShellExecute(hDlg, _T("open"), logfile, NULL, NULL, SW_SHOWNORMAL);
		}
	}

	return 0;
}

