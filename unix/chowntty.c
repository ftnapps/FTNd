/*****************************************************************************
 *
 * $Id: chowntty.c,v 1.2 2003/08/15 20:05:36 mbroek Exp $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdio.h>
#include <grp.h>
#include "mblogin.h"
#include <pwd.h>
#include "getdef.h"
#include "chowntty.h"



/*
 * is_my_tty -- determine if "tty" is the same as TTY stdin is using
 */
int is_my_tty(const char *tty)
{
	struct	stat	by_name, by_fd;

	if (stat (tty, &by_name) || fstat (0, &by_fd))
		return 0;

	if (by_name.st_rdev != by_fd.st_rdev)
		return 0;
	else
		return 1;
}



/*
 *	chown_tty() sets the login tty to be owned by the new user ID
 *	with TTYPERM modes
 */
void chown_tty(const char *tty, const struct passwd *info)
{
	char		buf[200], full_tty[200];
	char		*group;	    /* TTY group name or number */
	struct group	*grent;
	gid_t		gid;

	/*
	 *          * See if login.defs has some value configured for the port group
	 *                   * ID.  Otherwise, use the user's primary group ID.
	 *                            */

	if (! (group = getdef_str ("TTYGROUP")))
		gid = info->pw_gid;
	else if (group[0] >= '0' && group[0] <= '9')
		gid = atoi (group);
	else if ((grent = getgrnam (group)))
		gid = grent->gr_gid;
	else
		gid = info->pw_gid;


	/*
	 * Change the permissions on the TTY to be owned by the user with
	 * the group as determined above.
	 */

	if (*tty != '/') {
		snprintf(full_tty, sizeof full_tty, "/dev/%s", tty);
		tty = full_tty;
	}

	if (! is_my_tty (tty)) {
		syslog(LOG_WARNING, "unable to determine TTY name, got %s\n", tty);
		closelog();
		exit (1);
	}
	
	if (chown(tty, info->pw_uid, gid) || chmod(tty, 0600)) {
		snprintf(buf, sizeof buf, "Unable to change tty %s", tty);
		syslog(LOG_WARNING, "unable to change tty `%s' for user `%s'\n", tty, info->pw_name);
		closelog();
		perror (buf);
		exit(1);
	}

#ifdef __linux__
	/*
	 * Please don't add code to chown /dev/vcs* to the user logging in -
	 * it's a potential security hole.  I wouldn't like the previous user
	 * to hold the file descriptor open and watch my screen.  We don't
	 * have the *BSD revoke() system call yet, and vhangup() only works
	 * for tty devices (which vcs* is not).  --marekm
	 */
#endif
}
