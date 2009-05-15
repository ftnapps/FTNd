/*****************************************************************************
 *
 * $Id: loginprompt.c,v 1.3 2005/08/30 17:53:35 mbse Exp $
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include <signal.h>
#include <ctype.h>
#include "mblogin.h"
#include "getdef.h"
#include "xmalloc.h"
#include "env.h"
#include "loginprompt.h"


void login_exit(int sig)
{
	exit(1);
}



/*
 * login_prompt - prompt the user for their login name
 *
 * login_prompt() displays the standard login prompt.  If ISSUE_FILE
 * is set in login.defs, this file is displayed before the prompt.
 */
void login_prompt(const char *prompt, char *name, int namesize)
{
	char	buf[1024];
#define MAX_ENV 32
	char	*envp[MAX_ENV];
	int	envc;
	char	*cp;
	int	i;
	FILE	*fp;
	RETSIGTYPE	(*sigquit)(int);
#ifdef	SIGTSTP
	RETSIGTYPE	(*sigtstp)(int);
#endif

	/*
	 * There is a small chance that a QUIT character will be part of
	 * some random noise during a prompt.  Deal with this by exiting
	 * instead of core dumping.  If SIGTSTP is defined, do the same
	 * thing for that signal.
	 */

	sigquit = signal(SIGQUIT, login_exit);
#ifdef	SIGTSTP
	sigtstp = signal(SIGTSTP, login_exit);
#endif

	/*
	 * See if the user has configured the issue file to
	 * be displayed and display it before the prompt.
	 */

	if (prompt) {
		cp = getdef_str("ISSUE_FILE");
		if (cp && (fp = fopen(cp, "r"))) {
			while ((i = getc(fp)) != EOF)
				putc(i, stdout);

			fclose(fp);
		}
		gethostname(buf, sizeof buf);
		printf(prompt, buf);
		fflush(stdout);
	}

	/* 
	 * Read the user's response.  The trailing newline will be
	 * removed.
	 */
	memzero(buf, sizeof buf);
	if (fgets(buf, sizeof buf, stdin) != buf)
		exit(1);

	cp = strchr(buf, '\n');
	if (!cp)
		exit(1);
	*cp = '\0';	/* remove \n [ must be there ] */

	/*
	 * Skip leading whitespace.  This makes "  username" work right.
	 * Then copy the rest (up to the end or the first "non-graphic"
	 * character into the username.
	 */

	for (cp = buf;*cp == ' ' || *cp == '\t';cp++)
		;

//	for (i = 0; i < namesize - 1 && isgraph(*cp); name[i++] = *cp++);
	/*
	 * Allow double names for Fidonet login style.
	 */
	for (i = 0; i < namesize - 1 && isprint(*cp); name[i++] = *cp++);
	while (isgraph(*cp))
		cp++;

	if (*cp)
		cp++;

	name[i] = '\0';

	/*
	 * This is a disaster, at best.  The user may have entered extra
	 * environmental variables at the prompt.  There are several ways
	 * to do this, and I just take the easy way out.
	 */

	if (*cp != '\0') {		/* process new variables */
		char *nvar;
		int count = 1;

		for (envc = 0; envc < MAX_ENV; envc++) {
			nvar = strtok(envc ? (char *)0 : cp, " \t,");
			if (!nvar)
				break;
			if (strchr(nvar, '=')) {
				envp[envc] = nvar;
			} else {
				envp[envc] = xmalloc(strlen(nvar) + 32);
				snprintf(envp[envc], strlen(nvar) + 32, "L%d=%s", count++, nvar);
			}
		}
		set_env(envc, envp);
	}

	/*
	 * Set the SIGQUIT handler back to its original value
	 */

	signal(SIGQUIT, sigquit);
#ifdef	SIGTSTP
	signal(SIGTSTP, sigtstp);
#endif
}
