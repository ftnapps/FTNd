/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Utilities for tic processing 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "tic.h"
#include "mover.h"
#include "tic.h"
#include "utic.h"


extern	int	tic_bad;
extern	int	do_quiet;


char *MakeTicName()
{
    static char	buf[13];

    buf[12] = '\0';
    sprintf(buf, "%08lx.tic", sequencer());
    buf[0] = 'm';
    buf[1] = 'b';

    return buf;
}



/*
 * Return day in the year, 0..365
 */
int Day_Of_Year()
{
    time_t	Now;
    struct tm	*Tm;

    Now = time(NULL);
    Tm = localtime(&Now);

    return Tm->tm_yday;
}



/*
 * ReArc files in the current directory
 */
int Rearc(char *unarc)
{
    int	    i = 0, j = 0, k = 0;
    char    temp[PATH_MAX], *cmd = NULL;

    Syslog('f', "Entering Rearc(%s)", unarc);

    if (!getarchiver(unarc)) {
	return FALSE;
    }
    cmd = xstrcpy(archiver.farc);
    if (cmd == NULL) {
	WriteError("Rearc(): No arc command available");
	return FALSE;
    }

    while (TIC.NewFile[i] != '.')
	i++;
    i++;

    while (TIC.NewFullName[k] != '.')
	k++;
    k++;

    for (; i < strlen(TIC.NewFile); i++) {
	if (TIC.NewFile[i] > '9') {
	    TIC.NewFile[i] = toupper(unarc[j]);
	    if (isupper(TIC.NewFullName[i]))
		TIC.NewFullName[i] = toupper(unarc[k]);
	    else
		TIC.NewFullName[i] = tolower(unarc[k]);
	}
	j++;
	k++;
    }


    Syslog('f' , "NewFile=\"%s\", NewFullName=\"%s\"", TIC.NewFile, TIC.NewFullName);
	
    sprintf(temp, "%s/%s .", TIC.Inbound, TIC.NewFile);
    if (execute_str(cmd, temp, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
	free(cmd);
	return TRUE;
    }

    /*
     * Restore filenames
     */
    strncpy(TIC.NewFile, TIC.TicIn.File, sizeof(TIC.NewFile) -1);
    strncpy(TIC.NewFullName, TIC.TicIn.FullName, sizeof(TIC.NewFullName) -1);

    free(cmd);
    WriteError("Rearc(%s) Failed", unarc);
    return FALSE;
}



void DeleteVirusWork()
{
    char    *buf, *temp;

    buf  = calloc(PATH_MAX, sizeof(char));
    temp = calloc(PATH_MAX, sizeof(char));
    getcwd(buf, PATH_MAX);
    sprintf(temp, "%s/tmp", getenv("MBSE_ROOT"));

    if (chdir(temp) == 0) {
	Syslog('f', "DeleteVirusWork %s/arc", temp);
	execute_pth((char *)"rm", (char *)"-r -f arc", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
	execute_pth((char *)"mkdir", (char *)"arc", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    } else
	WriteError("$Can't chdir to %s", temp);

    chdir(buf);
    free(temp);
    free(buf);
}



void Bad(char *format, ...)
{
    char    outstr[1024];
    va_list va_ptr;

    va_start(va_ptr, format);
    vsprintf(outstr, format, va_ptr);
    va_end(va_ptr);

    WriteError(outstr);
    MoveBad();
    tic_bad++;
}



void ReCalcCrc(char *fn)
{
    TIC.Crc_Int = file_crc(fn, CFG.slow_util && do_quiet);
    sprintf(TIC.TicIn.Crc, "%08lX", TIC.Crc_Int);
    strcpy(T_File.Crc, TIC.TicIn.Crc);
}



int Get_File_Id()
{
    char    *temp;
    char    Desc[1024];
    FILE    *fp;
    int	    i, j, lines = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/tmp/FILE_ID.DIZ", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	sprintf(temp, "%s/tmp/file_id.diz", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
	    free(temp);
	    return FALSE;
	}
    }

    /*
     * Read no more then 25 lines.
     */
    while (((fgets(Desc, 1023, fp)) != NULL) && (TIC.File_Id_Ct < 25)) {
	lines++;
	/*
	 * Check if the FILE_ID.DIZ is in a normal layout.
	 * The layout should be max. 10 lines of max. 48 characters.
	 * We check at 51 characters and if the lines are longer,
	 * we trash the FILE_ID.DIZ file.
	 */
	if (strlen(Desc) > 51) {
	    fclose(fp);
	    unlink(temp);
	    for (i = 0; i < 25; i++)
		TIC.File_Id[i][0] = '\0';
	    TIC.File_Id_Ct = 0;
	    Syslog('f', "FILE_ID.DIZ line %d is %d chars", lines, strlen(Desc));
	    Syslog('!', "Trashing illegal formatted FILE_ID.DIZ");
	    free(temp);
	    return FALSE;
	}

	if (strlen(Desc) > 0) {
	    j = 0;
	    for (i = 0; i < strlen(Desc); i++) {
		if (isprint(Desc[i])) {
		    TIC.File_Id[TIC.File_Id_Ct][j] = Desc[i];
		    j++;
		    if (j > 47)
			break;
		}
	    }

	    /*
	     * Remove trailing spaces
	     */
	    while (j && isspace(TIC.File_Id[TIC.File_Id_Ct][j-1]))
		j--;
	    TIC.File_Id[TIC.File_Id_Ct][j] = '\0';
	    TIC.File_Id_Ct++;
	}
    }
    fclose(fp);
    unlink(temp);
    free(temp);

    /*
     * Strip empty lines at end of FILE_ID.DIZ
     */
    while ((strlen(TIC.File_Id[TIC.File_Id_Ct-1]) == 0) && (TIC.File_Id_Ct))
	TIC.File_Id_Ct--;

    Syslog('f', "Got %d FILE_ID.DIZ lines", TIC.File_Id_Ct);
    if (TIC.File_Id_Ct)
	return TRUE;
    else
	return FALSE;
}



void UpDateAlias(char *Alias)
{
    char    *path;
    FILE    *fp;

    Syslog('f', "UpDateAlias(%s) with %s", Alias, TIC.NewFile);

    if (!strlen(CFG.req_magic)) {
	WriteError("No magic filename path configured");
	return;
    }

    path = xstrcpy(CFG.req_magic);
    path = xstrcat(path, (char *)"/");
    path = xstrcat(path, Alias);

    if ((fp = fopen(path, "w")) == NULL) {
	WriteError("$Can't create %s", path);
	free(path);
	return;
    }

    fprintf(fp, "%s\n", TIC.NewFile);
    fclose(fp);
    free(path);
}



