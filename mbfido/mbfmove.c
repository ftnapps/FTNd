/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Move a file
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "mbfutil.h"
#include "mbfmove.h"



extern int	do_quiet;		/* Suppress screen output	    */



/*
 * Move a file
 */
void Move(int From, int To, char *File)
{
    char		*frompath, *topath, *temp1, *temp2, *fromlink, *tolink, *fromthumb, *tothumb;
    struct FILERecord	fdb;
    FILE		*fp1, *fp2;
    int			rc = FALSE, Found = FALSE;

    IsDoing("Move file");
    colour(LIGHTRED, BLACK);

    if (From == To) {
	WriteError("Area numbers are the same");
	if (!do_quiet)
	    printf("Can't move to the same area\n");
	die(MBERR_COMMANDLINE);
    }

    /*
     * Check From area
     */
    if (LoadAreaRec(From) == FALSE) {
	WriteError("Can't load record %d", From);
	die(MBERR_INIT_ERROR);
    }
    if (!area.Available) {
	WriteError("Area %d not available", From);
	if (!do_quiet)
	    printf("Area %d not available\n", From);
	die(MBERR_COMMANDLINE);
    }
    if (area.CDrom) {
	WriteError("Can't move from CD-ROM");
	if (!do_quiet)
	    printf("Can't move from CD-ROM\n");
	die(MBERR_COMMANDLINE);
    }
    if (CheckFDB(From, area.Path))
	die(MBERR_GENERAL);

    /*
     * Find the file in the "from" area, check LFN and 8.3 names.
     */
    temp1 = calloc(PATH_MAX, sizeof(char));
    sprintf(temp1, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), From);
    if ((fp1 = fopen(temp1, "r")) == NULL)
	die(MBERR_GENERAL);
    while (fread(&fdb, sizeof(fdb), 1, fp1) == 1) {
	if ((strcmp(fdb.LName, File) == 0) || strcmp(fdb.Name, File) == 0) {
	    Found = TRUE;
	    break;
	}
    }
    fclose(fp1);
    if (!Found) {
	WriteError("File %s not found in area %d", File, From);
	if (!do_quiet)
	    printf("File %s not found in area %d\n", File, From);
	free(temp1);
	die(MBERR_GENERAL);
    }

    frompath = xstrcpy(area.Path);
    frompath = xstrcat(frompath, (char *)"/");
    frompath = xstrcat(frompath, fdb.Name);
    fromlink = xstrcpy(area.Path);
    fromlink = xstrcat(fromlink, (char *)"/");
    fromlink = xstrcat(fromlink, fdb.LName);
    fromthumb = xstrcpy(area.Path);
    fromthumb = xstrcat(fromthumb, (char *)"/.");
    fromthumb = xstrcat(fromthumb, fdb.Name);

    /*
     * Check Destination area
     */
    if (LoadAreaRec(To) == FALSE) {
	WriteError("Can't load record %d", To);
	die(MBERR_GENERAL);
    }
    if (!area.Available) {
	WriteError("Area %d not available", To);
	if (!do_quiet)
	    printf("Area %d not available\n", To);
	die(MBERR_GENERAL);
    }
    if (area.CDrom) {
	WriteError("Can't move to CD-ROM");
	if (!do_quiet)
	    printf("Can't move to CD-ROM\n");
	die(MBERR_COMMANDLINE);
    }
    if (CheckFDB(To, area.Path))
	die(MBERR_GENERAL);

    topath = xstrcpy(area.Path);
    topath = xstrcat(topath, (char *)"/");
    topath = xstrcat(topath, fdb.Name);
    tolink = xstrcpy(area.Path);
    tolink = xstrcat(tolink, (char *)"/");
    tolink = xstrcat(tolink, fdb.LName);
    tothumb = xstrcpy(area.Path);
    tothumb = xstrcat(tothumb, (char *)"/.");
    tothumb = xstrcat(tothumb, fdb.Name);

    if (file_exist(topath, F_OK) == 0) {
	WriteError("File %s already exists in area %d", File, To);
	if (!do_quiet)
	    printf("File %s already exists in area %d\n", File, To);
	die(MBERR_COMMANDLINE);
    }
	
    temp2 = calloc(PATH_MAX, sizeof(char));
    sprintf(temp2, "%s/fdb/fdb%d.temp", getenv("MBSE_ROOT"), From);

    if ((fp1 = fopen(temp1, "r")) == NULL)
	die(MBERR_GENERAL);
    if ((fp2 = fopen(temp2, "a+")) == NULL)
	die(MBERR_GENERAL);

    /*
     * Search the file if the From area, if found, the
     * temp database holds all records except the moved
     * file.
     */
    while (fread(&fdb, sizeof(fdb), 1, fp1) == 1) {
	if (strcmp(fdb.LName, File) && strcmp(fdb.Name, File))
	    fwrite(&fdb, sizeof(fdb), 1, fp2);
	else {
	    rc = AddFile(fdb, To, topath, frompath, tolink);
	    if (rc) {
		unlink(fromlink);
		unlink(frompath);

		/*
		 * Try to move thumbnail if it exists
		 */
		if (file_exist(fromthumb, R_OK) == 0) {
		    file_mv(fromthumb, tothumb);
		}
	    }
	}
    }
    fclose(fp1);
    fclose(fp2);
    
    if (rc) {
	/*
	 * The move was successfull
	 */
	if (unlink(temp1) == 0) {
	    rename(temp2, temp1);
	    chmod(temp1, 0660);
	} else {
	    WriteError("$Can't unlink %s", temp1);
	    unlink(temp2);
	}
	colour(CYAN, BLACK);
    } else {
	/*
	 * The move failed, it is possible that the file is
	 * copied already. Don't remove it here, it might
	 * be removed if it was not meant to be, ie if you
	 * gave this command twice. Let "mbfile check" take
	 * care of unwanted copies.
	 */
	unlink(temp2);
    }

    Syslog('+', "Move %s from %d to %d %s", File, From, To, rc ? "successfull":"failed");
    if (!do_quiet)
	printf("Move %s from %d to %d %s\n", File, From, To, rc ? "successfull":"failed");

    free(temp1);
    free(temp2);
    free(frompath);
    free(fromlink);
    free(fromthumb);
    free(topath);
    free(tolink);
    free(tothumb);
}


