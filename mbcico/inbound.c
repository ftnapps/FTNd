/*****************************************************************************
 *
 * $Id$
 * File ..................: mbcico/inbound.c
 * Purpose ...............: Fidonet mailer, inbound functions 
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "inbound.h"


extern char	*inbound;
extern char	*tempinbound;
extern int	gotfiles;


/*
 * Open the inbound directory, set the temp inbound for the node
 * so that this is true multiline safe. All files received from
 * the node during the session are stored here.
 */





int inbound_open(faddr *addr, int protected)
{
    char    *temp;
    
    if (inbound)
	free(inbound);
    inbound = NULL;
    if (protected)
	inbound = xstrcpy(CFG.pinbound);
    else
	inbound = xstrcpy(CFG.inbound);

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/tmp.%d.%d.%d.%d", inbound, addr->zone, addr->net, addr->node, addr->point);
    tempinbound = xstrcpy(temp);
    sprintf(temp, "%s/foobar", tempinbound);
    mkdirs(temp, 0700);
    free(temp);

    Syslog('+', "Inbound set to \"%s\"", tempinbound);
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
    int		    rc;

    Syslog('+', "Closing temp inbound after a %s session", success?"good":"failed");
    if (! success) {
	if (tempinbound)
	    free(tempinbound);
	return 0;
    }

    if (tempinbound == NULL) {
	Syslog('!', "Temp inbound was not open");
	return 0;
    }

    if ((dp = opendir(tempinbound)) == NULL) {
	WriteError("$Can't open %s", tempinbound);
	return 1;
    }
 
    source = calloc(PATH_MAX, sizeof(char));
    dest   = calloc(PATH_MAX, sizeof(char));

    while ((de = readdir(dp))) {
	Syslog('s', "inbound_close() checking \"%s\"", MBSE_SS(de->d_name));
	sprintf(source, "%s/%s", tempinbound, de->d_name);
	sprintf(dest, "%s/%s", inbound, de->d_name);
	if ((lstat(source, &stb) == 0) && (S_ISREG(stb.st_mode))) {
	    Syslog('s', "Regular file");
	    if (file_exist(dest, F_OK) == 0) {
		Syslog('!', "Cannot move %s to %s, file exists", de->d_name, inbound);
	    } else {
		if ((rc = file_mv(source, dest))) {
		    WriteError("Can't move %s to %s: %s", source, dest, strerror(rc));
		} else {
		    Syslog('s', "Moved %s to %s", de->d_name, inbound);
		    gotfiles = TRUE;
		}
	    }
	} else {
	    Syslog('s', "Not a regular file");
	}
    }

    closedir(dp);
    free(source);
    free(dest);

    /*
     * Try to clean the temp inbound, if it fails log this, maybe the
     * next time it will work.
     */
    if ((rc = rmdir(tempinbound))) {
	WriteError("Can't remove %s: %s", tempinbound, strerror(rc));
    }
    free(tempinbound);
    tempinbound = NULL;

    return 0;
}



/*
 * Get the free space size in the temp inbound directory.
 */
long inbound_space(void)
{
    struct statfs   sfs;

    if (statfs(tempinbound, &sfs) != 0) {
	Syslog('!', "Cannot statfs \"%s\", assume enough space", tempinbound);
	return -1L;
    } else
	return (sfs.f_bsize * sfs.f_bfree);
}


