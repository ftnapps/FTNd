/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Sort filebase
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/mberrors.h"
#include "mbfutil.h"
#include "mbfsort.h"



extern int	do_quiet;		/* Suppress screen output	*/
extern int	do_index;		/* Reindex filebases		*/


typedef struct _fdbs {
    struct _fdbs	*next;
    struct FILERecord	filrec;
} fdbs;



void fill_fdbs(struct FILERecord, fdbs **);
void fill_fdbs(struct FILERecord filrec, fdbs **fap)
{
    fdbs    *tmp;

    tmp = (fdbs *)malloc(sizeof(fdbs));
    tmp->next = *fap;
    tmp->filrec = filrec;
    *fap = tmp;
}



void tidy_fdbs(fdbs **);
void tidy_fdbs(fdbs **fap)
{
    fdbs    *tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *fap = NULL;
}


int comp_fdbs(fdbs **, fdbs **);


void sort_fdbs(fdbs **);
void sort_fdbs(fdbs **fap)
{
    fdbs    *ta, **vector;
    size_t  n = 0, i;

    if (*fap == NULL)
	return;

    for (ta = *fap; ta; ta = ta->next)
	n++;

    vector = (fdbs **)malloc(n * sizeof(fdbs *));
    i = 0;
    for (ta = *fap; ta; ta = ta->next)
	vector[i++] = ta;

    qsort(vector, n, sizeof(fdbs *), (int(*)(const void*, const void *))comp_fdbs);
    (*fap) = vector[0];
    i = 1;

    for (ta = *fap; ta; ta = ta->next) {
	if (i < n)
	    ta->next = vector[i++];
	else
	    ta->next = NULL;
    }

    free(vector);
    return;
}



int comp_fdbs(fdbs **fap1, fdbs **fap2)
{
    return strcasecmp((*fap1)->filrec.LName, (*fap2)->filrec.LName);
}



/*
 *  Sort files database
 */
void SortFileBase(int Area)
{
    FILE    *fp, *pAreas, *pFile;
    int	    iAreas, iTotal = 0;
    char    *sAreas, *fAreas, *fTmp;
    fdbs    *fdx = NULL, *tmp;

    sAreas = calloc(PATH_MAX, sizeof(char));
    fAreas = calloc(PATH_MAX, sizeof(char));
    fTmp   = calloc(PATH_MAX, sizeof(char));

    IsDoing("Sort filebase");
    if (!do_quiet) {
	colour(3, 0);
    }

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	die(MBERR_INIT_ERROR);
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

	    if (!diskfree(CFG.freespace))
		die(MBERR_DISK_FULL);

	    if (!do_quiet) {
		printf("Sorting area %d: %-44s", Area, area.Name);
		fflush(stdout);
	    }

	    sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);
	    sprintf(fTmp,   "%s/fdb/fdb%d.temp", getenv("MBSE_ROOT"), Area);

	    if ((pFile = fopen(fAreas, "r")) == NULL) {
		Syslog('!', "Creating new %s", fAreas);
		if ((pFile = fopen(fAreas, "a+")) == NULL) {
		    WriteError("$Can't create %s", fAreas);
		    die(MBERR_GENERAL);
		}
	    } 

	    if ((fp = fopen(fTmp, "a+")) == NULL) {
		WriteError("$Can't create %s", fTmp);
		die(MBERR_GENERAL);
	    }

	    /*
	     * Fill the sort array
	     */
	    while (fread(&file, sizeof(file), 1, pFile) == 1) {
		iTotal++;
		fill_fdbs(file, &fdx);
		Syslog('f', "Adding %s", file.LName);
	    }

	    sort_fdbs(&fdx);

	    /*
	     * Write sorted files to temp database
	     */
	    for (tmp = fdx; tmp; tmp = tmp->next) {
		Syslog('f', "Sorted %s", tmp->filrec.LName);
		fwrite(&tmp->filrec, sizeof(file), 1, fp);
	    }
	    tidy_fdbs(&fdx);

	    fclose(fp);
	    fclose(pFile);

	    if ((rename(fTmp, fAreas)) == 0) {
		unlink(fTmp);
		chmod(fAreas, 00660);
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

    free(fTmp);
    free(sAreas);
    free(fAreas);
}



