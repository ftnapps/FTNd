/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Node locking
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "clcomm.h"
#include "common.h"



int nodelock(faddr *addr)
{
	char	*fn,*tfn,*p;
	char	tmp[16];
	FILE	*fp;
	pid_t	pid,mypid;
	int	tmppid,sverr;

	fn = bsyname(addr);
	tfn = xstrcpy(fn);
	if ((p=strrchr(tfn,'/'))) 
		*++p='\0';
	mypid = getpid();
	sprintf(tmp, "aa%d", mypid);
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
		sverr=errno;
	}

	if (sverr != EEXIST) {
		WriteError("$Could not link \"%s\" to \"%s\"",tfn,fn);
		WriteError("Locking %s failed", ascfnode(addr, 0x1f));
		unlink(tfn);
		free(tfn);
		return 1;
	}

	if ((fp=fopen(fn,"r")) == NULL) {
		WriteError("$Could not open existing lock file \"%s\"",fn);
		WriteError("Locking %s failed", ascfnode(addr, 0x1f));
		unlink(tfn);
		free(tfn);
		return 1;
	}

	/*
	 * Lock exists, check owner
	 */
	fscanf(fp, "%d", &tmppid);
	pid = tmppid;
	fclose(fp);

	/*
	 * If lock is our own lock, then it's ok and we are ready.
	 */
	if (getpid() == pid) {
		unlink(tfn);
		free(tfn);
		return 0;
	}

	if (kill(pid, 0) && (errno == ESRCH)) {
		Syslog('+', "Found stale bsy file for %s, unlink", ascfnode(addr,0x1f));
		unlink(fn);
	} else {
		Syslog('+', "Node %s is locked by pid %d", ascfnode(addr, 0x1f), pid);
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



int nodeulock(faddr *addr)
{
	char	*fn;
	FILE	*fp;
	pid_t	pid,mypid;
	int	tmppid;

	fn = bsyname(addr);
	if ((fp = fopen(fn, "r")) == NULL) {
		WriteError("$Can't open lock file (%s) \"%s\"", ascfnode(addr, 0x1f), fn);
		return 1;
	}

	fscanf(fp, "%d", &tmppid);
	pid = tmppid;
	fclose(fp);
	mypid = getpid();

	if (pid == mypid) {
		unlink(fn);
		return 0;
	} else {
		WriteError("Unlock (%s) file failed for process %u, we are %u", ascfnode(addr, 0x1f), pid,mypid);
		return 1;
	}
}


