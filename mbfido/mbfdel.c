/*****************************************************************************
 *
 * $Id$
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
 * Move a file
 */
void Delete(int UnDel, int Area, char *File)
{
    char		*temp;
    struct FILERecord	fdb;
    FILE		*fp;
    int			rc = FALSE;

    if (UnDel)
	IsDoing("Undelete file");
    else
	IsDoing("Delete file");
    colour(LIGHTRED, BLACK);

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
    if (area.CDrom) {
	WriteError("Can't %sdelete from CD-ROM", UnDel?"un":"");
	if (!do_quiet)
	    printf("Can't %sdelete from CD-ROM\n", UnDel?"un":"");
	die(MBERR_COMMANDLINE);
    }
    if (CheckFDB(Area, area.Path))
	die(MBERR_GENERAL);

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);

    if ((fp = fopen(temp, "r+")) == NULL)
	die(MBERR_GENERAL);

    colour(CYAN, BLACK);

    while (fread(&fdb, sizeof(fdb), 1, fp) == 1) {
	if ((! strcmp(fdb.LName, File) || (! strcmp(fdb.Name, File)))) {
	    if (UnDel && fdb.Deleted) {
		fdb.Deleted = FALSE;
		Syslog('+', "Marked file %s in area %d for undeletion", File, Area);
		if (!do_quiet)
		    printf("Marked file %s in area %d for undeletion\n", File, Area);
		rc = TRUE;
	    }
	    if (!UnDel && !fdb.Deleted) {
		fdb.Deleted = TRUE;
		Syslog('+', "Marked file %s in area %d for deletion", File, Area);
		if (!do_quiet)
		    printf("Marked file %s in area %d for deletion\n", File, Area);
		rc = TRUE;
	    }
	    if (rc) {
		fseek(fp, - sizeof(fdb), SEEK_CUR);
		fwrite(&fdb, sizeof(fdb), 1, fp);
	    }
	    break;
	}
    }
    fclose(fp);

    if (!rc) {
	Syslog('+', "%selete %s in area %d failed", UnDel?"Und":"D", File, Area);
	if (!do_quiet)
	    printf("%selete %s in area %d failed\n", UnDel?"Und":"D", File, Area);
    }

    free(temp);
}


