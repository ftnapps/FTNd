/*****************************************************************************
 *
 * $Id: mbflist.c,v 1.21 2005/10/11 20:49:47 mbse Exp $
 * Purpose: File Database Maintenance - List areas and totals
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
#include "mbfutil.h"
#include "mbflist.h"



extern int	do_quiet;		/* Suppress screen output	    */


void ListFileAreas(int Area)
{
    FILE    *pAreas, *pTic;
    int     i, iAreas, fcount, tcount = 0, iTotal = 0, columns = 80;
    int	    fsize, tsize = 0;
    char    *sAreas, *fAreas, *sTic, flags[6], *ticarea;
    struct _fdbarea *fdb_area = NULL;

    /*
     * If nothing to display allowed, return at once.
     */
    if (do_quiet)
	return;

    /*
     * See if we know the width of the users screen.
     */
    if (getenv("COLUMNS")) {
	i = atoi(getenv("COLUMNS"));
	if (i >= 80)
	    columns = i;
    }

    mbse_colour(LIGHTRED, BLACK);
    sAreas  = calloc(PATH_MAX, sizeof(char));
    fAreas  = calloc(PATH_MAX, sizeof(char));
    sTic    = calloc(PATH_MAX, sizeof(char));
    ticarea = calloc(21, sizeof(char));

    snprintf(sAreas, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	printf("Can't open %s\n", sAreas);
	die(MBERR_INIT_ERROR);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    if (Area) {
	IsDoing("List area %d", Area);

	snprintf(sTic, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((pTic = fopen(sTic, "r")) == NULL) {
	    WriteError("Can't open %s", sTic);
	    printf("Can't open %s\n", sTic);
	    die(MBERR_GENERAL);
	}
	fread(&tichdr, sizeof(tichdr), 1, pTic);
		
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
	     * Open the file database.
	     */
	    fdb_area = mbsedb_OpenFDB(Area, 30);
            fcount = 0;
	    fsize  = 0L;
	    mbse_colour(CYAN, BLACK);
	    printf("File listing of area %d, %s\n\n", Area, area.Name);
	    printf("Short name     Kb. File date  Down Flg TIC Area             Long name\n");
	    printf("------------ ----- ---------- ---- --- -------------------- ");
	    for (i = 60; i < columns; i++)
		printf("-");
	    printf("\n");

	    mbse_colour(LIGHTGRAY, BLACK);

	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		snprintf(flags, 4, "---");
		if (fdb.Deleted)
		    flags[0] = 'D';
		if (fdb.NoKill)
		    flags[1] = 'N';
		if (fdb.Announced)
		    flags[2] = 'A';

		fdb.LName[columns - 60] = '\0';
		printf("%-12s %5d %s %4d %s %-20s %s\n", fdb.Name, (int)(fdb.Size / 1024), StrDateDMY(fdb.FileDate), 
			(int)(fdb.TimesDL), flags, fdb.TicArea, fdb.LName);
		fcount++;
		fsize = fsize + fdb.Size;
	    }
	    fsize = fsize / 1024;

	    mbse_colour(CYAN, BLACK);
	    printf("------------------------------------------------------------");
	    for (i = 60; i < columns; i++)
		printf("-");
	    printf("\n");
	    printf("%d file%s, %d Kbytes\n", fcount, (fcount == 1) ? "":"s", fsize);
	    mbsedb_CloseFDB(fdb_area);

	} else {
	    WriteError("Area %d is not available", Area);
	    printf("Area %d is not available\n", Area);
	    return;
	}

	fclose(pAreas);
	fclose(pTic);
	free(ticarea);
	free(sAreas);
	free(fAreas);
	free(sTic);
	return;
    }

    IsDoing("List fileareas");
    mbse_colour(CYAN, BLACK);
    printf(" Area Files MByte File Group   Area name\n");
    printf("----- ----- ----- ------------ --------------------------------------------\n");
    mbse_colour(LIGHTGRAY, BLACK);

    for (i = 1; i <= iAreas; i++) {
	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    fdb_area = mbsedb_OpenFDB(i, 30);
	    fcount = 0;
	    fsize  = 0L;
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		fcount++;
		fsize = fsize + fdb.Size;
	    }
	    fsize = fsize / 1048576;
	    tcount += fcount;
	    tsize  += fsize;

	    printf("%5d %5d %5d %-12s %s\n", i, fcount, fsize, area.BbsGroup, area.Name);
	    iTotal++;
	    mbsedb_CloseFDB(fdb_area);
	}
    }

    mbse_colour(CYAN, BLACK);
    printf("----- ----- ----- ---------------------------------------------------------\n");
    printf("%5d %5d %5d \n", iTotal, tcount, tsize);
    fclose(pAreas);
    free(ticarea);
    free(sAreas);
    free(fAreas);
    free(sTic);
}

