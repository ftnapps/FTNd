/*****************************************************************************
 *
 * $Id: inbound.c,v 1.9 2007/08/26 14:02:28 mbse Exp $
 * Purpose ...............: Fidonet mailer, inbound functions 
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
#include "dirlock.h"
#include "inbound.h"


extern char	*inbound;
extern char	*tempinbound;
extern int	gotfiles;
extern int	laststat;


/*
 * Open the inbound directory, set the temp inbound for the node
 * so that this is true multiline safe. All files received from
 * the node during the session are stored here.
 * For binkp we add one extra directory to queue incoming files.
 */
int inbound_open(faddr *addr, int protected, int binkp_mode)
{
    char    *temp;
    DIR	    *dp;

    if (inbound)
	free(inbound);
    inbound = NULL;
    if (protected)
	inbound = xstrcpy(CFG.pinbound);
    else
	inbound = xstrcpy(CFG.inbound);

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX -1, "%s/tmp.%d.%d.%d.%d", inbound, addr->zone, addr->net, addr->node, addr->point);

    /*
     * Check if this directory already exist, it should not unless the previous
     * session with this node failed for some reason.
     */
    if ((dp = opendir(temp))) {
	Syslog('s', "Binkp: dir %s already exists, previous session failed", temp);
	laststat++;
	closedir(dp);
    }

    tempinbound = xstrcpy(temp);
    if (binkp_mode)
	snprintf(temp, PATH_MAX, "%s/tmp/foobar", tempinbound);
    else
	snprintf(temp, PATH_MAX, "%s/foobar", tempinbound);
    mkdirs(temp, 0700);
    free(temp);

    Syslog('s', "Inbound set to \"%s\"", tempinbound);
    return 0;
}



/*
 * Close the temp inbound, if the session is successfull, move all
 * received files to the real inbound, else leave them there for
 * the next attempt. Set a semafore if we did receive anything.
 * Clean and remove the temp inbound if it is empty.
 */
int inbound_close(int success)
{
    DIR		    *dp;
    struct dirent   *de;
    char	    *source, *dest;
    struct stat	    stb;
    int		    i, rc;

    Syslog('s', "Closing temp inbound after a %s session", success?"good":"failed");
    if (! success) {
	if (tempinbound)
	    free(tempinbound);
	return 0;
    }

    if (tempinbound == NULL) {
	Syslog('!', "Temp inbound was not open");
	return 0;
    }

    /*
     * Try to lock the inbound so we can safely move the files
     * to the inbound.
     */
    i = 30;
    while (TRUE) {
	if (lockdir(inbound))
	    break;
	i--;
	if (! i) {
	    WriteError("Can't lock %s", inbound);
	    return 1;
	}
	sleep(20);
	Nopper();
    }
    Syslog('s', "inbound_close(): %s locked", inbound);

    if ((dp = opendir(tempinbound)) == NULL) {
	WriteError("$Can't open %s", tempinbound);
	return 1;
    }
 
    source = calloc(PATH_MAX, sizeof(char));
    dest   = calloc(PATH_MAX, sizeof(char));

    while ((de = readdir(dp))) {
	snprintf(source, PATH_MAX, "%s/%s", tempinbound, de->d_name);
	snprintf(dest, PATH_MAX, "%s/%s", inbound, de->d_name);
	if ((lstat(source, &stb) == 0) && (S_ISREG(stb.st_mode))) {
	    if (file_exist(dest, F_OK) == 0) {
		Syslog('!', "Cannot move %s to %s, file exists", de->d_name, inbound);
	    } else {
		if ((rc = file_mv(source, dest))) {
		    WriteError("Can't move %s to %s: %s", source, dest, strerror(rc));
		} else {
		    Syslog('s', "inbound_close(): moved %s to %s", de->d_name, inbound);
		    gotfiles = TRUE;
		}
	    }
	}
    }

    closedir(dp);
    ulockdir(inbound);
    Syslog('s', "inbound_close(): %s unlocked", inbound);

    /*
     * Try to remove binkp tmp queue. Only log if not empty, else
     * don't log anything, this directory doesn't exist for normal
     * sessions.
     */
    snprintf(source, PATH_MAX, "%s/tmp", tempinbound);
    if ((rc = rmdir(source))) {
	if (errno == ENOTEMPTY) {
	    Syslog('+', "Keep binkp temp incoming directory, partial file");
	}
    } else {
	Syslog('s', "inbound_close(): removed %s", source);
    }
    
    /*
     * Try to clean the temp inbound, if it fails log this, maybe the
     * next time it will work.
     */
    if ((rc = rmdir(tempinbound))) {
	WriteError("$Can't remove %s", tempinbound);
    } else {
	Syslog('s', "inbound_close(): removed %s", tempinbound);
    }

    free(source);
    free(dest);
    free(tempinbound);
    tempinbound = NULL;

    return 0;
}



/*
 * Get the free space size in the temp inbound directory.
 */
int inbound_space(void)
{
#ifdef __NetBSD__
    struct statvfs   sfs;
 
    if (statvfs(tempinbound, &sfs) != 0) {
#else
    struct statfs   sfs;

    if (statfs(tempinbound, &sfs) != 0) {
#endif
	Syslog('!', "Cannot statfs \"%s\", assume enough space", tempinbound);
	return -1L;
    } else
	return (sfs.f_bsize * sfs.f_bfree);
}


