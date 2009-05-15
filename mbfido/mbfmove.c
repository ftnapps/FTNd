/*****************************************************************************
 *
 * $Id: mbfmove.c,v 1.19 2005/08/11 21:05:15 mbse Exp $
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
    char		    *frompath, *topath, *temp1, *fromlink, *tolink, *fromthumb, *tothumb;
    struct FILE_record	    f_db;
    int			    rc = FALSE, Found = FALSE;
    struct _fdbarea	    *src_area = NULL;

    IsDoing("Move file");
    mbse_colour(LIGHTRED, BLACK);

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
    if (CheckFDB(From, area.Path))
	die(MBERR_GENERAL);

    /*
     * Find the file in the "from" area, check LFN and 8.3 names.
     */
    if ((src_area = mbsedb_OpenFDB(From, 30)) == NULL)
	die(MBERR_GENERAL);

    while (fread(&f_db, fdbhdr.recsize, 1, src_area->fp) == 1) {
	if ((strcmp(f_db.LName, File) == 0) || strcmp(f_db.Name, File) == 0) {
	    Found = TRUE;
	    break;
	}
    }
    temp1 = calloc(PATH_MAX, sizeof(char)); // not serious
    if (!Found) {
	WriteError("File %s not found in area %d", File, From);
	if (!do_quiet)
	    printf("File %s not found in area %d\n", File, From);
	free(temp1);
	die(MBERR_GENERAL);
    }

    frompath = xstrcpy(area.Path);
    frompath = xstrcat(frompath, (char *)"/");
    frompath = xstrcat(frompath, f_db.Name);
    fromlink = xstrcpy(area.Path);
    fromlink = xstrcat(fromlink, (char *)"/");
    fromlink = xstrcat(fromlink, f_db.LName);
    fromthumb = xstrcpy(area.Path);
    fromthumb = xstrcat(fromthumb, (char *)"/.");
    fromthumb = xstrcat(fromthumb, f_db.Name);

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
    if (CheckFDB(To, area.Path))
	die(MBERR_GENERAL);

    topath = xstrcpy(area.Path);
    topath = xstrcat(topath, (char *)"/");
    topath = xstrcat(topath, f_db.Name);
    tolink = xstrcpy(area.Path);
    tolink = xstrcat(tolink, (char *)"/");
    tolink = xstrcat(tolink, f_db.LName);
    tothumb = xstrcpy(area.Path);
    tothumb = xstrcat(tothumb, (char *)"/.");
    tothumb = xstrcat(tothumb, f_db.Name);

    if (file_exist(topath, F_OK) == 0) {
	WriteError("File %s already exists in area %d", File, To);
	if (!do_quiet)
	    printf("File %s already exists in area %d\n", File, To);
	die(MBERR_COMMANDLINE);
    }

    rc = AddFile(f_db, To, topath, frompath, tolink);
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
    if (mbsedb_LockFDB(src_area, 30)) {
	f_db.Deleted = TRUE;
	fseek(src_area->fp, - fdbhdr.recsize, SEEK_CUR);
	fwrite(&f_db, fdbhdr.recsize, 1, src_area->fp);
	mbsedb_UnlockFDB(src_area);
    }
    mbsedb_PackFDB(src_area);
    mbsedb_CloseFDB(src_area);
    mbse_colour(CYAN, BLACK);

    Syslog('+', "Move %s from %d to %d %s", File, From, To, rc ? "successfull":"failed");
    if (!do_quiet)
	printf("Move %s from %d to %d %s\n", File, From, To, rc ? "successfull":"failed");

    free(temp1);
    free(frompath);
    free(fromlink);
    free(fromthumb);
    free(topath);
    free(tolink);
    free(tothumb);
}


