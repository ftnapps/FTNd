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
    char		    *frompath, *topath, *temp1, *fromlink, *tolink, *fromthumb, *tothumb;
    struct FILE_record	    f_db;
    int			    rc = FALSE, Found = FALSE;
#ifdef	USE_EXPERIMENT
    struct _fdbarea	    *src_area = NULL;
#else
    struct FILE_recordhdr   f_dbhdr;
    FILE                    *fp1, *fp2;
    char		    *temp2;
#endif

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
#ifdef	USE_EXPERIMENT
    if ((src_area = mbsedb_OpenFDB(From, 30)) == NULL)
	die(MBERR_GENERAL);

    while (fread(&f_db, fdbhdr.recsize, 1, src_area->fp) == 1) {
	if ((strcmp(f_db.LName, File) == 0) || strcmp(f_db.Name, File) == 0) {
	    Found = TRUE;
	    break;
	}
    }
    temp1 = calloc(PATH_MAX, sizeof(char)); // not serious
#else
    temp1 = calloc(PATH_MAX, sizeof(char));
    sprintf(temp1, "%s/fdb/file%d.data", getenv("MBSE_ROOT"), From);
    if ((fp1 = fopen(temp1, "r")) == NULL)
	die(MBERR_GENERAL);
    fread(&f_dbhdr, sizeof(fdbhdr), 1, fp1);

    while (fread(&f_db, f_dbhdr.recsize, 1, fp1) == 1) {
	if ((strcmp(f_db.LName, File) == 0) || strcmp(f_db.Name, File) == 0) {
	    Found = TRUE;
	    break;
	}
    }
    fclose(fp1);
#endif
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

#ifdef	USE_EXPERIMENT
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
    colour(CYAN, BLACK);
#else
    temp2 = calloc(PATH_MAX, sizeof(char));
    sprintf(temp2, "%s/fdb/file%d.temp", getenv("MBSE_ROOT"), From);

    if ((fp1 = fopen(temp1, "r")) == NULL)
	die(MBERR_GENERAL);
    fread(&f_dbhdr, sizeof(fdbhdr), 1, fp1);
    if ((fp2 = fopen(temp2, "a+")) == NULL)
	die(MBERR_GENERAL);
    fwrite(&f_dbhdr, f_dbhdr.hdrsize, 1, fp2);

    /*
     * Search the file if the From area, if found, the
     * temp database holds all records except the moved
     * file.
     */
    while (fread(&f_db, f_dbhdr.recsize, 1, fp1) == 1) {
	if (strcmp(f_db.LName, File) && strcmp(f_db.Name, File))
	    fwrite(&f_db, f_dbhdr.recsize, 1, fp2);
	else {
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
    free(temp2);
#endif

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


