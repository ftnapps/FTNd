/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: .TIC files magic processing. 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbtic.h"
#include "tic.h"
#include "utic.h"
#include "magic.h"


long	MagicNr;		/* Current magic record number		    */
int	Magics = 0;		/* Processed magics			    */



char *Magic_Macro(int);
char *Magic_Macro(int C)
{
	static char	buf[PATH_MAX];

	buf[0] = '\0';
	switch(toupper(C)) {
		case 'F':
			sprintf(buf, "%s/%s", TIC.BBSpath, TIC.NewName);
			break;

		case 'P':
			sprintf(buf, "%s", TIC.BBSpath);
			break;

		case 'N':
			sprintf(buf, "%s", strtok(strdup(TIC.NewName), "."));
			break;

		case 'E':
			sprintf(buf, "%s", strrchr(TIC.NewName, '.'));
			break;

		case 'L':
			sprintf(buf, "%s", strrchr(TIC.NewName, '.'));
			buf[0] = buf[1];
			buf[1] = buf[2];
			buf[2] = '\0';
			break;

		case 'D':
			sprintf(buf, "%03d", Day_Of_Year());
			break;

		case 'C':
			sprintf(buf, "%03d", Day_Of_Year());
			buf[0] = buf[1];
			buf[1] = buf[2];
			buf[2] = '\0';
			break;

		case 'A':
			sprintf(buf, "%s", TIC.TicIn.Area);
			break;
	}

	Syslog('f', "Mgc Macro(%c): \"%s\"", C, buf);
	return buf;
}




int GetMagicRec(int Typ, int First)
{
    int	    Eof = FALSE, DoMagic = TRUE;
    int	    i;
    char    *temp, *Magic, *p, *q, mask[256];
    FILE    *FeM;

    if (First)
	MagicNr = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/magic.data", getenv("MBSE_ROOT"));
    if ((FeM = fopen(temp, "r")) == NULL) {
	Syslog('+', "Huh? No magic file? (%s)", temp);
	free(temp);
	return FALSE;
    }

    fread(&magichdr, sizeof(magichdr), 1, FeM);

    do {
	if (fseek(FeM, magichdr.hdrsize + (MagicNr * magichdr.recsize), SEEK_SET) != 0) {
	    WriteError("$Can't seek record %ld in %s", MagicNr, temp);
	    free(temp);
	    fclose(FeM);
	    return FALSE;
	}

	MagicNr++;

	if (fread(&magic, magichdr.recsize, 1, FeM) == 1) {

	    if ((magic.Active) && (magic.Attrib == Typ) && (strcasecmp(magic.From, TIC.TicIn.Area) == 0)) {

//		p = tl(magic.Mask);
		Magic = xstrcpy(magic.Mask);
		p = tl(Magic);
		q = mask;
		*q++ = '^';
		while ((*p) && (q < (mask + sizeof(mask) - 4))) {
		    switch(*p) {
			case '\\':  *q++ = '\\'; *q++ = '\\'; break;
			case '?':   *q++ = '.'; break;
			case '.':   *q++ = '\\'; *q++ = '.'; break;
			case '+':   *q++ = '\\'; *q++ = '+'; break;
			case '*':   *q++ = '.'; *q++ = '*'; break;
			default:    *q++ = toupper(*p); break;
		    }
		    p++;
		}
		*q++ = '$';
		*q = '\0';
		Syslog('f', "Magic mask \"%s\" -> \"%s\"", MBSE_SS(Magic), MBSE_SS(mask));
		if ((re_comp(mask)) != NULL) {
		    if (re_exec(TIC.NewName))
			Syslog('f', "Should matched using regexp");
		    else
			Syslog('f', "No match using regexp");
		} else {
		    Syslog('f', "re_comp returned NULL");
		}
		free(Magic);

		/*
		 * Comparing of the filename must be done in 
		 * two parts, before and after the dot.
		 */
		if (strlen(magic.Mask) == strlen(TIC.NewName)) {
		    for (i = 0; i < strlen(magic.Mask); i++) {
			switch (magic.Mask[i]) {
			    case '?':	break;
			    case '@':	if (!isalpha(TIC.NewName[i]))
					    DoMagic = FALSE;
					break;
			    case '#':	if (!isdigit(TIC.NewName[i]))
					    DoMagic = FALSE;
					break;

			    default:	if (toupper(TIC.NewName[i]) != toupper(magic.Mask[i]))
					    DoMagic = FALSE;
			}
		    }
		} else {
		    DoMagic = FALSE;
		}

		Syslog('f', "Old test, found %s", DoMagic ? "True":"False");

		if (DoMagic) {
		    fclose(FeM);
		    free(temp);
		    return TRUE;
		}
	    }

	} else {
	    Eof = TRUE;
	}

    } while (!Eof);

    free(temp);
    fclose(FeM);
    return FALSE;
}



void MagicResult(char *format, ...)
{
	char	*outputstr;
	va_list	va_ptr;

	outputstr = calloc(1024, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outputstr, format, va_ptr);
	va_end(va_ptr);

	Syslog('+', "Magic: %s", outputstr);
	free(outputstr);
	Magics++;
}



void Magic_CheckCompile(void);
void Magic_CheckCompile(void)
{
	if (magic.Compile) {
		CompileNL = TRUE;
		Syslog('+', "Magic: Trigger Compile Nodelists");
	}
}



int Magic_MoveFile(void)
{
	if (GetMagicRec(MG_MOVE, TRUE)) {
		strcpy(TIC.TicIn.Area, magic.ToArea);
		MagicResult((char *)"Move %s to Area %s", TIC.RealName, TIC.TicIn.Area);
		return TRUE;	
	} else
		return FALSE;
}



void Magic_ExecCommand(void)
{
	int	i, j, k, Err, First = TRUE;
	char	Line[256];
	char	Temp[PATH_MAX];
	char	*cmd, *opts;

	while (GetMagicRec(MG_EXEC, First)) {
		First = FALSE;
		j = 0;
		memset(&Line, 0, sizeof(Line));
		for (i = 0; i < strlen(magic.Cmd); i++) {
			if (magic.Cmd[i] != '%') {
				Line[j] = magic.Cmd[i];
				j++;
			} else {
				i++;
				if (magic.Cmd[i] == '%') {
					Line[j] = '%';
					j++;
				} else {
					Temp[0] = '\0';
					sprintf(Temp, "%s", Magic_Macro(magic.Cmd[i]));
					for (k = 0; k < strlen(Temp); k++) {
						Line[j] = Temp[k];
						j++;
					}
				}
			}
		}

		cmd = xstrcpy(getenv("MBSE_ROOT"));
		cmd = xstrcat(cmd, (char *)"/bin/");
		cmd = xstrcat(cmd, strtok(Line, " "));
		opts = strtok(NULL, "\0");
		MagicResult((char *)"Exec: \"%s %s\"", cmd, opts);
	
		if ((Err = execute(cmd, opts, NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null")) == 0) {
			Magics++;
		} else
			Syslog('!', "Mgc Exec: (%s %s) returns %d", cmd, opts, Err);

		Magic_CheckCompile();
	}
}



void Magic_CopyFile(void)
{
	int	First = TRUE;
	char	*From, *To;

	From = calloc(PATH_MAX, sizeof(char));
	To   = calloc(PATH_MAX, sizeof(char));

	while (GetMagicRec(MG_COPY, First)) {
		First = FALSE;
		sprintf(From, "%s/%s", TIC.BBSpath, TIC.NewName);
		sprintf(To, "%s/%s", magic.Path, TIC.NewName);

		if (file_cp(From, To) == 0) {
			MagicResult((char *)"%s copied to %s", From, To);
			Magic_CheckCompile();
		} else
			WriteError("Magic: copy: %s to %s failed");
	}

	free(From);
	free(To);
}



void Magic_UnpackFile(void)
{
    int	rc, First = TRUE;
    char	*buf = NULL, *unarc = NULL, *cmd = NULL;
    char	Fn[PATH_MAX];

    while (GetMagicRec(MG_UNPACK, First)) {
	First = FALSE;
	buf = calloc(PATH_MAX, sizeof(char));
	getcwd(buf, 128);

	if (chdir(magic.Path) == 0) {
	    sprintf(Fn, "%s/%s", TIC.BBSpath, TIC.NewName);
	    if ((unarc = unpacker(Fn)) != NULL) {
		if (getarchiver(unarc)) {
		    cmd = xstrcpy(archiver.funarc);
		    if (strlen(cmd)) {
			rc = execute(cmd, Fn, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
			sync();
			if (rc)
			    WriteError("$Magic: unpack in %s, error %d", magic.Path, rc);
			else
			    MagicResult((char *)"unpacked in %s", magic.Path);
		    }
		    free(cmd);
		}
	    }

	    chdir(buf);
	    Magic_CheckCompile();
	} else
	    WriteError("$Magic: unpack: can't chdir \"%s\"", magic.Path);

	free(buf);
    }
}



void Magic_Keepnum(void)
{
	if (GetMagicRec(MG_KEEPNUM, TRUE)) {
		TIC.KeepNum = magic.KeepNum;
		MagicResult((char *)"Keep %d files", TIC.KeepNum);
	}
}



void Magic_UpDateAlias(void)
{
	if (GetMagicRec(MG_UPDALIAS, TRUE)) {
		UpDateAlias(TIC.TicIn.Area);
		MagicResult((char *)"Update Alias");
	}
}



void Magic_AdoptFile(void)
{
	int	First = TRUE;
	char	*temp;
	FILE	*Tf;

	temp = calloc(PATH_MAX, sizeof(char));

	while (GetMagicRec(MG_ADOPT, First)) {
		First = FALSE;

		if (SearchTic(magic.ToArea)) {
			MagicResult((char *)"Adoptfile in %s", magic.ToArea);

			sprintf(temp, "%s/%s", TIC.Inbound, MakeTicName());
			if ((Tf = fopen(temp, "a+")) == NULL)
				WriteError("$Can't create %s", temp);
			else {
				fprintf(Tf, "Hatch\r\n");
				fprintf(Tf, "NoMove\r\n");
				fprintf(Tf, "Created MBSE BBS v%s, %s\r\n", VERSION, SHORTRIGHT);
				fprintf(Tf, "Area %s\r\n", magic.ToArea);
				fprintf(Tf, "Origin %s\r\n", aka2str(tic.Aka));
				fprintf(Tf, "From %s\r\n", aka2str(tic.Aka));
				if (strlen(TIC.TicIn.Replace))
					fprintf(Tf, "Replaces %s\r\n", TIC.TicIn.Replace);
				if (strlen(TIC.TicIn.Magic))
					fprintf(Tf, "Magic %s\r\n", TIC.TicIn.Magic);
				fprintf(Tf, "File %s\r\n", TIC.NewName);
				fprintf(Tf, "Pth %s\r\n", TIC.BBSpath);
				fprintf(Tf, "Desc %s\r\n", TIC.TicIn.Desc);
				fprintf(Tf, "Crc %s\r\n", TIC.TicIn.Crc);
				fprintf(Tf, "Pw %s\r\n", CFG.hatchpasswd);
				fclose(Tf);
			}

			SearchTic(TIC.TicIn.Area);
		} else
			WriteError("Mgc Adopt: Area \"%s\" not found");
	}

	free(temp);
}



int Magic_DeleteFile(void)
{
	int	Result;

	if ((Result = GetMagicRec(MG_DELETE, TRUE)))
		MagicResult((char *)"Delete file");

	return Result;
}



