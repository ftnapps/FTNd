/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Give status of all filesystems
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10		Internet:	mbse@user.sourceforge.net
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "../lib/structs.h"
#include "taskdisk.h"
#include "taskutil.h"



/*
 * This function returns the information of all mounted filesystems,
 * but no more then 10 filesystems.
 */
char *get_diskstat()
{
    static char	    buf[SS_BUFSIZE];
    char	    *tmp = NULL;
    char	    tt[80];
    struct statfs   sfs;
    unsigned long   temp;
#if defined(__linux__)
    char            *mtab, *dev, *fs, *type;
    FILE            *fp;
    int             i = 0;
#elif defined(__FreeBSD__) || (__NetBSD__)
    struct statfs   *mntbuf;
    long            mntsize;
    int		    i, j = 0;
#endif

    buf[0] = '\0';

#if defined (__linux__)

    if ((fp = fopen((char *)"/etc/mtab", "r")) == NULL) {
	sprintf(buf, "100:0;");
	return buf;
    }

    mtab = calloc(PATH_MAX, sizeof(char));
    while (fgets(mtab, PATH_MAX - 1, fp)) {
	dev  = strtok(mtab, " \t");
	fs   = strtok(NULL, " \t");
	type = strtok(NULL, " \t");
	if (strncmp((char *)"/dev/", dev, 5) == 0) {
	    if (statfs(fs, &sfs) == 0) {
		i++;
		if (tmp == NULL)
		    tmp = xstrcpy((char *)",");
		else
		    tmp = xstrcat(tmp, (char *)",");
		tt[0] = '\0';
		temp = (unsigned long)(sfs.f_bsize / 512L);
		sprintf(tt, "%lu %lu %s %s", 
			(unsigned long)(sfs.f_blocks * temp) / 2048L,
			(unsigned long)(sfs.f_bavail * temp) / 2048L,
			fs, type);
		tmp = xstrcat(tmp, tt);
	    }
	    if (i == 10) /* No more then 10 filesystems */
		break;
	}
    }
    fclose(fp);
    free(mtab);

    if (strlen(tmp) > (SS_BUFSIZE - 8))
	sprintf(buf, "100:0;");
    else
	sprintf(buf, "100:%d%s;", i, tmp);

#elif defined(__FreeBSD__) || (__NetBSD__)

    if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0) {
	sprintf(buf, "100:0;");
	return buf;
    }

    for (i = 0; i < mntsize; i++) {
	if ((strncmp(mntbuf[i].f_fstypename, (char *)"kernfs", 6)) && 
	    (strncmp(mntbuf[i].f_fstypename, (char *)"procfs", 6)) &&
	    (statfs(mntbuf[i].f_mntonname, &sfs) == 0)) {
	    if (tmp == NULL)
		tmp = xstrcpy((char *)",");
	    else
		tmp = xstrcat(tmp, (char *)",");
	    tt[0] = '\0';
	    temp = (unsigned long)(sfs.f_bsize / 512L);
	    sprintf(tt, "%lu %lu %s %s",
		    (unsigned long)(sfs.f_blocks * temp) / 2048L,
		    (unsigned long)(sfs.f_bavail * temp) / 2048L,
		    mntbuf[i].f_mntonname, mntbuf[i].f_fstypename);
	    tmp = xstrcat(tmp, tt);
	    j++;
	    if (j == 10) /* No more then 10 filesystems */
		break;
	}
    }
    if (strlen(tmp) > (SS_BUFSIZE - 8))
	sprintf(buf, "100:0;");
    else
	sprintf(buf, "100:%d%s;", j, tmp);
#else
    /*
     * Not supported, so return nothing usefull.
     */
    sprintf(buf, "100:0;");
#endif

    if (tmp != NULL)
	free(tmp);

    return buf;
}



