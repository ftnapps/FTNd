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
#include <stdio.h>
#include <string.h>
#include "mblogin.h"
#include "getdef.h"
#include "tz.h"


/*
 * tz - return local timezone name
 *
 * tz() determines the name of the local timezone by reading the
 * contents of the file named by ``fname''.
 */

char *tz(const char *fname)
{
	FILE *fp = 0;
	static char tzbuf[BUFSIZ];
	const char *def_tz;

	if ((fp = fopen(fname,"r")) == NULL ||
			fgets (tzbuf, sizeof (tzbuf), fp) == NULL) {
		if (! (def_tz = getdef_str ("ENV_TZ")) || def_tz[0] == '/')
			def_tz = "TZ=CST6CDT";

		strcpy (tzbuf, def_tz);
	} else
		tzbuf[strlen(tzbuf) - 1] = '\0';

	if (fp)
		(void) fclose(fp);

	return tzbuf;
}
