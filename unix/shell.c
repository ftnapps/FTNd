/*****************************************************************************
 *
 * $Id: shell.c,v 1.2 2003/08/15 20:05:36 mbroek Exp $
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
#include <errno.h>
#include "mblogin.h"
#include "basename.h"
#include "shell.h"


extern char **newenvp;
extern size_t newenvc;

/*
 * shell - execute the named program
 *
 *	shell begins by trying to figure out what argv[0] is going to
 *	be for the named process.  The user may pass in that argument,
 *	or it will be the last pathname component of the file with a
 *	'-' prepended.  The first attempt is to just execute the named
 *	file.  If the errno comes back "ENOEXEC", the file is assumed
 *	at first glance to be a shell script.  The first two characters
 *	must be "#!", in which case "/bin/sh" is executed to process
 *	the file.  If all that fails, give up in disgust ...
 */

void shell(const char *file, const char *arg)
{
	char arg0[1024];
	int err;

	if (file == (char *) 0)
		exit (1);

	/*
	 * The argv[0]'th entry is usually the path name, but
	 * for various reasons the invoker may want to override
	 * that.  So, we determine the 0'th entry only if they
	 * don't want to tell us what it is themselves.
	 */

	if (arg == (char *) 0) {
		snprintf(arg0, sizeof arg0, "-%s", Basename((char *) file));
		arg = arg0;
	}
#ifdef DEBUG
	printf (_("Executing shell %s\n"), file);
#endif

	/*
	 * First we try the direct approach.  The system should be
	 * able to figure out what we are up to without too much
	 * grief.
	 */

	execle (file, arg, (char *) 0, newenvp);
	err = errno;

	/* Linux handles #! in the kernel, and bash doesn't make
	   sense of "#!" so it wouldn't work anyway...  --marekm */
#ifndef __linux__
	/*
	 * It is perfectly OK to have a shell script for a login
	 * shell, and this code attempts to support that.  It
	 * relies on the standard shell being able to make sense
	 * of the "#!" magic number.
	 */

	if (err == ENOEXEC) {
		FILE	*fp;

		if ((fp = fopen (file, "r"))) {
			if (getc (fp) == '#' && getc (fp) == '!') {
				fclose (fp);
				execle ("/bin/sh", "sh",
					file, (char *) 0, newenvp);
				err = errno;
			} else {
				fclose (fp);
			}
		}
	}
#endif

	/*
	 * Obviously something is really wrong - I can't figure out
	 * how to execute this stupid shell, so I might as well give
	 * up in disgust ...
	 */

	snprintf(arg0, sizeof arg0, "Cannot execute %s", file);
	errno = err;
	perror(arg0);
	exit(1);
}


