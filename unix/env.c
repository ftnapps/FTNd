/*****************************************************************************
 *
 * $Id: env.c,v 1.3 2005/08/30 17:53:35 mbse Exp $
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
#include <stdlib.h>
#include <string.h>
#include "mblogin.h"
#include "xmalloc.h"


/*
 * NEWENVP_STEP must be a power of two.  This is the number
 * of (char *) pointers to allocate at a time, to avoid using
 * realloc() too often.
 */ 
#define NEWENVP_STEP 16

size_t newenvc = 0;
char **newenvp = NULL;
extern char **environ;


static const char *forbid[] = {
	"_RLD_=",
	"BASH_ENV=",	/* GNU creeping featurism strikes again... */
	"ENV=",
	"HOME=",
	"IFS=",
	"KRB_CONF=",
	"LD_",		/* anything with the LD_ prefix */
	"LIBPATH=",
	"MAIL=",
	"NLSPATH=",
	"PATH=",
	"SHELL=",
	"SHLIB_PATH=",
	(char *) 0
};



/* these are allowed, but with no slashes inside
   (to work around security problems in GNU gettext) */
static const char *noslash[] = {
	"LANG=",
	"LANGUAGE=",
	"LC_",		/* anything with the LC_ prefix */
	(char *) 0
};



/*
 * initenv() must be called once before using addenv().
 */
void initenv(void)
{
	newenvp = (char **)xmalloc(NEWENVP_STEP * sizeof(char *));
	*newenvp = NULL;
}


void addenv(const char *string, const char *value)
{
	char *cp, *newstring;
	size_t i;
	size_t n;

	if (value) {
		newstring = xmalloc(strlen(string) + strlen(value) + 2);
		snprintf(newstring, strlen(string) + strlen(value) + 2, "%s=%s", string, value);
	} else {
		newstring = xstrdup(string);
	}

	/*
	 * Search for a '=' character within the string and if none is found
	 * just ignore the whole string.
	 */

	cp = strchr(newstring, '=');
	if (!cp) {
		free(newstring);
		return;
	}

	n = (size_t)(cp - newstring);

	for (i = 0; i < newenvc; i++) {
		if (strncmp(newstring, newenvp[i], n) == 0 &&
		    (newenvp[i][n] == '=' || newenvp[i][n] == '\0'))
			break;
	}

	if (i < newenvc) {
		free(newenvp[i]);
		newenvp[i] = newstring;
		return;
	}

	newenvp[newenvc++] = newstring;

	/*
	 * Check whether newenvc is a multiple of NEWENVP_STEP.
	 * If so we have to resize the vector.
	 * the expression (newenvc & (NEWENVP_STEP - 1)) == 0
	 * is equal to    (newenvc %  NEWENVP_STEP) == 0
	 * as long as NEWENVP_STEP is a power of 2.
	 */

	if ((newenvc & (NEWENVP_STEP - 1)) == 0) {
		char **__newenvp;
		size_t newsize;

		/*
		 * If the resize operation succeds we can
		 * happily go on, else print a message.
		 */

		newsize = (newenvc + NEWENVP_STEP) * sizeof(char *);
		__newenvp = (char **)realloc(newenvp, newsize);

		if (__newenvp) {
			/*
			 * If this is our current environment, update
			 * environ so that it doesn't point to some
			 * free memory area (realloc() could move it).
			 */
			if (environ == newenvp)
				environ = __newenvp;
			newenvp = __newenvp;
		} else {
			fprintf(stderr, "Environment overflow\n");
			free(newenvp[--newenvc]);
		}
	}

	/*
	 * The last entry of newenvp must be NULL
	 */

	newenvp[newenvc] = NULL;
}


/*
 * set_env - copy command line arguments into the environment
 */
void set_env(int argc, char * const *argv)
{
	int	noname = 1;
	char	variable[1024];
	char	*cp;

	for ( ; argc > 0; argc--, argv++) {
		if (strlen(*argv) >= sizeof variable)
			continue;	/* ignore long entries */

		if (! (cp = strchr (*argv, '='))) {
			snprintf(variable, sizeof variable, "L%d", noname++);
			addenv(variable, *argv);
		} else {
			const char **p;

			for (p = forbid; *p; p++)
				if (strncmp(*argv, *p, strlen(*p)) == 0)
					break;

			if (*p) {
				strncpy(variable, *argv, cp - *argv);
				variable[cp - *argv] = '\0';
				printf("You may not change $%s\n", variable);
				continue;
			}

			addenv(*argv, NULL);
		}
	}
}



/*
 * sanitize_env - remove some nasty environment variables
 * If you fall into a total paranoia, you should call this
 * function for any root-setuid program or anything the user
 * might change the environment with. 99% useless as almost
 * all modern Unixes will handle setuid executables properly,
 * but... I feel better with that silly precaution. -j.
 */

void sanitize_env(void)
{
	char **envp = environ;
	const char **bad;
	char **cur;
	char **move;

	for (cur = envp; *cur; cur++) {
		for (bad = forbid; *bad; bad++) {
			if (strncmp(*cur, *bad, strlen(*bad)) == 0) {
				for (move = cur; *move; move++)
					*move = *(move + 1);
				cur--;
				break;
			}
		}
	}

	for (cur = envp; *cur; cur++) {
		for (bad = noslash; *bad; bad++) {
			if (strncmp(*cur, *bad, strlen(*bad)) != 0)
				continue;
			if (!strchr(*cur, '/'))
				continue;  /* OK */
			for (move = cur; *move; move++)
				*move = *(move + 1);
			cur--;
			break;
		}
	}
}

