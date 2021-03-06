/*****************************************************************************
 *
 * ftnfpack.c
 * Purpose: File Database Maintenance - Pack filebase
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
#include "ftnfpack.h"



extern int	do_quiet;		/* Suppress screen output	*/
extern int	do_index;		/* Reindex filebases		*/


/*
 *  Removes records who are marked for deletion. If there is still a file
 *  on disk, it will be removed too.
 */
void PackFileBase(void)
{
    FILE    *pAreas;
    int	    i, iAreas, iAreasNew = 0, rc, iTotal = 0, iRemoved = 0;
    char    *sAreas, fn[PATH_MAX];
    struct _fdbarea *fdb_area = NULL;
    int	    purge;

    sAreas = calloc(PATH_MAX, sizeof(char));

    IsDoing("Pack filebase");
    if (!do_quiet) {
	ftnd_colour(CYAN, BLACK);
	printf("Packing file database...\n");
    }

    snprintf(sAreas, PATH_MAX, "%s/etc/fareas.data", getenv("FTND_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	die(FTNERR_INIT_ERROR);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    for (i = 1; i <= iAreas; i++) {

	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    if (enoughspace(CFG.freespace) == 0)
		die(FTNERR_DISK_FULL);

	    if (!do_quiet) {
		printf("\r%4d => %-44s", i, area.Name);
		fflush(stdout);
	    }
	    Marker();

	    if ((fdb_area = ftnddb_OpenFDB(i, 30)) == NULL)
		die(FTNERR_GENERAL);
	    purge = 0;

	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		iTotal++;

		if ((fdb.Deleted) || (fdb.Double) || (strcmp(fdb.Name, "") == 0)) {
		    iRemoved++;
		    purge++;
		    if (fdb.Double) {
			Syslog('+', "Removed double record file \"%s\" from area %d", fdb.LName, i);
		    } else {
			Syslog('+', "Removed file \"%s\" from area %d", fdb.LName, i);
			snprintf(fn, PATH_MAX, "%s/%s", area.Path, fdb.LName);
			rc = unlink(fn);
			if (rc && (errno != ENOENT))
			    WriteError("PackFileBase(): unlink %s failed, result %s", fn, strerror(rc));
			snprintf(fn, PATH_MAX, "%s/%s", area.Path, fdb.Name);
			rc = unlink(fn);
			if (rc && (errno != ENOENT))
			    WriteError("PackFileBase(): unlink %s failed, result %s", fn, strerror(rc));
			/*
			 * If a dotted version (thumbnail) exists, remove it silently
			 */
			snprintf(fn, PATH_MAX, "%s/.%s", area.Path, fdb.Name);
			unlink(fn);
		    }
		    do_index = TRUE;
		}
	    }

	    if (purge)
		ftnddb_PackFDB(fdb_area);
	    ftnddb_CloseFDB(fdb_area);
	    iAreasNew++;

	} /* if area.Available */
    }

    fclose(pAreas);
    Syslog('+', "Pack  Areas [%6d] Files [%6d] Removed [%6d]", iAreasNew, iTotal, iRemoved);

    if (!do_quiet) {
	printf("\r                                                              \r");
	fflush(stdout);
    }

    free(sAreas);
}


