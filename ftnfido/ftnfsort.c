/*****************************************************************************
 *
 * ftnfsort.c
 * Purpose: File Database Maintenance - Sort filebase
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
#include "ftnfsort.h"



extern int	do_quiet;		/* Suppress screen output	*/
extern int	do_index;		/* Reindex filebases		*/



/*
 *  Sort files database
 */
void SortFileBase(int Area)
{
    FILE    *pAreas;
    int	    iAreas;
    char    *sAreas;
    struct _fdbarea *fdb_area = NULL;
    
    sAreas = calloc(PATH_MAX, sizeof(char));

    IsDoing("Sort filebase");
    if (!do_quiet) {
	ftnd_colour(CYAN, BLACK);
    }

    snprintf(sAreas, PATH_MAX, "%s/etc/fareas.data", getenv("FTND_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	die(FTNERR_INIT_ERROR);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    if ((Area < 1) || (Area > iAreas)) {
	if (!do_quiet) {
	    printf("Area must be between 1 and %d\n", iAreas);
	}
    } else {
	fseek(pAreas, ((Area - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    if (enoughspace(CFG.freespace) == 0)
		die(FTNERR_DISK_FULL);

	    if (!do_quiet) {
		printf("Sorting area %d: %-44s", Area, area.Name);
		fflush(stdout);
	    }

	    if ((fdb_area = ftnddb_OpenFDB(Area, 30))) {
		ftnddb_SortFDB(fdb_area);
		ftnddb_CloseFDB(fdb_area);
	    }
	    Syslog('+', "Sorted file area %d: %s", Area, area.Name);
	    do_index = TRUE;

	} else {
	    printf("You cannot sort area %d\n", Area);
	}
    }

    fclose(pAreas);

    if (!do_quiet) {
	printf("\r                                                              \r");
	fflush(stdout);
    }

    free(sAreas);
}



