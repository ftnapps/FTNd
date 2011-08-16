/*****************************************************************************
 *
 * $Id: bopenfile.c,v 1.4 2008/11/26 22:01:01 mbse Exp $
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
#include "bopenfile.h"


static FILE	*infp=NULL;
static char	*infpath=NULL;
static time_t	intime;
static int	isfreq;
extern char	*freqname;
extern int	gotfiles;
extern char	*tempinbound;


/*
 * Openfile special for binkp sessions.
 */
FILE *bopenfile(char *fname, time_t remtime, off_t remsize, off_t *resofs)
{
    char	    ctt[32], *temp;
    struct stat	    st;
    FILE	    *fp;

    strcpy(ctt,date(remtime));

    Syslog('b', "Binkp: bopenfile(\"%s\",%s,%u)", MBSE_SS(fname), ctt, (unsigned int)remsize);

    if ((fname == NULL) || (fname[0] == '\0')) {
	Syslog('+', "Binkp: bopenfile fname=NULL");
	return NULL;
    }
    if (tempinbound == NULL) {
	Syslog('+', "Binkp: bopenfile tempinbound is not set");
	return NULL;
    }

    if ((strlen(fname) == 12) && (strspn(fname,"0123456789abcdefABCDEF") == 8) && (strcasecmp(fname+8,".req") == 0)) {
	Syslog('b', "Binkp: received wazoo freq file");
	isfreq = TRUE;
    } else 
	isfreq = FALSE;

    infpath = xstrcpy(tempinbound);
    infpath = xstrcat(infpath, (char *)"/tmp/");
    infpath = xstrcat(infpath, fname);
    *resofs = 0L;

    /*
     * Set offset if (partial) fie is present
     */
    if (stat(infpath, &st) == 0) {
	Syslog('b', "Binkp: remtine=%ld, st_time=%ld, remsize=%ld, st_size=%ld", remtime, st.st_mtime, remsize, st.st_size);
	Syslog('+', "Binkp: file %s is already here", fname);
	*resofs = st.st_size;
    }

    Syslog('b', "Binkp: try fopen(\"%s\")", infpath);
    if ((infp = fopen(infpath, "a+")) == NULL) {
	WriteError("$Binkp: cannot open local file \"%s\"", infpath);
	free(infpath);
	infpath=NULL;
	return NULL;
    }

    /*
     * Write state file
     */
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s.state", infpath);
    if ((fp = fopen(temp, "w"))) {
	fprintf(fp, "%u\n", (unsigned int)remtime);
	fprintf(fp, "%u\n", (unsigned int)remsize);
	fclose(fp);
    }
    free(temp);

    intime = remtime;

    if (isfreq) {
	if (freqname) 
	    free(freqname);
	freqname = xstrcpy(tempinbound);
	freqname = xstrcat(freqname, (char *)"/");
	freqname = xstrcat(freqname, fname);
    }

    Syslog('b', "Binkp: opened file \"%s\", restart at %u", infpath, (unsigned int)*resofs);
    return infp;
}



/*
 *  close file, even if the file is partial received, we set the date on the
 *  file so that in a next session we know we must append to that file instead of
 *  trying to get the file again.
 */
int bclosefile(int success)
{
    int		    rc = 0;
    struct utimbuf  ut;
    char	    *temp;
    
    Syslog('b', "Binkp: closefile(), for file \"%s\"", MBSE_SS(infpath));

    if ((infp == NULL) || (infpath == NULL)) {
	Syslog('+', "Binkp: closefile(), nothing to close");
	return 1;
    }

    rc = fclose(infp);
    infp = NULL;

    /*
     * If transfer failed, leave the file in the tmp queue, just close it.
     */
    if (! success) {
	if (rc == 0) {
	    ut.actime = intime;
	    ut.modtime = intime;
	    if ((rc = utime(infpath, &ut)))
		WriteError("$Binkp: utime on %s failed", infpath);
	}
	isfreq = FALSE;
	free(infpath);
	infpath = NULL;
	return rc;
    }

    /*
     * Remove state file.
     */
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s.state", infpath);
    if (unlink(temp))
	WriteError("$Binkp: can't unlink %s", temp);
    else
	Syslog('b', "Binkp: unlinked %s", temp);

    /*
     * Move file from extra tmp to normal tempinbound.
     */
    snprintf(temp, PATH_MAX, "%s/%s", tempinbound, basename(infpath));

    if (rc == 0) {
	snprintf(temp, PATH_MAX, "%s/%s", tempinbound, basename(infpath));
	rc = file_mv(infpath, temp);
	if (rc) {
	    WriteError("$Binkp: file_mv %s to %s rc=%d", infpath, temp, rc);
	} else {
	    Syslog('b', "Binkp: moved to %s", temp);
	}
    }

    if (rc == 0) {
	ut.actime = intime;
	ut.modtime = intime;
	if ((rc = utime(temp, &ut)))
	    WriteError("$Binkp: utime on %s failed", temp);
    }

    if (isfreq) {
	if (rc != 0) {
	    Syslog('+', "Binkp: removing unsuccessfuly received wazoo freq");
	    unlink(freqname);
	    free(freqname);
	    freqname = NULL;
	}
	isfreq = FALSE;
    }

    free(temp); 
    free(infpath);
    infpath = NULL;
    return rc;
}


