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


void ListFileAreas(int Area)
{
    FILE    *pAreas, *pFile;
    int     i, iAreas, fcount, tcount = 0;
    int     iTotal = 0;
    long    fsize, tsize = 0;
    char    *sAreas, *fAreas, flags[6];

    /*
     * If nothing to display allowed, return at once.
     */
    if (do_quiet)
	return;

    colour(LIGHTRED, BLACK);
    sAreas = calloc(PATH_MAX, sizeof(char));
    fAreas = calloc(PATH_MAX, sizeof(char));

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	printf("Can't open %s\n", sAreas);
	die(0);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;
    
    if (Area) {
	IsDoing("List area %d", Area);

	if (fseek(pAreas, ((Area - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET)) {
	    WriteError("$Can't seek area %d", Area);
	    printf("Can't seek area %d\n", Area);
	    return;
	}
	if (fread(&area, areahdr.recsize, 1, pAreas) != 1) {
	    WriteError("$Can't read record for area %d", Area);
	    printf("Can't read record for area %d\n", Area);
	    return;
	}

	if (area.Available) {

	    /*
	     * Open the file database, create new one if it doesn't exist.
	     */
	    sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);
	    if ((pFile = fopen(fAreas, "r+")) == NULL) {
		Syslog('!', "Creating new %s", fAreas);
		if ((pFile = fopen(fAreas, "a+")) == NULL) {
		    WriteError("$Can't create %s", fAreas);
		    die(0);
		}
	    }

            fcount = 0;
	    fsize  = 0L;
	    colour(CYAN, BLACK);
	    printf("File listing of area %d, %s\n\n", Area, area.Name);
	    printf("File name    Kbytes File date  Dnlds Flags Description\n");
	    printf("------------ ------ ---------- ----- ----- ------------------------------------\n");
	    colour(LIGHTGRAY, BLACK);

	    while (fread(&file, sizeof(file), 1, pFile) == 1) {
		sprintf(flags, "-----");
		if (file.Free)
		    flags[0] = 'F';
		if (file.Deleted)
		    flags[1] = 'D';
		if (file.Missing)
		    flags[2] = 'M';
		if (file.NoKill)
		    flags[3] = 'N';
		if (file.Announced)
		    flags[4] = 'A';

		if (strlen(file.Desc[0]) > 36)
		    file.Desc[0][36] = '\0';
		printf("%-12s %6ld %s %5ld %s %s\n", 
			file.Name, (long)(file.Size / 1024), StrDateDMY(file.FileDate), 
			(long)(file.TimesDL + file.TimesFTP + file.TimesReq), flags, file.Desc[0]);
		fcount++;
		fsize = fsize + file.Size;
	    }
	    fsize = fsize / 1024;

	    colour(CYAN, BLACK);
	    printf("-------------------------------------------------------------------------------\n");
	    printf("%d file%s, %ld Kbytes\n", fcount, (fcount == 1) ? "":"s", fsize);

	} else {
	    WriteError("Area %d is not available", Area);
	    printf("Area %d is not available\n", Area);
	    return;
	}

	free(sAreas);
	free(fAreas);
	return;
    }

    IsDoing("List fileareas");
    colour(CYAN, BLACK);
    printf(" Area Files MByte File Group   Area name\n");
    printf("----- ----- ----- ------------ --------------------------------------------\n");
    colour(LIGHTGRAY, BLACK);

    for (i = 1; i <= iAreas; i++) {
	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    /*
	     * Open the file database, create new one if it doesn't exist.
	     */
	    sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);
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

	    printf("%5d %5d %5ld %-12s %s\n", i, fcount, fsize, area.BbsGroup, area.Name);
	    iTotal++;
	}
    }

    colour(CYAN, BLACK);
    printf("----- ----- ----- ---------------------------------------------------------\n");
    printf("%5d %5d %5ld \n", iTotal, tcount, tsize);
    fclose(pAreas);
    free(sAreas);
    free(fAreas);
}

