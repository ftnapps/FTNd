/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyrioght ...: Julianne Frances Haugh and others.
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
#include "mblogin.h"
#include "getdef.h"
#include "env.h"
#include "ttytype.h"

// extern char *getenv();

/*
 * ttytype - set ttytype from port to terminal type mapping database
 */

void ttytype(const char *line)
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	*typefile;
	char	*cp;
	char	type[BUFSIZ];
	char	port[BUFSIZ];

	if (getenv ("TERM"))
		return;
	if ((typefile=getdef_str("TTYTYPE_FILE")) == NULL )
		return;
	if (access(typefile, F_OK))
		return;

	if (! (fp = fopen (typefile, "r"))) {
		perror (typefile);
		return;
	}
	while (fgets(buf, sizeof buf, fp)) {
		if (buf[0] == '#')
			continue;

		if ((cp = strchr (buf, '\n')))
			*cp = '\0';

#if defined(SUN) || defined(BSD) || defined(SUN4)
		if ((sscanf (buf, "%s \"%*[^\"]\" %s", port, type) == 2 ||
				sscanf (buf, "%s %*s %s", port, type) == 2) &&
				strcmp (line, port) == 0)
			break;
#else	/* USG */
		if (sscanf (buf, "%s %s", type, port) == 2 &&
				strcmp (line, port) == 0)
			break;
#endif
	}
	if (! feof (fp) && ! ferror (fp))
		addenv("TERM", type);

	fclose (fp);
}


