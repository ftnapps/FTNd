/*****************************************************************************
 *
 * $Id: mbfdel.c,v 1.17 2005/08/11 21:05:15 mbse Exp $
 * Purpose: File Database Maintenance - Delete/Undelete a file
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
    mbse_colour(LIGHTRED, BLACK);

    /*
     * Check area
     */
    if (LoadAreaRec(Area) == FALSE) {
	WriteError("Can't load record %d", Area);
	die(MBERR_INIT_ERROR);
    }
    if (!area.Available) {
	WriteError("Area %d not available", Area);
	if (!do_quiet)
	    printf("Area %d not available\n", Area);
	die(MBERR_CONFIG_ERROR);
    }

    if ((fdb_area = mbsedb_OpenFDB(Area, 30)) == NULL)
	die(MBERR_GENERAL);


    mbse_colour(CYAN, BLACK);
    strcpy(mask, re_mask(File, FALSE));
    if (re_comp(mask))
	die(MBERR_GENERAL);

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
		if (mbsedb_LockFDB(fdb_area, 30)) {
		    fseek(fdb_area->fp, - fdbhdr.recsize, SEEK_CUR);
		    fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
		    mbsedb_UnlockFDB(fdb_area);
		} else {
		    rc = FALSE;
		}
	    }
	}
    }
    mbsedb_CloseFDB(fdb_area);

    if (!rc) {
	Syslog('+', "%selete %s in area %d failed", UnDel?"Und":"D", File, Area);
	if (!do_quiet)
	    printf("%selete %s in area %d failed\n", UnDel?"Und":"D", File, Area);
    }
}


