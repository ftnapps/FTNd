/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - List areas and totals
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "mbfutil.h"
#include "mbflist.h"



extern int	do_quiet;		/* Supress screen output	    */


void ListFileAreas(void)
{
    FILE    *pAreas, *pFile;
    int     i, iAreas, fcount, tcount = 0;
    int     iTotal = 0;
    long    fsize, tsize = 0;
    char    *sAreas, *fAreas;

    sAreas = calloc(PATH_MAX, sizeof(char));
    fAreas = calloc(PATH_MAX, sizeof(char));

    IsDoing("List fileareas");
    if (!do_quiet) {
	colour(3, 0);
	printf(" Area Files MByte File Group   Area name\n");
	printf("----- ----- ----- ------------ --------------------------------------------\n");
	colour(7, 0);
    }

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	if (!do_quiet)
	    printf("Can't open %s\n", sAreas);
	die(0);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    for (i = 1; i <= iAreas; i++) {
	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);

	    /*
	     * Open the file database, create new one if it doesn't excist.
	     */
	    if ((pFile = fopen(fAreas, "r+")) == NULL) {
		Syslog('!', "Creating new %s", fAreas);
		if ((pFile = fopen(fAreas, "a+")) == NULL) {
		    WriteError("$Can't create %s", fAreas);
		    die(0);
		}
	    }

	    fcount = 0;
	    fsize  = 0L;
	    while (fread(&file, sizeof(file), 1, pFile) == 1) {
		fcount++;
		fsize = fsize + file.Size;
	    }
	    fsize = fsize / 1048576;
	    tcount += fcount;
	    tsize  += fsize;

	    if (!do_quiet)
		printf("%5d %5d %5ld %-12s %s\n", i, fcount, fsize, area.BbsGroup, area.Name);
	    iTotal++;
	}
    }

    if (!do_quiet) {
	colour(3, 0);
	printf("----- ----- ----- ---------------------------------------------------------\n");
	printf("%5d %5d %5ld \n", iTotal, tcount, tsize);
    }

    fclose(pAreas);
    if (!do_quiet) {
	printf("\r                                                              \r");
	fflush(stdout);
    }

    free(sAreas);
    free(fAreas);
}

