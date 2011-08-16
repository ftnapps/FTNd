/*****************************************************************************
 *
 * $Id: setugid.c,v 1.3 2003/08/15 20:05:36 mbroek Exp $
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

/*
 * Separated from setup.c.  --marekm
 */

#include "../config.h"
#include <stdio.h>
#include <syslog.h>
#include "mblogin.h"
#include <pwd.h>
#include <grp.h>
#include "getdef.h"
#include "setugid.h"


/*
 * setup_uid_gid() split in two functions for PAM support -
 * pam_setcred() needs to be called after initgroups(), but
 * before setuid().
 */

int setup_groups(const struct passwd *info)
{
	/*
	 * Set the real group ID to the primary group ID in the password
	 * file.
	 */
	if (setgid (info->pw_gid) == -1) {
		perror("setgid");
		syslog(LOG_ERR, "bad group ID `%d' for user `%s': %m\n", info->pw_gid, info->pw_name);
		closelog();
		return -1;
	}
#ifdef HAVE_INITGROUPS
	/*
	 * For systems which support multiple concurrent groups, go get
	 * the group set from the /etc/group file.
	 */
	if (initgroups(info->pw_name, info->pw_gid) == -1) {
		perror("initgroups");
		syslog(LOG_ERR, "initgroups failed for user `%s': %m\n", info->pw_name);
		closelog();
		return -1;
	}
#endif
	return 0;
}



int change_uid(const struct passwd *info)
{
	/*
	 * Set the real UID to the UID value in the password file.
	 */
#ifndef	BSD
	if (setuid(info->pw_uid))
#else
	if (setreuid(info->pw_uid, info->pw_uid))
#endif
	{
		perror("setuid");
		syslog(LOG_ERR, "bad user ID `%d' for user `%s': %m\n", (int) info->pw_uid, info->pw_name);
		closelog();
		return -1;
	}
	return 0;
}



/*
 *	setup_uid_gid() performs the following steps -
 *
 *	set the group ID to the value from the password file entry
 *	set the supplementary group IDs
 *
 *	Returns 0 on success, or -1 on failure.
 */
int setup_uid_gid(const struct passwd *info, int is_console)
{
	if (setup_groups(info) < 0)
		return -1;

	if (change_uid(info) < 0)
		return -1;

	return 0;
}
