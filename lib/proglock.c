/*****************************************************************************
 *
 * proglock.c
 * Purpose ...............: Program Locking
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
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
#include "ftndlib.h"


extern		int do_quiet;		/* Quiet flag			    */


/*
 *  Put a lock on a program.
 */
int lockprogram(char *progname)
{
    char    *tempfile, *lockfile;
    FILE    *fp;
    pid_t   oldpid;

    tempfile = calloc(PATH_MAX, sizeof(char));
    lockfile = calloc(PATH_MAX, sizeof(char));

    snprintf(tempfile, PATH_MAX -1, "%s/var/run/%s.tmp", getenv("FTND_ROOT"), progname);
    snprintf(lockfile, PATH_MAX -1, "%s/var/run/%s", getenv("FTND_ROOT"), progname);

    if ((fp = fopen(tempfile, "w")) == NULL) {
	WriteError("$Can't create lockfile \"%s\"", tempfile);
	free(tempfile);
	free(lockfile);
	return 1;
    }
    fprintf(fp, "%10u\n", getpid());
    fclose(fp);

    while (TRUE) {
	if (link(tempfile, lockfile) == 0) {
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 0;
	}
	if ((fp = fopen(lockfile, "r")) == NULL) {
	    WriteError("$Can't open lockfile \"%s\"", lockfile);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
	if (fscanf(fp, "%u", &oldpid) != 1) {
	    WriteError("$Can't read old pid from \"%s\"", tempfile);
	    fclose(fp);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
	fclose(fp);
	if (kill(oldpid,0) == -1) {
	    if (errno == ESRCH) {
		Syslog('+', "Stale lock found for pid %u", oldpid);
		unlink(lockfile);
		/* no return, try lock again */  
	    } else {
		WriteError("$Kill for %u failed", oldpid);
		unlink(tempfile);
		free(tempfile);
		free(lockfile);
		return 1;
	    }
	} else {
	    Syslog('+', "%s already running, pid=%u", progname, oldpid);
	    if (!do_quiet)
		printf("Another %s is already running.\n", progname);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
    }
}



void ulockprogram(char *progname)
{
    char    *lockfile;
    FILE    *fp;
    pid_t   oldpid;

    lockfile = calloc(PATH_MAX, sizeof(char));
    snprintf(lockfile, PATH_MAX -1, "%s/var/run/%s", getenv("FTND_ROOT"), progname);

    if ((fp = fopen(lockfile, "r")) == NULL) {
	WriteError("$Can't open lockfile \"%s\"", lockfile);
	free(lockfile);
	return;
    }

    if (fscanf(fp, "%u", &oldpid) != 1) {
	WriteError("$Can't read old pid from \"%s\"", lockfile);
    }

    fclose(fp);
    unlink(lockfile);
    free(lockfile);
    lockfile = NULL;
}

