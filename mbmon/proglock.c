/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Program Locking
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/mberrors.h"
#include "../lib/structs.h"
#include "common.h"
#include "proglock.h"




/*
 *  Put a lock on a program.
 */
int lockprogram(char *progname)
{
    char    *tempfile, *lockfile;
    FILE    *fp;
    pid_t   oldpid;

    tempfile  = calloc(PATH_MAX, sizeof(char));
    lockfile = calloc(PATH_MAX, sizeof(char));

    sprintf(tempfile, "%s/var/run/%s.tmp", getenv("MBSE_ROOT"), progname);
    sprintf(lockfile, "%s/var/run/%s", getenv("MBSE_ROOT"), progname);

    if ((fp = fopen(tempfile, "w")) == NULL) {
	Syslog('?', "$Can't create lockfile \"%s\"", tempfile);
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
	    Syslog('?', "$Can't open lockfile \"%s\"", lockfile);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
	if (fscanf(fp, "%u", &oldpid) != 1) {
	    Syslog('?', "$Can't read old pid from \"%s\"", tempfile);
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
		Syslog('?', "$Kill for %u failed", oldpid);
		unlink(tempfile);
		free(tempfile);
		free(lockfile);
		return 1;
	    }
	} else {
	    Syslog('+', "%s already running, pid=%u", progname, oldpid);
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
    pid_t   oldpid;
    FILE    *fp;

    lockfile = calloc(PATH_MAX, sizeof(char));
    sprintf(lockfile, "%s/var/run/%s", getenv("MBSE_ROOT"), progname);

    if ((fp = fopen(lockfile, "r")) == NULL) {
	Syslog('?', "$Can't open lockfile \"%s\"", lockfile);
	free(lockfile);
	return;
    }
    if (fscanf(fp, "%u", &oldpid) != 1) {
	Syslog('?', "$Can't read old pid from \"%s\"", lockfile);
	fclose(fp);
	unlink(lockfile);
	free(lockfile);
	return;
    }

    if (oldpid == getpid()) {
	(void)unlink(lockfile);
    } else {
	Syslog('?', "Lockfile owned by pid %d, not removed", oldpid);
    }

    free(lockfile);
}

