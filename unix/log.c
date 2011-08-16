/*****************************************************************************
 *
 * $Id: log.c,v 1.2 2003/08/15 20:05:36 mbroek Exp $
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
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include "mblogin.h"
#include <utmp.h>
#include "log.h"


/* 
 * dolastlog - create lastlog entry
 *
 *	A "last login" entry is created for the user being logged in.  The
 *	UID is extracted from the global (struct passwd) entry and the
 *	TTY information is gotten from the (struct utmp).
 */

void dolastlog(struct lastlog *ll, const struct passwd *pw, const char *line, const char *host)
{
	int	fd;
	off_t	offset;
	struct	lastlog	newlog;

	/*
	 * If the file does not exist, don't create it.
	 */

	if ((fd = open(LASTLOG_FILE, O_RDWR)) == -1)
		return;

	/*
	 * The file is indexed by UID number.  Seek to the record
	 * for this UID.  Negative UID's will create problems, but ...
	 */

	offset = (unsigned long) pw->pw_uid * sizeof newlog;

	if (lseek(fd, offset, SEEK_SET) != offset) {
		close(fd);
		return;
	}

	/*
	 * Read the old entry so we can tell the user when they last
	 * logged in.  Then construct the new entry and write it out
	 * the way we read the old one in.
	 */

	if (read(fd, (char *) &newlog, sizeof newlog) != sizeof newlog)
		memzero(&newlog, sizeof newlog);
	if (ll)
		*ll = newlog;

	time(&newlog.ll_time);
	strncpy(newlog.ll_line, line, sizeof newlog.ll_line);
#ifdef HAVE_LL_HOST
	strncpy(newlog.ll_host, host, sizeof newlog.ll_host);
#endif
	if (lseek(fd, offset, SEEK_SET) == offset)
		write(fd, (char *) &newlog, sizeof newlog);
	close(fd);
}

