/*****************************************************************************
 *
 * Purpose ...............: File locker
 *
 *****************************************************************************
 * Copyright (C) 1997-2009
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


int f_lock(char *fn)
{
    int		    lfd=-1;
    struct flock    fl;
    struct stat	    st;

    if (fn) {
	if ((lfd = open(fn, O_RDWR | O_CREAT, 0644)) < 0) {
	    WriteError("$Error opening file %s", fn);
	    return -1;
	}

	fl.l_type=F_WRLCK;
	fl.l_whence=0;
	fl.l_start=0L;
	fl.l_len=0L;
	fl.l_pid=getpid();

	if (fcntl(lfd,F_SETLK,&fl) != 0) {
	    if (errno != EAGAIN)
		Syslog('+', "Error locking file %s",fn);
	    close(lfd);
	    return -1;
	}

	if (stat(fn,&st) != 0) {
	    WriteError("$Error accessing file %s",fn);
	    close(lfd);
	    return -1;
	}
    }
    return lfd;
}



void funlock(int fd)
{
    close(fd);
    return;
}



