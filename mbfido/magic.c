/*****************************************************************************
 *
 * $Id: magic.c,v 1.27 2005/11/12 12:52:30 mbse Exp $
 * Purpose ...............: .TIC files magic processing. 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "orphans.h"
#include "tic.h"
#include "utic.h"
#include "magic.h"


int	MagicNr;		/* Current magic record number		    */
int	Magics = 0;		/* Processed magics			    */



char *Magic_Macro(int);
char *Magic_Macro(int C)
{
    static char	buf[PATH_MAX];

    buf[0] = '\0';
    switch(toupper(C)) {
	case 'F':   snprintf(buf, PATH_MAX, "%s/%s", TIC.BBSpath, TIC.NewFile);
		    break;
	case 'P':   snprintf(buf, PATH_MAX, "%s", TIC.BBSpath);
		    break;
	case 'N':   snprintf(buf, PATH_MAX, "%s", strtok(strdup(TIC.NewFile), "."));
		    break;
	case 'E':   snprintf(buf, PATH_MAX, "%s", strrchr(TIC.NewFile, '.'));
		    break;
	case 'L':   snprintf(buf, PATH_MAX, "%s", strrchr(TIC.NewFile, '.'));
		    buf[0] = buf[1];
		    buf[1] = buf[2];
		    buf[2] = '\0';
		    break;
	case 'D':   snprintf(buf, 3, "%03d", Day_Of_Year());
		    break;
	case 'C':   snprintf(buf, 3, "%03d", Day_Of_Year());
		    buf[0] = buf[1];
		    buf[1] = buf[2];
		    buf[2] = '\0';
		    break;
	case 'A':   snprintf(buf, PATH_MAX, "%s", TIC.TicIn.Area);
		    break;
    }

    Syslog('f', "Mgc Macro(%c): \"%s\"", C, buf);
    return buf;
}




int GetMagicRec(int Typ, int First)
{
    int	    Eof = FALSE;
    char    *temp, mask[256];
    FILE    *FeM;

    if (First)
	MagicNr = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/magic.data", getenv("MBSE_ROOT"));
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

		memset(&mask, 0, sizeof(mask));
		strcpy(mask, re_mask(magic.Mask, FALSE));
		if ((re_comp(mask)) == NULL) {
		    if (re_exec(TIC.NewFile)) {
			fclose(FeM);
			free(temp);
			return TRUE;
		    }
		} else {
		    WriteError("Magic: re_comp(%s) failed", mask);
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
    char    *outputstr;
    va_list va_ptr;

    outputstr = calloc(1024, sizeof(char));

    va_start(va_ptr, format);
    vsnprintf(outputstr, 1024, format, va_ptr);
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
	MagicResult((char *)"Move %s to Area %s", TIC.NewFile, TIC.TicIn.Area);
	return TRUE;	
    } else
	return FALSE;
}



void Magic_ExecCommand(void)
{
    int	    i, j, k, Err, First = TRUE;
    char    Line[256], Temp[PATH_MAX], *cmd, *opts;

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
		    snprintf(Temp, PATH_MAX, "%s", Magic_Macro(magic.Cmd[i]));
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
	
	if ((Err = execute_str(cmd, opts, NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null")) == 0) {
	    Magics++;
	} else
	    Syslog('!', "Mgc Exec: (%s %s) returns %d", cmd, opts, Err);

	Magic_CheckCompile();
    }
}



void Magic_CopyFile(void)
{
    int	    First = TRUE, rc;
    char    *From, *To;

    From = calloc(PATH_MAX, sizeof(char));
    To   = calloc(PATH_MAX, sizeof(char));

    while (GetMagicRec(MG_COPY, First)) {
	First = FALSE;
	snprintf(From, PATH_MAX, "%s/%s", TIC.BBSpath, TIC.NewFile);
	snprintf(To, PATH_MAX, "%s/%s", magic.Path, TIC.NewFile);

	if ((rc = file_cp(From, To) == 0)) {
	    MagicResult((char *)"%s copied to %s", From, To);
	    Magic_CheckCompile();
	} else
	    WriteError("Magic: copy: %s to %s failed, %s", strerror(rc));
    }

    free(From);
    free(To);
}



void Magic_UnpackFile(void)
{
    int	    rc, First = TRUE;
    char    *Fn, *buf = NULL, *unarc = NULL, *cmd = NULL;

    Fn = calloc(PATH_MAX, sizeof(char));
    while (GetMagicRec(MG_UNPACK, First)) {
	First = FALSE;
	buf = calloc(PATH_MAX, sizeof(char));
	getcwd(buf, 128);

	if (chdir(magic.Path) == 0) {
	    snprintf(Fn, PATH_MAX, "%s/%s", TIC.BBSpath, TIC.NewFile);
	    if ((unarc = unpacker(Fn)) != NULL) {
		if (getarchiver(unarc)) {
		    cmd = xstrcpy(archiver.munarc);
		    if (strlen(cmd)) {
			rc = execute_str(cmd, Fn, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
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
    free(Fn);
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
	magic_update(TIC.TicIn.Area, TIC.NewFile);
	MagicResult((char *)"Update Alias");
    }
}



void Magic_AdoptFile(void)
{
    int	    First = TRUE;
    char    *temp;
    FILE    *Tf;

    temp = calloc(PATH_MAX, sizeof(char));

    while (GetMagicRec(MG_ADOPT, First)) {
	First = FALSE;

	if (SearchTic(magic.ToArea)) {
	    MagicResult((char *)"Adoptfile in %s", magic.ToArea);

	    snprintf(temp, PATH_MAX, "%s/%s", TIC.Inbound, MakeTicName());
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
		fprintf(Tf, "File %s\r\n", TIC.NewFile);
		if (strlen(TIC.NewFullName))
		    fprintf(Tf, "Fullname %s\r\n", TIC.NewFullName);
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



