/*****************************************************************************
 *
 * File ..................: mbuseradd/myname.c
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Last modification date : 25-Jul-2000
 * Original Source .......: Shadow Password Suite
 * Original Copyrioght ...: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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

/*
 * myname.c - determine the current username and get the passwd entry
 *
 * Copyright (C) 1996 Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>
 *
 * This code may be freely used, modified and distributed for any purpose.
 * There is no warranty, if it breaks you have to keep both pieces, etc.
 * If you improve it, please send me your changes.  Thanks!
 */

#include "../config.h"
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>


struct passwd *get_my_pwent(void)
{
	struct passwd *pw;
	const char *cp = getlogin();
	uid_t ruid = getuid();

	/*
	 * Try getlogin() first - if it fails or returns a non-existent
	 * username, or a username which doesn't match the real UID, fall
	 * back to getpwuid(getuid()).  This should work reasonably with
	 * usernames longer than the utmp limit (8 characters), as well as
	 * shared UIDs - but not both at the same time...
	 *
	 * XXX - when running from su, will return the current user (not
	 * the original user, like getlogin() does).  Does this matter?
	 */
	if (cp && *cp && (pw = getpwnam(cp)) && pw->pw_uid == ruid)
		return pw;

	return getpwuid(ruid);
}


