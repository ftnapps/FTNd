/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "config.h"
#include "lutil.h"
#include "openfile.h"


static FILE	*infp=NULL;
static char	*infpath=NULL;
static time_t	intime;
static int	isfreq;
char		*freqname=NULL;
int		gotfiles = FALSE;


/*
 * Try to find present (probably incomplete) file with the same timestamp
 * (it might be renamed), open it for append and store resync offset.
 * Store 0 in resync offset if the file is new. Return FILE* or NULL.
 * resync() must accept offset in bytes and return 0 on success, nonzero
 * otherwise (and then the file will be open for write).  For Zmodem,
 * resync is always possible, but for SEAlink it is not.  Do not try
 * any resyncs if remsize == 0.
 */
FILE *openfile(char *fname, time_t remtime, off_t remsize, off_t *resofs, int(*resync)(off_t))
{
	char		*opentype;
	char		*p, x;
	char		ctt[32];
	int		rc, ncount;
	struct stat	st;
	struct flock	fl;
	char		tmpfname[16];

	fl.l_type   = F_WRLCK;
	fl.l_whence = 0;
	fl.l_start  = 0L;
	fl.l_len    = 0L;
	strcpy(ctt,date(remtime));

	Syslog('S', "openfile(\"%s\",%s,%lu,...)", MBSE_SS(fname), MBSE_SS(ctt),(unsigned long)remsize);

	if ((fname == NULL) || (fname[0] == '\0')) {
		sprintf(tmpfname,"%08lx.pkt",(unsigned long)sequencer());
		fname=tmpfname;
	}

	if ((strlen(fname) == 12) && (strspn(fname,"0123456789abcdefABCDEF") == 8) && (strcasecmp(fname+8,".req") == 0)) {
		Syslog('s', "Received wazoo freq file");
		isfreq = TRUE;
	} else 
		isfreq = FALSE;

	/*
	 * First check if the file is already in the real inbound directory.
	 * If it's there, resoffs will be set equal to remsize to signal the
	 * receiving protocol to skip the file.
	 */
	if (infpath)
		free(infpath);
	infpath = xstrcpy(inbound);
	infpath = xstrcat(infpath, (char *)"/");
	infpath = xstrcat(infpath, fname);
	if (stat(infpath, &st) == 0) {
		Syslog('S', "remtine=%ld, st_time=%ld, remsize=%ld, st_size=%ld", remtime, st.st_mtime, remsize, st.st_size);

		if ((remtime == st.st_mtime) && (remsize == st.st_size)) {
			Syslog('+', "File %s is already here", fname);
			*resofs = st.st_size;
			free(infpath);
			infpath = NULL;
			return NULL;
		} 
	}

	/*
	 * If the file is not already in the inbound, but there is a file
	 * with the same name, the new file will be renamed.
	 *
	 * If the file is 0 bytes, erase it so it can be received again.
	 *
	 * Renaming algorythm is as follows: start with the present name,
	 * increase the last character of the file name, jumping from 
	 * '9' to 'a', from 'z' to 'A', from 'Z' to '0'. If _all_ these 
	 * names are occupied, create random name.
	 */
	if (infpath)
		free(infpath);
	infpath = xstrcpy(inbound);
	infpath = xstrcat(infpath, (char *)"/tmp/");
	infpath = xstrcat(infpath, fname);

	if (((rc = stat(infpath, &st)) == 0) && (st.st_size == 0)) {
		Syslog('+', "Zero bytes file in the inbound, unlinking");
		unlink(infpath);
	}

	p = infpath + strlen(infpath) -1;
	x = *p;
	ncount = 0;
	while (((rc = stat(infpath, &st)) == 0) && (remtime != st.st_mtime) && (ncount++ < 62)) {
		if (x == '9') 
			x = 'a';
		else if (x == 'z') 
			x = 'A';
		else if (x == 'Z') 
			x = '0';
		else 
			x++;
		*p = x;
	}

	if (ncount >= 62) { /* names exhausted */
		rc = 1;
		p = strrchr(infpath,'/');
		*p = '\0';
		sprintf(ctt,"%08lx.doe",(unsigned long)sequencer());
		free(infpath);
		infpath = xstrcpy(p);
		infpath = xstrcat(infpath, ctt);
	}

	*resofs = 0L;
	opentype = (char *)"w";
	if ((rc == 0) && (remsize != 0)) {
		Syslog('+', "Resyncing at offset %lu of \"%s\"", (unsigned long)st.st_size, infpath);
		if (resync(st.st_size) == 0) {
			opentype = (char *)"a";
			*resofs = st.st_size;
		}
	}

	Syslog('S', "try fopen(\"%s\",\"%s\")",infpath,opentype);

	/*
	 * If first attempt doesn't succeed, create tmp directory
	 * and try again.
	 */
	if ((infp = fopen(infpath,opentype)) == NULL) {
		mkdirs(infpath, 0770);
		if ((infp = fopen(infpath, opentype)) == NULL) {
			WriteError("$Cannot open local file \"%s\" for \"%s\"", infpath,opentype);
			free(infpath);
			infpath=NULL;
			return NULL;
		}
	}

	fl.l_pid = getpid();
	if (fcntl(fileno(infp),F_SETLK,&fl) != 0) {
		Syslog('+', "$cannot lock local file \"%s\"",infpath);
		fclose(infp);
		infp = NULL;
		free(infpath);
		infpath = NULL;
		return NULL;
	}
	intime=remtime;

	if (isfreq) {
		if (freqname) 
			free(freqname);
		freqname = xstrcpy(infpath);
	}

	Syslog('S', "opened file \"%s\" for \"%s\", restart at %lu", infpath,opentype,(unsigned long)*resofs);
	return infp;
}



/*
 *  close file and if (success) { move it to the final location }
 */
int closefile(int success)
{
	char		*newpath, *p, ctt[32];
	int		rc=0,ncount;
	char		x;
	struct stat	st;
	struct		utimbuf ut;

	Syslog('S', "closefile(%d), for file \"%s\"",success, MBSE_SS(infpath));

	if ((infp == NULL) || (infpath == NULL)) {
		WriteError("Internal error: try close unopened file!");
		return 1;
	}

	rc = fclose(infp);
	infp = NULL;

	if (rc == 0) {
		ut.actime = intime;
		ut.modtime = intime;
		if ((rc = utime(infpath,&ut)))
			WriteError("$utime failed");
	}

	if (isfreq) {
		if ((rc != 0) || (!success)) {
			Syslog('+', "Removing unsuccessfuly received wazoo freq");
			unlink(freqname);
			free(freqname);
			freqname=NULL;
		}
		isfreq = FALSE;
	} else if ((rc == 0) && success) {
		newpath = xstrcpy(inbound);
		newpath = xstrcat(newpath, strrchr(infpath,'/'));
		
		p = newpath + strlen(newpath) - 1;
		x = *p;
		ncount = 0;
		while (((rc = stat(newpath, &st)) == 0) && (ncount++ < 62)) {
			if (x == '9') 
				x = 'a';
			else if (x == 'z') 
				x = 'A';
			else if (x == 'Z') 
				x = '0';
			else 
				x++;
			*p = x;
		}
		if (ncount >= 62) { /* names exhausted */
			rc = 1;
			p = strrchr(newpath,'/');
			*p = '\0';
			sprintf(ctt,"%08lx.doe",(unsigned long)sequencer());
			free(newpath);
			newpath = xstrcpy(p);
			newpath = xstrcat(newpath, ctt);
		}

		Syslog('S', "moving \"%s\" -> \"%s\"", MBSE_SS(infpath), MBSE_SS(newpath));
		rc = rename(infpath, newpath);
		if (rc) 
			WriteError("$error renaming \"%s\" -> \"%s\"", MBSE_SS(infpath),MBSE_SS(newpath));
		else
			gotfiles = TRUE;
		free(newpath);
	}
	free(infpath);
	infpath = NULL;
	return rc;
}


