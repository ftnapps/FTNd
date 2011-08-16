/*****************************************************************************
 *
 * $Id: nodelock.c,v 1.13 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Node locking
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:	2:280/2802
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



int nodelock(faddr *addr, pid_t mypid)
{
    char    *fn, *tfn, *p, tmp[16], *progname;
    FILE    *fp;
    pid_t   pid;
    int	    tmppid, sverr, rc;
    time_t  ltime, now;

    fn = bsyname(addr);
    tfn = xstrcpy(fn);
    if ((p=strrchr(tfn,'/'))) 
	*++p='\0';
    snprintf(tmp, 16, "aa%d", mypid);
    tfn = xstrcat(tfn, tmp);
    mkdirs(tfn, 0770);

    if ((fp = fopen(tfn,"w")) == NULL) {
	WriteError("$Can't open tmp file for bsy lock (%s) \"%s\"",ascfnode(addr, 0x1f), tfn);
	free(tfn);
	return 1;
    }

    fprintf(fp,"%10d\n", mypid);
    fclose(fp);
    chmod(tfn, 0440);
    if (link(tfn, fn) == 0) {
	unlink(tfn);
	free(tfn);
	return 0;
    } else {
	sverr = errno;
    }

    if (sverr != EEXIST) {
	WriteError("$Could not link \"%s\" to \"%s\"",tfn,fn);
	WriteError("Locking %s failed", ascfnode(addr, 0x1f));
	unlink(tfn);
	free(tfn);
	return 1;
    }

    if ((fp = fopen(fn,"r")) == NULL) {
	WriteError("$Could not open existing lock file \"%s\"",fn);
	WriteError("Locking %s failed", ascfnode(addr, 0x1f));
	unlink(tfn);
	free(tfn);
	return 1;
    }

    /*
     * Lock exists, check owner. If rc <> 1 then the lock may have
     * been created by another OS (zero bytes lock).
     */
    rc = fscanf(fp, "%d", &tmppid);
    pid = tmppid;
    fclose(fp);

    /*
     * If lock is our own lock, then it's ok and we are ready.
     */
    if (mypid == pid) {
	unlink(tfn);
	free(tfn);
	return 0;
    }

    /*
     * Stale or old lock tests
     */
    ltime = file_time(fn);
    now = time(NULL);
    if (CFG.ZeroLocks && (rc != 1) && (((unsigned int)now - (unsigned int)ltime) > 21600)) {
	Syslog('+', "Found zero byte lock older then 6 hours for %s, unlink", ascfnode(addr,0x1f));
	unlink(fn);
    } else if (CFG.ZeroLocks && (rc != 1)) {
	Syslog('+', "Node %s is locked from another OS", ascfnode(addr, 0x1f));
	unlink(tfn);
	free(tfn);
	return 1;
    } else if (kill(pid, 0) && (errno == ESRCH)) {
	Syslog('+', "Found stale bsy file for %s, unlink", ascfnode(addr,0x1f));
	unlink(fn);
    } else if (((unsigned int)now - (unsigned int)ltime) > 21600) {
	Syslog('+', "Found lock older then 6 hours for %s, unlink", ascfnode(addr,0x1f));
	unlink(fn);
    } else {
	progname = calloc(PATH_MAX, sizeof(char));
	if (pid2prog(pid, progname, PATH_MAX) == 0)
	    Syslog('+', "Node %s is locked by pid %d (%s)", ascfnode(addr, 0x1f), pid, progname);
	else
	    Syslog('+', "Node %s is locked by pid %d", ascfnode(addr, 0x1f), pid);
	free(progname);
	unlink(tfn);
	free(tfn);
	return 1;
    }

    if (link(tfn,fn) == 0) {
	unlink(tfn);
	free(tfn);
	return 0;
    } else {
	WriteError("$Could not link \"%s\" to \"%s\"",tfn,fn);
	WriteError("Locking %s failed", ascfnode(addr, 0x1f));
	unlink(tfn);
	free(tfn);
	return 1;
    }
}



int nodeulock(faddr *addr, pid_t mypid)
{
    char    *fn;
    FILE    *fp;
    pid_t   pid;
    int	    tmppid = 0, rc;

    fn = bsyname(addr);
    if ((fp = fopen(fn, "r")) == NULL) {
	Syslog('+', "Unlock %s failed, not locked", ascfnode(addr, 0x1f));
	return 1;
    }

    rc = fscanf(fp, "%d", &tmppid);
    pid = tmppid;
    fclose(fp);

    if (CFG.ZeroLocks && (rc != 1)) {
	/*
	 * Zero byte lock from another OS, leave alone.
	 */
	return 0;
    } else if (pid == mypid) {
	unlink(fn);
	return 0;
    } else {
	WriteError("Unlock (%s) file failed for process %u, we are %u", ascfnode(addr, 0x1f), pid,mypid);
	return 1;
    }
}


