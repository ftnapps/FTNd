/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Build index for request processor
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
#include "mbfindex.h"



extern int	do_quiet;		/* Supress screen output	    */


typedef struct _Index {
	struct _Index		*next;
	struct FILEIndex	idx;
} Findex;



void tidy_index(Findex **);
void tidy_index(Findex **fap)
{
	Findex	*tmp, *old;

	for (tmp = *fap; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fap = NULL;
}



void fill_index(struct FILEIndex, Findex **);
void fill_index(struct FILEIndex idx, Findex **fap)
{
	Findex	*tmp;

	tmp = (Findex *)malloc(sizeof(Findex));
	tmp->next = *fap;
	tmp->idx  = idx;
	*fap = tmp;
}


int comp_index(Findex **, Findex **);

void sort_index(Findex **);
void sort_index(Findex **fap)
{
	Findex	*ta, **vector;
	size_t	n = 0, i;

	if (*fap == NULL)
		return;

	for (ta = *fap; ta; ta = ta->next)
		n++;

	vector = (Findex **)malloc(n * sizeof(Findex *));

	i = 0;
	for (ta = *fap; ta; ta = ta->next)
		vector[i++] = ta;

	qsort(vector, n, sizeof(Findex *), 
		(int(*)(const void*, const void *))comp_index);

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



int comp_index(Findex **fap1, Findex **fap2)
{
	return strcasecmp((*fap1)->idx.LName, (*fap2)->idx.LName);
}



/*
 * Build a sorted index for the file request processor.
 */
void Index(void)
{
	FILE	*pAreas, *pFile, *pIndex;
	long	i, iAreas, iAreasNew = 0, record;
	int	iTotal = 0;
	char	*sAreas, *fAreas, *newdir = NULL, *sIndex;
	Findex	*fdx = NULL;
	Findex	*tmp;
	struct FILEIndex	idx;

	sAreas = calloc(PATH_MAX, sizeof(char));
	fAreas = calloc(PATH_MAX, sizeof(char));
	sIndex = calloc(PATH_MAX, sizeof(char));

	IsDoing("Kill files");
	if (!do_quiet) {
		colour(3, 0);
		printf("Create filerequest index...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("$Can't open %s", sAreas);
		die(0);
	}

	sprintf(sIndex, "%s/etc/request.index", getenv("MBSE_ROOT"));
	if ((pIndex = fopen(sIndex, "w")) == NULL) {
		WriteError("$Can't create %s", sIndex);
		die(0);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	fseek(pAreas, 0, SEEK_END);
	iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

	for (i = 1; i <= iAreas; i++) {

		fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
		fread(&area, areahdr.recsize, 1, pAreas);

		if ((area.Available) && (area.FileReq)) {

			if (!diskfree(CFG.freespace))
				die(101);

			if (!do_quiet) {
				printf("\r%4ld => %-44s    \b\b\b\b", i, area.Name);
				fflush(stdout);
			}

			/*
			 * Check if download directory exists, 
			 * if not, create the directory.
			 */
			if (access(area.Path, R_OK) == -1) {
				Syslog('!', "Create dir: %s", area.Path);
				newdir = xstrcpy(area.Path);
				newdir = xstrcat(newdir, (char *)"/");
				mkdirs(newdir);
				free(newdir);
				newdir = NULL;
			}

			sprintf(fAreas, "%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), i);

			/*
			 * Open the file database, if it doesn't exist,
			 * create an empty one.
			 */
			if ((pFile = fopen(fAreas, "r+")) == NULL) {
				Syslog('!', "Creating new %s", fAreas);
				if ((pFile = fopen(fAreas, "a+")) == NULL) {
					WriteError("$Can't create %s", fAreas);
					die(0);
				}
			} 

			/*
			 * Now start creating the unsorted index.
			 */
			record = 0;
			while (fread(&file, sizeof(file), 1, pFile) == 1) {
				iTotal++;
				if ((iTotal % 10) == 0)
					Marker();
				memset(&idx, 0, sizeof(idx));
				sprintf(idx.Name, "%s", tu(file.Name));
				sprintf(idx.LName, "%s", tu(file.LName));
				idx.AreaNum = i;
				idx.Record = record;
				fill_index(idx, &fdx);
				record++;
			}

			fclose(pFile);
			iAreasNew++;

		} /* if area.Available */
	}

	fclose(pAreas);

	sort_index(&fdx);
	for (tmp = fdx; tmp; tmp = tmp->next)
		fwrite(&tmp->idx, sizeof(struct FILEIndex), 1, pIndex);
	fclose(pIndex);
	tidy_index(&fdx);

	Syslog('+', "Index Areas [%5d] Files [%5d]", iAreasNew, iTotal);

	if (!do_quiet) {
		printf("\r                                                          \r");
		fflush(stdout);
	}

	free(sIndex);
	free(sAreas);
	free(fAreas);
	RemoveSema((char *)"reqindex");
}



