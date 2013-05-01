/*****************************************************************************
 *
 * ftnfdel.c
 * Purpose: File Database Maintenance - Delete/Undelete a file
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/users.h"
#include "../lib/ftnddb.h"
#include "ftnfutil.h"
#include "ftnfmove.h"



extern int	do_quiet;		/* Suppress screen output	    */



/*
 * Delete a file
 */
void Delete(int UnDel, int Area, char *File)
{
    char		mask[256];
    int			rc = FALSE;
    struct _fdbarea	*fdb_area = NULL;

    if (UnDel)
	IsDoing("Undelete file");
    else
	IsDoing("Delete file");
    ftnd_colour(LIGHTRED, BLACK);

    /*
     * Check area
     */
    if (LoadAreaRec(Area) == FALSE) {
	WriteError("Can't load record %d", Area);
	die(FTNERR_INIT_ERROR);
    }
    if (!area.Available) {
	WriteError("Area %d not available", Area);
	if (!do_quiet)
	    printf("Area %d not available\n", Area);
	die(FTNERR_CONFIG_ERROR);
    }

    if ((fdb_area = ftnddb_OpenFDB(Area, 30)) == NULL)
	die(FTNERR_GENERAL);


    ftnd_colour(CYAN, BLACK);
    strcpy(mask, re_mask(File, FALSE));
    if (re_comp(mask))
	die(FTNERR_GENERAL);

    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
	if (re_exec(fdb.LName) || re_exec(fdb.Name)) {
	    if (UnDel && fdb.Deleted) {
		fdb.Deleted = FALSE;
		Syslog('+', "Marked file %s in area %d for undeletion", fdb.Name, Area);
		if (!do_quiet)
		    printf("Marked file %s in area %d for undeletion\n", fdb.Name, Area);
		rc = TRUE;
	    }
	    if (!UnDel && !fdb.Deleted) {
		fdb.Deleted = TRUE;
		Syslog('+', "Marked file %s in area %d for deletion", fdb.Name, Area);
		if (!do_quiet)
		    printf("Marked file %s in area %d for deletion\n", fdb.Name, Area);
		rc = TRUE;
	    }
	    if (rc) {
		if (ftnddb_LockFDB(fdb_area, 30)) {
		    fseek(fdb_area->fp, - fdbhdr.recsize, SEEK_CUR);
		    fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
		    ftnddb_UnlockFDB(fdb_area);
		} else {
		    rc = FALSE;
		}
	    }
	}
    }
    ftnddb_CloseFDB(fdb_area);

    if (!rc) {
	Syslog('+', "%selete %s in area %d failed", UnDel?"Und":"D", File, Area);
	if (!do_quiet)
	    printf("%selete %s in area %d failed\n", UnDel?"Und":"D", File, Area);
    }
}


