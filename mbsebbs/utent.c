/*****************************************************************************
 *
 * $Id$
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"

#ifndef	HAVE_GETUTENT

#include "mblogin.h"
#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>


static	int	utmp_fd = -1;
static	struct	utmp	utmp_buf;

/*
 * setutent - open or rewind the utmp file
 */

void
setutent(void)
{
	if (utmp_fd == -1)
		if ((utmp_fd = open (_UTMP_FILE, O_RDWR)) == -1)
			utmp_fd = open (_UTMP_FILE, O_RDONLY);

	if (utmp_fd != -1)
		lseek (utmp_fd, (off_t) 0L, SEEK_SET);
}

/*
 * endutent - close the utmp file
 */

void
endutent(void)
{
	if (utmp_fd != -1)
		close (utmp_fd);

	utmp_fd = -1;
}

/*
 * getutent - get the next record from the utmp file
 */

struct utmp *
getutent(void)
{
	if (utmp_fd == -1)
		setutent ();

	if (utmp_fd == -1)
		return 0;

	if (read (utmp_fd, &utmp_buf, sizeof utmp_buf) != sizeof utmp_buf)
		return 0;

	return &utmp_buf;
}

/*
 * getutline - get the utmp entry matching ut_line
 */

struct utmp *
getutline(const struct utmp *utent)
{
	struct	utmp	save;
	struct	utmp	*new;

	save = *utent;
	while (new = getutent ())
		if (strncmp (new->ut_line, save.ut_line, sizeof new->ut_line))
			continue;
		else
			return new;

	return (struct utmp *) 0;
}
#else
extern int errno;  /* warning: ANSI C forbids an empty source file */
#endif
