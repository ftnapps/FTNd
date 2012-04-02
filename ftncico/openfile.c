/*****************************************************************************
 *
 * $Id: openfile.c,v 1.14 2008/11/26 22:01:01 mbse Exp $
 * Purpose ...............: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
#include "config.h"
#include "lutil.h"
#include "openfile.h"


static FILE	*infp=NULL;
static char	*infpath=NULL;
static time_t	intime;
static int	isfreq;
char		*freqname=NULL;
int		gotfiles = FALSE;
extern char	*tempinbound;


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
    char	    *opentype, *p, x, ctt[32], tmpfname[16];
    int		    rc, rsc, ncount;
    struct stat	    st;

    strcpy(ctt,date(remtime));

    Syslog('s', "openfile(\"%s\",%s,%lu,...)", MBSE_SS(fname), ctt, (unsigned int)remsize);

    if ((fname == NULL) || (fname[0] == '\0')) {
	snprintf(tmpfname,16,"%08x.pkt",sequencer());
	fname=tmpfname;
    }

    if ((strlen(fname) == 12) && (strspn(fname,"0123456789abcdefABCDEF") == 8) && (strcasecmp(fname+8,".req") == 0)) {
	Syslog('s', "Received wazoo freq file");
	isfreq = TRUE;
    } else 
	isfreq = FALSE;

    /*
     * First check if the file is already in the inbound directory.
     * If it's there, resoffs will be set equal to remsize to signal the
     * receiving protocol to skip the file.
     */
    if (tempinbound == NULL) {
	/*
	 * This is when we get a FTS-0001 handshake packet
	 */
	infpath = xstrcpy(CFG.inbound);
	infpath = xstrcat(infpath, (char *)"/tmp/");
	mkdirs(infpath, 0700);
    } else {
	infpath = xstrcpy(tempinbound);
	infpath = xstrcat(infpath, (char *)"/");
    }
    infpath = xstrcat(infpath, fname);
    if (stat(infpath, &st) == 0) {
	Syslog('s', "remtine=%ld, st_time=%ld, remsize=%ld, st_size=%ld", remtime, st.st_mtime, remsize, st.st_size);

	if ((remtime == st.st_mtime) && (remsize == st.st_size)) {
	    Syslog('+', "File %s is already here", fname);
	    *resofs = st.st_size;
	    free(infpath);
	    infpath = NULL;
	    return NULL;
	} 
    }

    /*
     * If the file is in the inbound with a zero length, erase the
     * file as if it wasn't there at all.
     */
    if (((rc = stat(infpath, &st)) == 0) && (st.st_size == 0)) {
	Syslog('+', "Zero bytes file in the inbound, unlinking");
	unlink(infpath);
    }

    /*
     * If the file is not already in the inbound, but there is a file
     * with the same name, the new file will be renamed if the file
     * has another timestamp as the file we expect.
     *
     * Renaming algorythm is as follows: start with the present name,
     * increase the last character of the file name, jumping from 
     * '9' to 'a', from 'z' to 'A', from 'Z' to '0'. If _all_ these 
     * names are occupied, create random name.
     */
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
	snprintf(ctt,32,"%08x.doe",sequencer());
	free(infpath);
	infpath = xstrcpy(p);
	infpath = xstrcat(infpath, ctt);
    }
    if (ncount)
	Syslog('s', "File renamed to %s", infpath);
    
    *resofs = 0L;
    opentype = (char *)"w";
    if ((rc == 0) && (remsize != 0)) {
	rsc = resync(st.st_size);
	Syslog('s', "resync(%d) rsc=%d", (int)st.st_size, rsc);
	switch (rsc) {
	    case 0:	/* Success  */
			opentype = (char *)"a+";
			*resofs = st.st_size;
			Syslog('+', "Resyncing at offset %lu of \"%s\"", (unsigned int)st.st_size, infpath);
			break;
	    case -1:	/* Binkp did send a GET, return here and do not open file */
			free(infpath);
			infpath = NULL;
			return NULL;
	    default:	/* Error from xmrecv, do nothing    */
			break;
	}
    }
    Syslog('s', "try fopen(\"%s\",\"%s\")", infpath, opentype);

    if ((infp = fopen(infpath, opentype)) == NULL) {
	WriteError("$Cannot open local file \"%s\" for \"%s\"", infpath,opentype);
	free(infpath);
	infpath=NULL;
	return NULL;
    }
    intime = remtime;

    if (isfreq) {
	if (freqname) 
	    free(freqname);
	freqname = xstrcpy(infpath);
    }

    Syslog('s', "opened file \"%s\" for \"%s\", restart at %lu", infpath,opentype,(unsigned int)*resofs);
    return infp;
}



/*
 *  close file, even if the file is partial received, we set the date on the
 *  file so that in a next session we know we must append to that file instead of
 *  trying to get the file again.
 */
int closefile(void)
{
    int		    rc = 0;
    struct utimbuf  ut;

    Syslog('s', "closefile(), for file \"%s\"", MBSE_SS(infpath));

    if ((infp == NULL) || (infpath == NULL)) {
	Syslog('+', "closefile(), nothing to close");
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
	if (rc != 0) {
	    Syslog('+', "Removing unsuccessfuly received wazoo freq");
	    unlink(freqname);
	    free(freqname);
	    freqname=NULL;
	}
	isfreq = FALSE;
    } 
	
    free(infpath);
    infpath = NULL;
    return rc;
}


