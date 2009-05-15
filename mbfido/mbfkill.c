/*****************************************************************************
 *
 * $Id: mbfkill.c,v 1.27 2005/08/28 14:10:06 mbse Exp $
 * Purpose: File Database Maintenance, kill or move old files
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
#include "mbfkill.h"
#include "mbfutil.h"



extern int	do_quiet;		/* Suppress screen output	    */
extern int	do_pack;		/* Perform pack			    */



/*
 *  Check files for age, and not downloaded for x days. If they match
 *  one of these criteria (setable in areas setup), the file will be
 *  move to some retire area or deleted, depending on the setup.
 *  If they are moved, the upload date is reset to the current date,
 *  so you can set new removal criteria again.
 */
void Kill(void)
{
    FILE		*pAreas;
    int			i, iAreas, iAreasNew = 0, iTotal = 0, iKilled =  0, iMoved = 0, rc, Killit;
    char		*sAreas, *newdir = NULL, *sTemp, from[PATH_MAX], to[PATH_MAX];
    time_t		Now;
    struct fileareas	darea;
    struct _fdbarea	*fdb_area = NULL, *dst_area = NULL;
    
    sAreas = calloc(PATH_MAX, sizeof(char));
    sTemp  = calloc(PATH_MAX, sizeof(char));

    IsDoing("Kill files");
    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("Kill/move files...\n");
    }

    snprintf(sAreas, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	die(MBERR_INIT_ERROR);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;
    Now = time(NULL);

    for (i = 1; i <= iAreas; i++) {

	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if ((area.Available) && (area.DLdays || area.FDdays)) {

	    if (enoughspace(CFG.freespace) == 0)
		die(MBERR_DISK_FULL);

	    if (!do_quiet) {
		printf("\r%4d => %-44s    \b\b\b\b", i, area.Name);
		fflush(stdout);
	    }

	    /*
	     * Check if download directory exists, if not, create the directory.
	     */
	    if (access(area.Path, R_OK) == -1) {
		Syslog('!', "Create dir: %s", area.Path);
		newdir = xstrcpy(area.Path);
		newdir = xstrcat(newdir, (char *)"/");
		mkdirs(newdir, 0755);
		free(newdir);
		newdir = NULL;
	    }

	    if ((fdb_area = mbsedb_OpenFDB(i, 30)) == NULL)
		die(MBERR_GENERAL);

	    /*
	     * Now start checking the files in the filedatabase
	     * against the contents of the directory.
	     */
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		iTotal++;
		Marker();

		Killit = FALSE;
		if (!fdb.UploadDate)
		    Syslog('!', "Warning: file %s in area %d has no upload date", fdb.Name, i);

		if (area.DLdays) {
		    /*
		     * Test last download date or never downloaded and the
		     * file is more then n days available for download.
		     */
		    if ((fdb.LastDL) && (((Now - fdb.LastDL) / 84400) > area.DLdays)) {
			Killit = TRUE;
		    }
		    if ((!fdb.LastDL) && fdb.UploadDate && (((Now - fdb.UploadDate) / 84400) > area.DLdays)) {
			Killit = TRUE;
		    }
		}

		if (area.FDdays) {
		    /*
		     * Check filedate
		     */
		    if (fdb.UploadDate && (((Now - fdb.UploadDate) / 84400) > area.FDdays)) {
			Killit = TRUE;
		    }
		}

		if (Killit) {
		    do_pack = TRUE;
		    if (area.MoveArea) {
			fseek(pAreas, ((area.MoveArea -1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
			fread(&darea, areahdr.recsize, 1, pAreas);
			snprintf(from, PATH_MAX, "%s/%s", area.Path, fdb.Name);
			snprintf(to,   PATH_MAX, "%s/%s", darea.Path, fdb.Name);
			if ((rc = file_mv(from, to)) == 0) {
			    Syslog('+', "Move %s, area %d => %d", fdb.Name, i, area.MoveArea);
			    if ((dst_area = mbsedb_OpenFDB(area.MoveArea, 30))) {
				fdb.UploadDate = time(NULL);
				fdb.LastDL = time(NULL);
				mbsedb_InsertFDB(dst_area, fdb, FALSE);
				mbsedb_CloseFDB(dst_area);
			    }

			    /*
			     * Now again if there is a dotted version (thumbnail) of this file.
			     */
			    snprintf(from, PATH_MAX, "%s/.%s", area.Path, fdb.Name);
			    snprintf(to,   PATH_MAX, "%s/.%s", darea.Path, fdb.Name);
			    if (file_exist(from, R_OK) == 0)
				file_mv(from, to);

			    /*
			     * Unlink the old symbolic link
			     */
			    snprintf(from, PATH_MAX, "%s/%s", area.Path, fdb.LName);
			    unlink(from);

			    /*
			     * Create the new symbolic link
			     */
			    snprintf(from, PATH_MAX, "%s/%s", darea.Path, fdb.Name);
			    snprintf(to,   PATH_MAX, "%s/%s", darea.Path, fdb.LName);
			    symlink(from, to);

			    fdb.Deleted = TRUE;
			    if (mbsedb_LockFDB(fdb_area, 30)) {
				fseek(fdb_area->fp, - fdbhdr.recsize, SEEK_CUR);
				fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
				mbsedb_UnlockFDB(fdb_area);
			    }
			    iMoved++;
			} else {
			    WriteError("Move %s to area %d failed, %s", fdb.Name, area.MoveArea, strerror(rc));
			}
		    } else {
			Syslog('+', "Delete %s, area %d", fdb.LName, i);
			fdb.Deleted = TRUE;
			if (mbsedb_LockFDB(fdb_area, 30)) {
			    fseek(fdb_area->fp, - fdbhdr.recsize, SEEK_CUR);
			    fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
			    mbsedb_UnlockFDB(fdb_area);
			}
			iKilled++;
			snprintf(from, PATH_MAX, "%s/%s", area.Path, fdb.LName);
			unlink(from);
			snprintf(from, PATH_MAX, "%s/%s", area.Path, fdb.Name);
			unlink(from);
			snprintf(from, PATH_MAX, "%s/.%s", area.Path, fdb.Name);
			unlink(from);
		    }
		}
	    }

	    /*
	     * Now we must pack this area database otherwise
	     * we run into trouble later on.
	     */
	    mbsedb_PackFDB(fdb_area);
	    mbsedb_CloseFDB(fdb_area);
	    iAreasNew++;

	} /* if area.Available */
    }

    fclose(pAreas);

    Syslog('+', "Kill  Areas [%6d] Files [%6d] Deleted [%6d] Moved [%6d]", iAreasNew, iTotal, iKilled, iMoved);

    if (!do_quiet) {
	printf("\r                                                          \r");
	fflush(stdout);
    }

    free(sTemp);
    free(sAreas);
}



