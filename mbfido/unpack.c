/*****************************************************************************
 *
 * $Id: unpack.c,v 1.10 2008/11/26 22:12:28 mbse Exp $
 * Purpose ...............: Unpacker
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "flock.h"
#include "unpack.h"

#define UNPACK_FACTOR 300
#define TOSS_FACTOR 120


extern int  do_quiet;


int checkspace(char *dir, char *fn, int factor)
{
	struct stat	st;
#ifdef __NetBSD__
        struct statvfs   sfs;
 
        if ((stat(fn,&st) != 0) || (statvfs(dir,&sfs) != 0)) {
#else
	struct statfs	sfs;

	if ((stat(fn,&st) != 0) || (statfs(dir,&sfs) != 0)) {
#endif
		WriteError("Cannot stat \"%s\" or statfs \"%s\", assume enough space", fn, dir);
		return 1;
	}

	if ((((st.st_size / sfs.f_bsize +1) * factor) / 100L) > sfs.f_bfree) {
		Syslog('!', "Only %lu %lu-byte blocks left on device where %s is located",
			sfs.f_bfree,sfs.f_bsize,dir);
		return 0;
	}
	return 1;
}



/*
 * Unpack archive
 */
int unpack(char *fn)
{
    char	newname[16];
    char	*cmd = NULL, *unarc;
    int		rc = 0, ld;

    if (!do_quiet) {
	mbse_colour(LIGHTCYAN, BLACK);
	printf("Unpacking file %s\n", fn);
    }

    if ((unarc = unpacker(fn)) == NULL) 
	return 1;

    if (!getarchiver(unarc))
	return 1;

    if (strlen(archiver.munarc) == 0)
	return -1;

    cmd = xstrcpy(archiver.munarc);

    if ((ld = f_lock(fn)) == -1) {
	free(cmd);
	return 1;
    }

    if ((rc = execute_str(cmd,fn,(char *)NULL,(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null")) == 0) {
	unlink(fn);
    } else {
	sync();
	sleep(1);
	WriteError("Warning: unpack %s failed, trying again after sync()", fn);
	if ((rc = execute_str(cmd,fn,(char *)NULL,(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null")) == 0) {
	    unlink(fn);
	} else {
	    strncpy(newname,fn,sizeof(newname)-1);
	    strcpy(newname+8,".bad");
	    rename(fn,newname);
	}
    }

    free(cmd);
    funlock(ld); 
    return rc;
}



