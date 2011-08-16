/*****************************************************************************
 *
 * $Id: sub.c,v 1.2 2003/08/15 20:05:36 mbroek Exp $
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek                FIDO:           2:280/2802
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
#include <stdio.h>
#include <sys/types.h>
#include <syslog.h>
#include "mblogin.h"
#include <pwd.h>
#include "sub.h"


#define	BAD_SUBROOT2	"invalid root `%s' for user `%s'\n"
#define	NO_SUBROOT2	"no subsystem root `%s' for user `%s'\n"

/*
 * subsystem - change to subsystem root
 *
 *	A subsystem login is indicated by the presense of a "*" as
 *	the first character of the login shell.  The given home
 *	directory will be used as the root of a new filesystem which
 *	the user is actually logged into.
 */

void subsystem(const struct passwd *pw)
{
	/*
	 * The new root directory must begin with a "/" character.
	 */

	if (pw->pw_dir[0] != '/') {
		printf("Invalid root directory \"%s\"\n", pw->pw_dir);
		syslog(LOG_WARNING, BAD_SUBROOT2, pw->pw_dir, pw->pw_name);
		closelog();
		exit (1);
	}

	/*
	 * The directory must be accessible and the current process
	 * must be able to change into it.
	 */

	if (chdir (pw->pw_dir) || chroot (pw->pw_dir)) {
		printf("Can't change root directory to \"%s\"\n", pw->pw_dir);
		syslog(LOG_WARNING, NO_SUBROOT2, pw->pw_dir, pw->pw_name);
		closelog();
		exit (1);
	}
}


