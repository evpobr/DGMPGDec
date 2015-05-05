#include <windows.h>
#include "resource.h"
#include "Shlwapi.h"
#include "global.h"

int parse_cli(LPTSTR lpCmdLine, LPTSTR ucCmdLine)
{
	TCHAR cwd[DG_MAX_PATH];
	int i;

	if (!_tcsstr(lpCmdLine, _T("=")))
	{
		// UNIX-style CLI.
		TCHAR *p = lpCmdLine, *q;
		TCHAR *f, name[DG_MAX_PATH], *ptr;
		int in_quote = 0;
		int tmp;
		int enable_demux = 0;
		TCHAR opt[32], *o;
		TCHAR suffix[DG_MAX_PATH];
		int val;

		// CLI invocation.
		NumLoadedFiles = 0;
		CLIPreview = 0;
		ExitOnEnd = 0;
		hadRGoption = 0;

		while (1)
		{
			while (*p == _T(' ') || *p == _T('\t')) p++;
			if (*p == _T('-'))
			{
				p++;
				o = opt;
				while (*p != _T(' ') && *p != _T('\t') && *p != 0)
					*o++ = *p++;
				*o = 0;
				if (!_tcsncmp(opt, _T("i"), 3))
				{
another:
					while (*p == _T(' ') || *p == _T('\t')) p++;
					f = name;
					while (1)
					{
						if ((in_quote == 0) && (*p == _T(' ') || *p == _T('\t') || *p == 0))
							break;
						if ((in_quote == 1) && (*p == 0))
							break;
						if (*p == _T('"'))
						{
							if (in_quote == 0)
							{
								in_quote = 1;
								p++;
							}
							else
							{
								in_quote = 0;
								p++;
								break;
							}
						}
						*f++ = *p++;
					}
					*f = 0;
					/* If the specified file does not include a path, use the
					   current directory. */
					if (name[0] != _T('\\') && name[1] != _T(':'))
					{
						GetCurrentDirectory(_countof(cwd) - 1, cwd);
						_tcscat_s(cwd, _T("\\"));
						_tcscat_s(cwd, name);
					}
					else
					{
						_tcscpy_s(cwd, name);
					}
					if ((tmp = _topen(cwd, _O_RDONLY | _O_BINARY)) != -1)
					{
						_tcscpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;
					}
					if (*p != 0 && *(p + 1) != _T('-'))
						goto another;
					Recovery();
					RefreshWindow(true);
				}
				if (!_tcsncmp(opt, _T("ai"), 3))
				{
					while (*p == _T(' ') || *p == _T('\t')) p++;
					f = name;
					while (1)
					{
						if ((in_quote == 0) && (*p == _T(' ') || *p == _T('\t') || *p == 0))
							break;
						if ((in_quote == 1) && (*p == 0))
							break;
						if (*p == _T('"'))
						{
							if (in_quote == 0)
							{
								in_quote = 1;
								p++;
							}
							else
							{
								in_quote = 0;
								p++;
								break;
							}
						}
						*f++ = *p++;
					}
					*f = 0;
					for (;;)
					{
						/* If the specified file does not include a path, use the
						   current directory. */
						if (name[0] != _T('\\') && name[1] != _T(':'))
						{
							GetCurrentDirectory(_countof(cwd) - 1, cwd);
							_tcscat_s(cwd, _T("\\"));
							_tcscat_s(cwd, name);
						}
						else
						{
							_tcscpy_s(cwd, name);
						}
						if ((tmp = _topen(cwd, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL)) == -1)
							break;
						_tcscpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;

						// First scan back from the end of the name for an _ character.
						ptr = name + DGStrLength(name);
						while (*ptr != _T('_') && ptr >= name) ptr--;
						if (*ptr != _T('_')) break;
						// Now pick up the number value and increment it.
						ptr++;
						//TODO: may be bug in unicode mode
						if (*ptr < _T('0') || *ptr > _T('9')) break;
						_stscanf(ptr, _T("%d"), &val);
						val++;
						// Save the suffix after the number.
						q = ptr;
						//TODO: may be bug in unicode mode
						while (*ptr >= _T('0') && *ptr <= _T('9')) ptr++;
						_tcscpy_s(suffix, ptr);
						// Write the new incremented number.
						_stprintf(q, _T("%d"), val);
						// Append the saved suffix.
						_tcscat_s(name, suffix);
					}
					Recovery();
					RefreshWindow(true);
				}
				else if (!_tcsncmp(opt, _T("rg"), 3))
				{
					while (*p == _T(' ') || *p == _T('\t')) p++;
					_stscanf_s(p, _T("%d %I64x %d %I64x\n"),
						&process.leftfile, &process.leftlba, &process.rightfile, &process.rightlba);
					while (*p != _T('-') && *p != 0) p++;
					p--;

					process.startfile = process.leftfile;
					process.startloc = process.leftlba * SECTOR_SIZE;
					process.endfile = process.rightfile;
					process.endloc = (process.rightlba - 1) * SECTOR_SIZE;

					process.run = 0;
					for (i=0; i<process.leftfile; i++)
						process.run += Infilelength[i];
					process.trackleft = ((process.run + process.leftlba * SECTOR_SIZE) * TRACK_PITCH / Infiletotal);

					process.run = 0;
					for (i=0; i<process.rightfile; i++)
						process.run += Infilelength[i];
					process.trackright = ((process.run + (__int64)process.rightlba*SECTOR_SIZE)*TRACK_PITCH/Infiletotal);

					process.end = 0;
					for (i=0; i<process.endfile; i++)
						process.end += Infilelength[i];
					process.end += process.endloc;

					hadRGoption = 1;
				}
				else if (!_tcsncmp(opt, _T("vp"), 3))
				{
					while (*p == _T(' ') || *p == _T('\t')) p++;
					_stscanf(p, _T("%x"), &MPEG2_Transport_VideoPID);
					while (*p != _T('-') && *p != 0) p++;
					p--;
				}
				else if (!_tcsncmp(opt, _T("ap"), 3))
				{
					while (*p == _T(' ') || *p == _T('\t')) p++;
					_stscanf(p, _T("%x"), &MPEG2_Transport_AudioPID);
					while (*p != _T('-') && *p != 0) p++;
					p--;
				}
				else if (!_tcsncmp(opt, _T("pp"), 3))
				{
					while (*p == _T(' ') || *p == _T('\t')) p++;
					_stscanf(p, _T("%x"), &MPEG2_Transport_PCRPID);
					while (*p != _T('-') && *p != 0) p++;
					p--;
				}
				else if (!_tcsncmp(opt, _T("ia"), 3))
				{
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
 					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					while (*p == _T(' ') || *p == _T('\t')) p++;
		   
					//TODO: may be bug in unicode mode
					switch (*p++)
					{
					case _T('1'):
					  iDCT_Flag = IDCT_MMX;
					  break;
					case _T('2'):
					  iDCT_Flag = IDCT_SSEMMX;
					  break;
					default:
					case _T('3'):
					  iDCT_Flag = IDCT_SSE2MMX;
					  break;
					case _T('4'):
					  iDCT_Flag = IDCT_FPU;
					  break;
					case _T('5'):
					  iDCT_Flag = IDCT_REF;
					  break;
					case _T('6'):
					  iDCT_Flag = IDCT_SKAL;
					  break;
					case _T('7'):
					  iDCT_Flag = IDCT_SIMPLE;
					  break;
					}
				}
				else if (!_tcsncmp(opt, _T("fo"), 3))
				{
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
					SetDlgItemText(hDlg, IDC_INFO, _T(""));
					while (*p == _T(' ') || *p == _T('\t')) p++;

					//TODO: may be bug in unicode mode
					switch (*p++)
					{
					default:
					case _T('0'):
					  FO_Flag = FO_NONE;
					  CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
					  break;
					case _T('1'):
					  FO_Flag = FO_FILM;
					  CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
					  break;
					case _T('2'):
					  FO_Flag = FO_RAW;
					  CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
					  break;
					}
				}
				else if (!_tcsncmp(opt, _T("yr"), 3))
				{
					CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
					while (*p == _T(' ') || *p == _T('\t')) p++;
		    
					switch (*p++)
					{
					default:
					case _T('1'):
					  Scale_Flag = true;
					  setRGBValues();    
					  CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
					  break;
		      
					case _T('2'):
					  Scale_Flag = false;
					  setRGBValues();
					  CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
					  break;
					}
				}
				else if (!_tcsncmp(opt, _T("tn"), 3))
				{
					TCHAR track_list[1024];
					unsigned int i, audio_id;
					// First get the track list into Track_List.
					while (*p == _T(' ') || *p == _T('\t')) p++;
					_tcscpy_s(track_list, p);
					while (*p != _T('-') && *p != 0) p++;
					ptr = track_list;
					while (*ptr != _T(' ') && *ptr != 0)
						ptr++;
					*ptr = 0;
					_tcscpy_s(Track_List, track_list);
					// Now parse it and enable the specified audio ids for demuxing.
					for (i = 0; i < 0xc8; i++)
						audio[i].selected_for_demux = false;
					ptr = Track_List;
					//TODO: may be bug in unicode
					while ((*ptr >= _T('0') && *ptr <= _T('9')) || (*ptr >= _T('a') && *ptr <= _T('f')) || (*ptr >= _T('A') && *ptr <= _T('F')))
					{
						_stscanf(ptr, _T("%x"), &audio_id);
						if (audio_id > 0xc7)
							break;
						audio[audio_id].selected_for_demux = true;
						while (*ptr != _T(',') && *ptr != 0) ptr++;
						if (*ptr == 0)
							break;
						ptr++;
					}

					Method_Flag = AUDIO_DEMUX;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				}
				else if (!_tcsncmp(opt, _T("om"), 3))
				{
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					while (*p == _T(' ') || *p == _T('\t')) p++;

					switch (*p++)
					{
					default:
					case _T('0'):
						Method_Flag = AUDIO_NONE;
						CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
						break;
		      
					case _T('1'):
						Method_Flag = AUDIO_DEMUX;
						CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
						break;
		     
					case _T('2'):
						Method_Flag = AUDIO_DEMUXALL;
						CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
						break;
		     
					case _T('3'):
						Method_Flag = AUDIO_DECODE;
						CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
						break;
					}
				}
				else if (!_tcsncmp(opt, _T("drc"), 3))
				{
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					while (*p == _T(' ') || *p == _T('\t')) p++;

					switch (*p++)
					{
					default:
					case _T('0'):
					  CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
					  DRC_Flag = DRC_NONE;
					  break;
					case _T('1'):
					  CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
					  DRC_Flag = DRC_LIGHT;
					  break;
					case _T('2'):
					  CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
					  DRC_Flag = DRC_NORMAL;
					  break;
					case _T('3'):
					  CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
					  DRC_Flag = DRC_HEAVY;
					  break;
					}
				}
				else if (!_tcsncmp(opt, _T("dsd"), 3))
				{
					CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
					while (*p == _T(' ') || *p == _T('\t')) p++;

					DSDown_Flag = *p++ - _T('0');
					if (DSDown_Flag)
					  CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);
				}
				else if (!_tcsncmp(opt, _T("dsa"), 3))
				{
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					while (*p == _T(' ') || *p == _T('\t')) p++;

					switch (*p++)
					{
					default:
					case _T('0'):
						SRC_Flag = SRC_NONE;
						CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
						break;
					case _T('1'):
						SRC_Flag = SRC_LOW;
						CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
						break;
					case _T('2'):
						SRC_Flag = SRC_MID;
						CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
						break;
					case _T('3'):
						SRC_Flag = SRC_HIGH;
						CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
						break;
					case _T('4'):
						SRC_Flag = SRC_UHIGH;
						CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
						break;
					}
				}
				else if (!_tcsncmp(opt, _T("pre"), 3))
				{
					CLIPreview = 1;
				}
				else if (!_tcsncmp(opt, _T("at"), 3))
				{
					FILE *bf;

					while (*p == _T(' ') || *p == _T('\t')) p++;
					if (*p == _T('-') || *p == 0)
					{
						// A null file name specifies no template.
						AVSTemplatePath[0] = 0;
						p--;
					}
					else
					{
						f = name;
						while (1)
						{
							if ((in_quote == 0) && (*p == _T(' ') || *p == _T('\t') || *p == 0))
								break;
							if ((in_quote == 1) && (*p == 0))
								break;
							if (*p == _T('"'))
							{
								if (in_quote == 0)
								{
									in_quote = 1;
									p++;
								}
								else
								{
									in_quote = 0;
									p++;
									break;
								}
							}
							*f++ = *p++;
						}
						*f = 0;
						/* If the specified file does not include a path, use the
						   current directory. */
						if (name[0] != _T('\\') && name[1] != _T(':'))
						{
							GetCurrentDirectory(_countof(cwd) - 1, cwd);
							_tcscat_s(cwd, _T("\\"));
							_tcscat_s(cwd, name);
						}
						else
						{
							_tcscpy_s(cwd, name);
						}
						// Check that the specified template file exists and is readable.
						bf = _tfopen(cwd, _T("r"));
						if (bf != 0)
						{
							// Looks good; save the path.
							fclose(bf);
							_tcscpy_s(AVSTemplatePath, cwd);
						}
						else
						{
							// Something is wrong, so don't use a template.
							AVSTemplatePath[0] = 0;
						}
					}
				}
				else if (!_tcsncmp(opt, _T("exit"), 4))
				{
					ExitOnEnd = 1;
				}
				else if (!_tcsncmp(opt, _T("o"), 3) || !_tcsncmp(opt, _T("od"), 3))
				{
					// Set up demuxing if requested.
					if (p[-1] == _T('d'))
					{
						MuxFile = (FILE *) 0;
					}
					while (*p == _T(' ') || *p == _T('\t')) p++;

					// Don't pop up warning boxes for automatic invocation.
					crop1088_warned = true;
					CLIActive = 1;
					f = name;
					while (1)
					{
						if ((in_quote == 0) && (*p == _T(' ') || *p == _T('\t') || *p == 0))
							break;
						if ((in_quote == 1) && (*p == 0))
							break;
						if (*p == _T('"'))
						{
							if (in_quote == 0)
							{
								in_quote = 1;
								p++;
							}
							else
							{
								in_quote = 0;
								p++;
								break;
							}
						}
						*f++ = *p++;
					}
					*f = 0;
					/* If the specified file does not include a path, use the
					   current directory. */
					if (name[0] != _T('\\') && name[1] != _T(':'))
					{
						GetCurrentDirectory(_countof(szOutput) - 1, szOutput);
						_tcscat_s(szOutput, _T("\\"));
						_tcscat_s(szOutput, name);
					}
					else
					{
						_tcscpy_s(szOutput, name);
					}
				}
			}
			else if (*p == 0)
				break;
			else
				break;
		}
		if (NumLoadedFiles == 0 && WindowMode == SW_HIDE)
		{
			MessageBox(hWnd, _T("Couldn't open input file in HIDE mode! Exiting."), NULL, MB_OK | MB_ICONERROR);
			return -1;
		}
		if (!CLIActive && WindowMode == SW_HIDE)
		{
			MessageBox(hWnd, _T("No output file in HIDE mode! Exiting."), NULL, MB_OK | MB_ICONERROR);
			return -1;
		}
		CheckFlag();
	}

	else if(*lpCmdLine != 0)
	{
		// Old-style CLI.
		int tmp;
        int hadRGoption = 0;
		TCHAR delimiter1[2], delimiter2[2];
		TCHAR *ende, save;
		TCHAR *ptr, *fptr, *p, *q;
		TCHAR aFName[DG_MAX_PATH];
		TCHAR suffix[DG_MAX_PATH];
		int val;

		NumLoadedFiles = 0;
		delimiter1[0] = _T('[');
		delimiter2[0] = _T(']');
		delimiter1[1] = delimiter2[1] = 0;
		if ((ptr = _tcsstr(ucCmdLine, _T("-SET-DELIMITER="))) || (ptr = _tcsstr(ucCmdLine, _T("-SD="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			while (*ptr++ != _T('='));
			delimiter1[0] = delimiter2[0] = *ptr;
		}
		if ((ptr = _tcsstr(ucCmdLine, _T("-AUTO-INPUT-FILES="))) || (ptr = _tcsstr(ucCmdLine, _T("-AIF="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = _tcsstr(ptr, delimiter1) + 1;
			ende = _tcsstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			_tcscpy_s(aFName, ptr);
			*ende = save;
			for (;;)
			{
				/* If the specified file does not include a path, use the
				   current directory. */
				if (!_tcsstr(aFName, _T("\\")))
				{
					GetCurrentDirectory(_countof(cwd) - 1, cwd);
					_tcscat_s(cwd, _T("\\"));
					_tcscat_s(cwd, aFName);
				}
				else
				{
					_tcscpy_s(cwd, aFName);
				}
				if ((tmp = _topen(cwd, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL)) == -1) break;
				_tcscpy(Infilename[NumLoadedFiles], cwd);
				Infile[NumLoadedFiles] = tmp;
				NumLoadedFiles++;

				// First scan back from the end of the name for an _ character.
				p = aFName+DGStrLength(aFName);
				while (*p != _T('_') && p >= aFName) p--;
				if (*p != _T('_')) break;
				// Now pick up the number value and increment it.
				p++;
				//TODO: may be bug in unicode
				if (*p < _T('0') || *p > _T('9')) break;
				_stscanf(p, _T("%d"), &val);
				val++;
				// Save the suffix after the number.
				q = p;
				//TODO: may be bug in unicode
				while (*p >= _T('0') && *p <= _T('9')) p++;
				_tcscpy(suffix, p);
				// Write the new incremented number.
				_stprintf(q, _T("%d"), val);
				// Append the saved suffix.
				_tcscat(aFName, suffix);
			}
		}
		else if ((ptr = _tcsstr(ucCmdLine, _T("-INPUT-FILES="))) || (ptr = _tcsstr(ucCmdLine, _T("-IF="))))
		{
		  ptr = lpCmdLine + (ptr - ucCmdLine);
		  ptr  = _tcsstr(ptr, delimiter1) + 1;
		  ende = _tcsstr(ptr + 1, delimiter2);
  
		  do
		  {
			i = 0;
			if ((fptr = _tcsstr(ptr, _T(","))) || (fptr = _tcsstr(ptr + 1, delimiter2)))
			{
			  while (ptr < fptr)
			  {
				aFName[i] = *ptr;
				ptr++;
				i++;
			  }
			  aFName[i] = 0x00;
			  ptr++;
			}
			else
			{
			  _tcscpy(aFName, ptr);
			  ptr = ende;
			}

			/* If the specified file does not include a path, use the
			   current directory. */
			if (!_tcsstr(aFName, _T("\\")))
			{
				GetCurrentDirectory(_countof(cwd) - 1, cwd);
				_tcscat_s(cwd, _T("\\"));
				_tcscat_s(cwd, aFName);
			}
			else
			{
				_tcscpy_s(cwd, aFName);
			}
			if ((tmp = _topen(cwd, _O_RDONLY | _O_BINARY)) != -1)
			{
				_tcscpy(Infilename[NumLoadedFiles], cwd);
				Infile[NumLoadedFiles] = tmp;
				NumLoadedFiles++;
			}
		  }
		  while (ptr < ende);
		}
		else if ((ptr = _tcsstr(ucCmdLine, _T("-BATCH-FILES="))) || (ptr = _tcsstr(ucCmdLine, _T("-BF="))))
		{
			FILE *bf;
			TCHAR line[1024];

			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = _tcsstr(ptr, delimiter1) + 1;
			ende = _tcsstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			_tcscpy_s(aFName, ptr);
			*ende = save;
			/* If the specified batch file does not include a path, use the
			   current directory. */
			if (!_tcsstr(aFName, _T("\\")))
			{
				GetCurrentDirectory(_countof(cwd) - 1, cwd);
				_tcscat_s(cwd, _T("\\"));
				_tcscat_s(cwd, aFName);
			}
			else
			{
				_tcscpy_s(cwd, aFName);
			}
			bf = _tfopen(cwd, _T("r"));
			if (bf != 0)
			{
				while (_fgetts(line, 1023, bf) != 0)
				{
					// Zap the newline.
					line[DGStrLength(line)-1] = 0;
					/* If the specified batch file does not include a path, use the
					   current directory. */
					if (!_tcsstr(line, _T("\\")))
					{
						GetCurrentDirectory(_countof(cwd) - 1, cwd);
						_tcscat_s(cwd, _T("\\"));
						_tcscat_s(cwd, line);
					}
					else
					{
						_tcscpy_s(cwd, line);
					}
					if ((tmp = _topen(cwd, _O_RDONLY | _O_BINARY)) != -1)
					{
						_tcscpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;
					}
				}
			}
		}
		Recovery();
		if ((ptr = _tcsstr(ucCmdLine, _T("-RANGE="))) || (ptr = _tcsstr(ucCmdLine, _T("-RG="))))
		{
            ptr = lpCmdLine + (ptr - ucCmdLine);
			while (*ptr++ != _T('='));
  
			_stscanf(ptr, _T("%d/%I64x/%d/%I64x\n"),
	            &process.leftfile, &process.leftlba, &process.rightfile, &process.rightlba);

		    process.startfile = process.leftfile;
		    process.startloc = process.leftlba * SECTOR_SIZE;
		    process.endfile = process.rightfile;
		    process.endloc = (process.rightlba - 1) * SECTOR_SIZE;

			process.run = 0;
			for (i=0; i<process.leftfile; i++)
				process.run += Infilelength[i];
			process.trackleft = ((process.run + process.leftlba * SECTOR_SIZE) * TRACK_PITCH / Infiletotal);

            process.run = 0;
			for (i=0; i<process.rightfile; i++)
				process.run += Infilelength[i];
			process.trackright = ((process.run + (__int64)process.rightlba*SECTOR_SIZE)*TRACK_PITCH/Infiletotal);

		    process.end = 0;
		    for (i=0; i<process.endfile; i++)
			    process.end += Infilelength[i];
		    process.end += process.endloc;

//			SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
            hadRGoption = 1;
        }

		if (NumLoadedFiles == 0 && WindowMode == SW_HIDE)
		{
			MessageBox(hWnd, _T("Couldn't open input file in HIDE mode! Exiting."), NULL, MB_OK | MB_ICONERROR);
			return -1;
		}

		// Transport PIDs
		if ((ptr = _tcsstr(ucCmdLine, _T("-VIDEO-PID="))) || (ptr = _tcsstr(ucCmdLine, _T("-VP="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			_stscanf(_tcsstr(ptr, _T("=")) + 1, _T("%x"), &MPEG2_Transport_VideoPID);
		}
		if ((ptr = _tcsstr(ucCmdLine, _T("-AUDIO-PID="))) || (ptr = _tcsstr(ucCmdLine, _T("-AP="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			_stscanf(_tcsstr(ptr, _T("=")) + 1, _T("%x"), &MPEG2_Transport_AudioPID);
		}
		if ((ptr = _tcsstr(ucCmdLine, _T("-PCR-PID="))) || (ptr = _tcsstr(ucCmdLine, _T("-PP="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			_stscanf(_tcsstr(ptr, _T("=")) + 1, _T("%x"), &MPEG2_Transport_PCRPID);
		}

		//iDCT Algorithm
		if ((ptr = _tcsstr(ucCmdLine, _T("-IDCT-ALGORITHM="))) || (ptr = _tcsstr(ucCmdLine, _T("-IA="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
 			CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
   
			switch (*(_tcsstr(ptr, _T("=")) + 1))
			{
			case _T('1'):
			  iDCT_Flag = IDCT_MMX;
			  break;
			case _T('2'):
			  iDCT_Flag = IDCT_SSEMMX;
			  break;
			default:
			case _T('3'):
			  iDCT_Flag = IDCT_SSE2MMX;
			  break;
			case _T('4'):
			  iDCT_Flag = IDCT_FPU;
			  break;
			case _T('5'):
			  iDCT_Flag = IDCT_REF;
			  break;
			case _T('6'):
			  iDCT_Flag = IDCT_SKAL;
			  break;
			case _T('7'):
			  iDCT_Flag = IDCT_SIMPLE;
			  break;
			}
			CheckFlag();
		}

		//Field-Operation
		if ((ptr = _tcsstr(ucCmdLine, _T("-FIELD-OPERATION="))) || (ptr = _tcsstr(ucCmdLine, _T("-FO="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			SetDlgItemText(hDlg, IDC_INFO, _T(""));

			switch (*(_tcsstr(ptr, _T("=")) + 1))
			{
			default:
			case _T('0'):
			  FO_Flag = FO_NONE;
			  CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
			  break;
			case _T('1'):
			  FO_Flag = FO_FILM;
			  CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
			  break;
			case _T('2'):
			  FO_Flag = FO_RAW;
			  CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
			  break;
			}
		}

		//YUV->RGB
		if ((ptr = _tcsstr(ucCmdLine, _T("-YUV-RGB="))) || (ptr = _tcsstr(ucCmdLine, _T("-YR="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);    
			CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
    
			switch (*(_tcsstr(ptr, _T("=")) + 1))
			{
			default:
			case _T('1'):
			  Scale_Flag = true;
			  setRGBValues();    
			  CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
			  break;
      
			case _T('2'):
			  Scale_Flag = false;
			  setRGBValues();
			  CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
			  break;
			}
		}

		// Luminance filter and cropping not implemented

		//Track number
		if ((ptr = _tcsstr(ucCmdLine, _T("-TRACK-NUMBER="))) || (ptr = _tcsstr(ucCmdLine, _T("-TN="))))
		{
            TCHAR track_list[1024], *p;
            unsigned int i, audio_id;
            // First get the track list into Track_List.
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr = _tcsstr(ptr, _T("=")) + 1;
            _tcscpy_s(track_list, ptr);
            p = track_list;
			while (*p != _T(' ') && *p != 0)
				p++;
            *p = 0;
            _tcscpy_s(Track_List, track_list);
            // Now parse it and enable the specified audio ids for demuxing.
            for (i = 0; i < 0xc8; i++)
                audio[i].selected_for_demux = false;
            p = Track_List;
			//TODO: may be bug in unicode
			while ((*p >= _T('0') && *p <= _T('9')) || (*p >= _T('a') && *p <= _T('f')) || (*p >= _T('A') && *p <= _T('F')))
            {
				_stscanf(p, _T("%x"), &audio_id);
                if (audio_id > 0xc7)
                    break;
                audio[audio_id].selected_for_demux = true;
				while (*p != _T(',') && *p != 0) p++;
                if (*p == 0)
                    break;
                p++;
            }

			Method_Flag = AUDIO_DEMUX;
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
		}

		// Output Method
		if ((ptr = _tcsstr(ucCmdLine, _T("-OUTPUT-METHOD="))) || (ptr = _tcsstr(ucCmdLine, _T("-OM="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
			switch (*(_tcsstr(ptr, _T("=")) + 1))
			{
			default:
			case _T('0'):
				Method_Flag = AUDIO_NONE;
				CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				break;
      
			case _T('1'):
				Method_Flag = AUDIO_DEMUX;
				CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				break;
     
			case _T('2'):
				Method_Flag = AUDIO_DEMUXALL;
				CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				break;
     
			case _T('3'):
				Method_Flag = AUDIO_DECODE;
				CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
				break;
			}
		}

		// Dynamic-Range-Control
		if ((ptr = _tcsstr(ucCmdLine, _T("-DYNAMIC-RANGE-CONTROL="))) || (ptr = _tcsstr(ucCmdLine, _T("-DRC="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
			switch (*(_tcsstr(ptr, _T("=")) + 1))
			{
			default:
			case _T('0'):
			  CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
			  DRC_Flag = DRC_NONE;
			  break;
			case _T('1'):
			  CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
			  DRC_Flag = DRC_LIGHT;
			  break;
			case _T('2'):
			  CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
			  DRC_Flag = DRC_NORMAL;
			  break;
			case _T('3'):
			  CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
			  DRC_Flag = DRC_HEAVY;
			  break;
			}
		}

		// Dolby Surround Downmix
		if ((ptr = _tcsstr(ucCmdLine, _T("-DOLBY-SURROUND-DOWNMIX="))) || (ptr = _tcsstr(ucCmdLine, _T("-DSD="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
			//TODO: may be bug in unicode
			DSDown_Flag = *(_tcsstr(ptr, _T("=")) + 1) - _T('0');
			if (DSDown_Flag)
			  CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);
		}

		// 48 -> 44 kHz
		if ((ptr = _tcsstr(ucCmdLine, _T("-DOWNSAMPLE-AUDIO="))) || (ptr = _tcsstr(ucCmdLine, _T("-DSA="))))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
			switch (*(_tcsstr(ptr, _T("=")) + 1))
			{
			default:
			case _T('0'):
				SRC_Flag = SRC_NONE;
				CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
				break;
			case _T('1'):
				SRC_Flag = SRC_LOW;
				CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
				break;
			case _T('2'):
				SRC_Flag = SRC_MID;
				CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
				break;
			case _T('3'):
				SRC_Flag = SRC_HIGH;
				CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
				break;
			case _T('4'):
				SRC_Flag = SRC_UHIGH;
				CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
				break;
			}
		}

		// Normalization not implemented

		RefreshWindow(true);

		// AVS Template
		if ((ptr = _tcsstr(ucCmdLine, _T("-AVS-TEMPLATE="))) || (ptr = _tcsstr(ucCmdLine, _T("-AT="))))
		{
			FILE *bf;

			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = _tcsstr(ptr, delimiter1) + 1;
			if (ptr == (TCHAR *) 1 || *ptr == delimiter2[0])
			{
				// A null file name specifies no template.
				AVSTemplatePath[0] = 0;
			}
			else
			{
				ende = _tcsstr(ptr + 1, delimiter2);
				save = *ende;
				*ende = 0;
				_tcscpy(aFName, ptr);
				*ende = save;
				/* If the specified template file does not include a path, use the
				   current directory. */
				if (!_tcsstr(aFName, _T("\\")))
				{
					GetCurrentDirectory(_countof(cwd) - 1, cwd);
					_tcscat_s(cwd, _T("\\"));
					_tcscat_s(cwd, aFName);
				}
				else
				{
					_tcscpy_s(cwd, aFName);
				}
				// Check that the specified template file exists and is readable.
				bf = _tfopen(cwd, _T("r"));
				if (bf != 0)
				{
					// Looks good; save the path.
					fclose(bf);
					_tcscpy(AVSTemplatePath, cwd);
				}
				else
				{
					// Something is wrong, so don't use a template.
					AVSTemplatePath[0] = 0;
				}
			}
		}

		// Output D2V file
		if ((ptr = _tcsstr(ucCmdLine, _T("-OUTPUT-FILE="))) || (ptr = _tcsstr(ucCmdLine, _T("-OF="))) ||
			(ptr = _tcsstr(ucCmdLine, _T("-OUTPUT-FILE-DEMUX="))) || (ptr = _tcsstr(ucCmdLine, _T("-OFD="))))
		{
			// Set up demuxing if requested.
			if (_tcsstr(ucCmdLine, _T("-OUTPUT-FILE-DEMUX=")) || _tcsstr(ucCmdLine, _T("-OFD=")))
			{
				MuxFile = (FILE *) 0;
			}

			// Don't pop up warning boxes for automatic invocation.
			crop1088_warned = true;
			CLIActive = 1;
			ExitOnEnd = _tcsstr(ucCmdLine, _T("-EXIT")) ? 1 : 0;
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = _tcsstr(ptr, delimiter1) + 1;
			ende = _tcsstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			_tcscpy_s(aFName, ptr);
			*ende = save;
            // We need to store the full path, so that all our path handling options work
            // the same way as for GUI mode.
			if (aFName[0] != _T('\\') && aFName[1] != _T(':'))
			{
				//TODO: Use PathCombine
				GetCurrentDirectory(_countof(szOutput) - 1, szOutput);
				_tcscat_s(szOutput, _T("\\"));
				_tcscat_s(szOutput, aFName);
			}
			else
			{
				_tcscpy_s(szOutput, aFName);
			}
		}

		// Preview mode for generating the Info log file
		CLIPreview = _tcsstr(ucCmdLine, _T("-PREVIEW")) ? 1 : 0;

		if (!CLIActive && WindowMode == SW_HIDE)
		{
			MessageBox(hWnd, _T("No output file in HIDE mode! Exiting."), NULL, MB_OK | MB_ICONERROR);
			return -1;
		}
	}
	return 0;
}