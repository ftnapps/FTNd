/*****************************************************************************
 *
 * setupenv.c
 * Purpose ...............: FTNd Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2003 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

/*
 * Separated from setup.c.  --marekm
 */

#include "../config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include "mblogin.h"
#include <pwd.h>
#include "getdef.h"
#include "xmalloc.h"
#include "env.h"
#include "setupenv.h"



/*
 *	change to the user's home directory
 *	set the HOME, SHELL, MAIL, PATH, and LOGNAME or USER environmental
 *	variables.
 */
void setup_env(struct passwd *info)
{
	char	*cp;

	/*
	 * Change the current working directory to be the home directory
	 * of the user.  It is a fatal error for this process to be unable
	 * to change to that directory.  There is no "default" home
	 * directory.
	 *
	 * We no longer do it as root - should work better on NFS-mounted
	 * home directories.
	 */
	if (chdir(info->pw_dir) == -1) {
	    static char temp_pw_dir[] = "/";
	    if (!getdef_bool("DEFAULT_HOME") || chdir("/") == -1) {
		fprintf(stderr, _("Unable to cd to \"%s\"\n"), info->pw_dir);
		syslog(LOG_WARNING, "unable to cd to `%s' for user `%s'\n", info->pw_dir, info->pw_name);
		closelog();
		exit (1);
	    }
	    puts(_("No directory, logging in with HOME=/"));
	    info->pw_dir = temp_pw_dir;
	}

	/*
	 * Create the HOME environmental variable and export it.
	 */
	addenv("HOME", info->pw_dir);

	/*
	 * Create the SHELL environmental variable and export it.
	 */
	if (info->pw_shell == (char *) 0 || ! *info->pw_shell) {
		static char temp_pw_shell[] = "/bin/sh";
		info->pw_shell = temp_pw_shell;
	}

	addenv("SHELL", info->pw_shell);

	/*
	 * Create the PATH environmental variable and export it.
	 */
	cp = getdef_str("ENV_PATH");
	addenv(cp ? cp : "PATH=/bin:/usr/bin", NULL);

	/*
	 * Export the user name.  For BSD derived systems, it's "USER", for
	 * all others it's "LOGNAME".  We set both of them.
	 */

	addenv("USER", info->pw_name);
	addenv("LOGNAME", info->pw_name);
}

