/*****************************************************************************
 *
 * File ..................: dbdupe.c
 * Purpose ...............: Dupe checking.
 * Last modification date : 25-May-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "clcomm.h"
#include "dbdupe.h"


typedef struct _dupesrec {
	unsigned long	*crcs;
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
    for (i = 0; i < 3; i++) {
	dupes[i].crcs= NULL;
	dupes[i].loaded = FALSE;
	dupes[i].changed = FALSE;
	dupes[i].count = 0;
	dupes[i].max = 0;
    }
}



int CheckDupe(unsigned long crc, int idx, int max)
{
    char	    *dfile;
    FILE	    *fil;
    unsigned long   test;
    int		    i, size = 0;

    if (!dupes[idx].loaded) {
	dfile = calloc(PATH_MAX, sizeof(char));
	sprintf(dfile, "%s/etc/%s.dupe", getenv("MBSE_ROOT"), files[idx]);
	if ((fil = fopen(dfile, "r+")) == NULL) {
	    /*
	     * Dupe database doesn't exist yet.
	     */
	    if ((fil = fopen(dfile, "w")) == NULL) {
		WriteError("$PANIC: dbdupe.c, can't create %s", dfile);
		free(dfile);
		exit(1);
	    }
	    fclose(fil);
	    fil = fopen(dfile, "r+");
	} else {
	    fseek(fil, 0L, SEEK_END);
	    size = ftell(fil) / sizeof(unsigned long);
	    fseek(fil, 0L, SEEK_SET);
	}

	/*
	 * Reserve some extra memory and record howmuch.
	 */
	if (size > max)
	    dupes[idx].peak = size + 5000;
	else
	    dupes[idx].peak = max + 5000;
	dupes[idx].crcs = (unsigned long *)malloc(dupes[idx].peak * sizeof(unsigned long));

	/*
	 *  Load dupe records
	 */
	while (fread(&test, sizeof(test), 1, fil) == 1) {
	    dupes[idx].crcs[dupes[idx].count] = test;
	    dupes[idx].count++;
	}
	Syslog('n', "Loaded %d dupe records in %s", dupes[idx].count++, files[idx]);
	fclose(fil);
	free(dfile);
	dupes[idx].loaded = TRUE;
	dupes[idx].max = max;
    }

    Syslog('n', "dupetest %08x %s %d", crc, files[idx], max);

    for (i = 0; i < dupes[idx].count; i++) {
	if (dupes[idx].crcs[i] == crc) {
	    Syslog('n', "dupe at %d", i);
	    return TRUE;
	}
    }
    /*
     * Not a dupe, append new crc value
     */
    dupes[idx].crcs[dupes[idx].count] = crc;
    Syslog('n', "Added new dupe at %d", dupes[idx].count);
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
    Syslog('n', "Checking %s.dupe", files[idx]);
    if (dupes[idx].loaded) {
	if (dupes[idx].changed) {
	    if (dupes[idx].count > dupes[idx].max)
		start = dupes[idx].count - dupes[idx].max;
	    else
		start = 0;
	    sprintf(dfile, "%s/etc/%s.dupe", getenv("MBSE_ROOT"), files[idx]);
	    if ((fil = fopen(dfile, "w"))) {
		Syslog('n', "Writing dupes %d to %d", start, dupes[idx].count);
		for (j = start; j < dupes[idx].count; j++)
		    fwrite(&dupes[idx].crcs[j], sizeof(unsigned long), 1, fil);
		fclose(fil);
	    } else {
		WriteError("$Can't write %s", dfile);
	    }
	} else {
	    Syslog('n', "Not changed so not saved");
	}

	dupes[idx].changed = FALSE;
	dupes[idx].loaded  = FALSE;
	dupes[idx].count = 0;
	dupes[idx].max   = 0;
	dupes[idx].peak  = 0;
	free(dupes[idx].crcs);
	dupes[idx].crcs = NULL;

    } else {
	Syslog('n', "Not loaded");
    }
    free(dfile);
}



void CloseDupes()
{
    int	i;

    Syslog('n', "Closing dupes databases");
    for (i = 0; i < 3; i++)
	CloseDdb(i);
}

