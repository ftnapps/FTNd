/*****************************************************************************
 *
 * $Id: dbdupe.c,v 1.14 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: Dupe checking.
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
#include "mbselib.h"
#include "mbse.h"
#include "users.h"
#include "mbsedb.h"



typedef struct _dupesrec {
	unsigned int	*crcs;
	int		loaded;
	int		changed;
	int		count;
	int		max;
	int		peak;
} dupesrec;


dupesrec	dupes[3];
static char	*files[] = {(char *)"echomail", (char *)"fileecho", (char *)"news"};
void		CloseDdb(int);



void InitDupes()
{
    int	i;

    Syslog('n', "Init Dupes");
    memset(dupes, 0, sizeof(dupes));

    for (i = 0; i < 3; i++) {
	dupes[i].crcs= NULL;
	dupes[i].loaded = FALSE;
	dupes[i].changed = FALSE;
	dupes[i].count = 0;
	dupes[i].max = 0;
    }
}



int CheckDupe(unsigned int crc, int idx, int max)
{
    char	    *dfile;
    FILE	    *fil;
    unsigned int    test;
    int		    i, size = 0;

    if (!dupes[idx].loaded) {
	dfile = calloc(PATH_MAX, sizeof(char));
	snprintf(dfile, PATH_MAX -1, "%s/etc/%s.dupe", getenv("MBSE_ROOT"), files[idx]);
	if ((fil = fopen(dfile, "r+")) == NULL) {
	    /*
	     * Dupe database doesn't exist yet.
	     */
	    if ((fil = fopen(dfile, "w")) == NULL) {
		WriteError("$PANIC: dbdupe.c, can't create %s", dfile);
		free(dfile);
		exit(MBERR_INIT_ERROR);
	    }
	    fclose(fil);
	    fil = fopen(dfile, "r+");
	} else {
	    fseek(fil, 0L, SEEK_END);
	    size = ftell(fil) / sizeof(unsigned int);
	    fseek(fil, 0L, SEEK_SET);
	}

	/*
	 * Reserve some extra memory and record howmuch.
	 */
	if (size > max)
	    dupes[idx].peak = size + 5000;
	else
	    dupes[idx].peak = max + 5000;
	dupes[idx].crcs = (unsigned int *)malloc(dupes[idx].peak * sizeof(unsigned int));
	memset(dupes[idx].crcs, 0, dupes[idx].peak * sizeof(unsigned int));

	/*
	 *  Load dupe records
	 */
	while (fread(&test, sizeof(test), 1, fil) == 1) {
	    dupes[idx].crcs[dupes[idx].count] = test;
	    dupes[idx].count++;
	}
	fclose(fil);
	free(dfile);
	dupes[idx].loaded = TRUE;
	dupes[idx].max = max;
    }

    for (i = 0; i < dupes[idx].count; i++) {
	if (dupes[idx].crcs[i] == crc) {
	    return TRUE;
	}
    }
    /*
     * Not a dupe, append new crc value
     */
    dupes[idx].crcs[dupes[idx].count] = crc;
    dupes[idx].count++;
    dupes[idx].changed = TRUE;

    /*
     * If we reach the high limit, flush the current dupelist.
     */
    if (dupes[idx].count >= dupes[idx].peak)
	CloseDdb(idx);
    return FALSE;
}



void CloseDdb(int idx)
{
    int	    j, start;
    char    *dfile;
    FILE    *fil;

    dfile = calloc(PATH_MAX, sizeof(char));
    if (dupes[idx].loaded) {
	if (dupes[idx].changed) {
	    if (dupes[idx].count > dupes[idx].max)
		start = dupes[idx].count - dupes[idx].max;
	    else
		start = 0;
	    snprintf(dfile, PATH_MAX -1, "%s/etc/%s.dupe", getenv("MBSE_ROOT"), files[idx]);
	    if ((fil = fopen(dfile, "w"))) {
		for (j = start; j < dupes[idx].count; j++)
		    fwrite(&dupes[idx].crcs[j], sizeof(unsigned int), 1, fil);
		fclose(fil);
	    } else {
		WriteError("$Can't write %s", dfile);
	    }
	}

	dupes[idx].changed = FALSE;
	dupes[idx].loaded  = FALSE;
	dupes[idx].count = 0;
	dupes[idx].max   = 0;
	dupes[idx].peak  = 0;
	free(dupes[idx].crcs);
	dupes[idx].crcs = NULL;

    }
    free(dfile);
}



void CloseDupes()
{
    int	i;

    for (i = 0; i < 3; i++)
	CloseDdb(i);
}

