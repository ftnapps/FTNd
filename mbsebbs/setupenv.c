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



void addenv_path(const char *varname, const char *dirname, const char *filename)
{
	char *buf;

	buf = xmalloc(strlen(dirname) + strlen(filename) + 2);
	sprintf(buf, "%s/%s", dirname, filename);
	addenv(varname, buf);
	free(buf);
}



void read_env_file(const char *filename)
{
	FILE *fp;
	char buf[1024];
	char *cp, *name, *val;

	fp = fopen(filename, "r");
	if (!fp)
		return;
	while (fgets(buf, sizeof buf, fp) == buf) {
		cp = strrchr(buf, '\n');
		if (!cp)
			break;
		*cp = '\0';

		cp = buf;
		/* ignore whitespace and comments */
		while (*cp && isspace(*cp))
			cp++;
		if (*cp == '\0' || *cp == '#')
			continue;
		/*
		 * ignore lines which don't follow the name=value format
		 * (for example, the "export NAME" shell commands)
		 */
		name = cp;
		while (*cp && !isspace(*cp) && *cp != '=')
			cp++;
		if (*cp != '=')
			continue;
		/* NUL-terminate the name */
		*cp++ = '\0';
		val = cp;
		addenv(name, val);
	}
	fclose(fp);
}




/*
 *	change to the user's home directory
 *	set the HOME, SHELL, MAIL, PATH, and LOGNAME or USER environmental
 *	variables.
 */
void setup_env(struct passwd *info)
{
	char *cp, *envf;

	/*
	 * Change the current working directory to be the home directory
	 * of the user.  It is a fatal error for this process to be unable
	 * to change to that directory.  There is no "default" home
	 * directory.
	 *
	 * We no longer do it as root - should work better on NFS-mounted
	 * home directories.  Some systems default to HOME=/, so we make
	 * this a configurable option.  --marekm
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

	cp = getdef_str( info->pw_uid == 0 ? "ENV_SUPATH" : "ENV_PATH" );
	addenv(cp ? cp : "PATH=/bin:/usr/bin", NULL);

	/*
	 * Export the user name.  For BSD derived systems, it's "USER", for
	 * all others it's "LOGNAME".  We set both of them.
	 */

	addenv("USER", info->pw_name);
	addenv("LOGNAME", info->pw_name);

	/*
	 * MAILDIR environment variable for Qmail
	 */
//	if ((cp=getdef_str("QMAIL_DIR")))
//		addenv_path("MAILDIR", info->pw_dir, cp);

	/*
	 * Create the MAIL environmental variable and export it.  login.defs
	 * knows the prefix.
	 */

//	if ((cp=getdef_str("MAIL_DIR")))
//		addenv_path("MAIL", cp, info->pw_name);
//	else if ((cp=getdef_str("MAIL_FILE")))
//		addenv_path("MAIL", info->pw_dir, cp);
//	else {
//#if defined(MAIL_SPOOL_FILE)
//		addenv_path("MAIL", info->pw_dir, MAIL_SPOOL_FILE);
//#elif defined(MAIL_SPOOL_DIR)
//		addenv_path("MAIL", MAIL_SPOOL_DIR, info->pw_name);
//#endif
//	}

	/*
	 * Read environment from optional config file.  --marekm
	 */
	if ((envf = getdef_str("ENVIRON_FILE")))
		read_env_file(envf);
}

