/*****************************************************************************
 *
 * $Id: ulock.c,v 1.12 2005/08/28 13:45:26 mbse Exp $
 * Purpose ...............: Fidonet mailer
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
#include "../lib/mbselib.h"

#ifndef LOCKDIR
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define LOCKDIR "/var/spool/lock"
#else
#define LOCKDIR "/var/lock"
#endif
#endif

#define LCKPREFIX LOCKDIR"/LCK.."
#define LCKTMP LOCKDIR"/TMP."



int lock(char *line)
{
	pid_t	mypid,rempid=0;
	int	tmppid;
	char	tmpname[256],lckname[256];
	char	*p;
	int	i,rc;
	FILE	*f;

	rc=-1;
	if ((p=strrchr(line,'/')) == NULL) 
		p=line; 
	else 
		p++;
	mypid = getpid();
	snprintf(tmpname,256,"%s%d",LCKTMP,mypid);
	if ((f = fopen(tmpname,"w")) == NULL) {
		WriteError("$ulock: can't create %s",tmpname);
		return(-1);
	}

	fprintf(f,"%10d\n",mypid);
	fclose(f);
	chmod(tmpname,0444);
	snprintf(lckname,256,"%s%s",LCKPREFIX,p);
	p=lckname+strlen(lckname)-1;
	*p=tolower(*p);

	for (i=0; (i++<5) && ((rc = link(tmpname, lckname)) != 0) && 
				(errno == EEXIST);)
	{
		if ((f=fopen(lckname,"r")) == NULL) {
			Syslog('l', "$Can't open existing lock file");
		} else {
			fscanf(f,"%d",&tmppid);
			rempid=tmppid;
			fclose(f);
			Syslog('l', "lock: file read for process %d",rempid);
		}

		if (kill(rempid,0) && (errno ==  ESRCH)) {
			Syslog('l', "process inactive, unlink file");
			unlink(lckname);
		} else {
			Syslog('l', "process active, sleep a bit");
			sleep(2);
		}
	}

	if (rc)
		Syslog('l', "$ulock: result %d (errno %d)",rc,errno);
	unlink(tmpname);
	return(rc);
}



int ulock(char *line)
{
	pid_t	mypid,rempid;
	int	tmppid;
	char	lckname[256];
	char	*p;
	int	rc;
	FILE	*f;

	rc=-1;
	if ((p=strrchr(line,'/')) == NULL) 
		p=line; 
	else 
		p++;
	mypid=getpid();
	snprintf(lckname,256,"%s%s",LCKPREFIX,p);
	p=lckname+strlen(lckname)-1;
	*p=tolower(*p);

	if ((f=fopen(lckname,"r")) == NULL)
	{
		WriteError("$cannot open lock file %s",lckname);
		return rc;
	}

	fscanf(f,"%d",&tmppid);
	rempid=tmppid;
	fclose(f);
	if (rempid ==  mypid) {
		rc = unlink(lckname);
		if (rc)
			Syslog('l', "Unlock %s rc=%d", lckname, rc);
	}
	return(rc);
}


