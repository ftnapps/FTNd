/*****************************************************************************
 *
 * $Id: getdef.c,v 1.4 2005/08/30 17:53:35 mbse Exp $
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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


#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <sys/param.h>
#include <pwd.h>
#include "getdef.h"



/*
 * A configuration item definition.
 */
struct itemdef {
	const char *name;	/* name of the item			*/
	char *value;		/* value given, or NULL if no value	*/
};



/*
 *  This list *must* be sorted by the "name" member.
 *  It doesn't hurt that there are extra entries here
 *  that are not known by the system this is running
 *  on. Missing entries here gives a nasty message to
 *  new bbs users.
 */
#define NUMDEFS	(sizeof(def_table)/sizeof(def_table[0]))
static struct itemdef def_table[] = {
	{ "ALLOW_MBSE",			NULL },
	{ "ASK_NEWUSER",		NULL },
	{ "DEFAULT_HOME",		NULL },
	{ "ENV_HZ",			NULL },
	{ "ENV_PATH" ,			NULL },
	{ "ENV_TZ",			NULL },
	{ "ERASECHAR",			NULL },
	{ "FAIL_DELAY",			NULL },
	{ "ISSUE_FILE",			NULL },
	{ "KILLCHAR",			NULL },
	{ "LASTLOG_ENAB",		NULL },
	{ "LOGIN_RETRIES",		NULL },
	{ "LOGIN_TIMEOUT",		NULL },
	{ "LOG_OK_LOGINS",		NULL },
	{ "LOG_UNKFAIL_ENAB",		NULL },
	{ "MD5_CRYPT_ENAB",		NULL },
	{ "NEWUSER_ACCOUNT",		NULL },
	{ "NOLOGINS_FILE",		NULL },
	{ "TTYGROUP",			NULL },
	{ "TTYPERM",			NULL },
	{ "ULIMIT",			NULL },
	{ "UMASK",			NULL },
	{ "USERGROUPS_ENAB",		NULL },
};


static int def_loaded = 0;		/* are defs already loaded?	*/


/* local function prototypes */
static struct itemdef *def_find (const char *);



/*
 * getdef_str - get string value from table of definitions.
 *
 * Return point to static data for specified item, or NULL if item is not
 * defined.  First time invoked, will load definitions from the file.
 */
char *getdef_str(const char *item)
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	return ((d = def_find(item)) == NULL ? (char *)NULL : d->value);
}



/*
 * getdef_bool - get boolean value from table of definitions.
 *
 * Return TRUE if specified item is defined as "yes", else FALSE.
 */
int getdef_bool(const char *item)
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	if ((d = def_find(item)) == NULL || d->value == NULL)
		return 0;

	return (strcmp(d->value, "yes") == 0);
}



/*
 * getdef_num - get numerical value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */
int getdef_num(const char *item, int dflt)
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	if ((d = def_find(item)) == NULL || d->value == NULL)
		return dflt;

	return (int) strtol(d->value, (char **)NULL, 0);
}



/*
 * getdef_long - get long integer value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */
long getdef_long(const char *item, long dflt)
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	if ((d = def_find(item)) == NULL || d->value == NULL)
		return dflt;

	return strtol(d->value, (char **)NULL, 0);
}



/*
 * def_find - locate named item in table
 *
 * Search through a sorted table of configurable items to locate the
 * specified configuration option.
 */
static struct itemdef *def_find(const char *name)
{
	int min, max, curr, n;

	/*
	 * Invariant - desired item in range [min:max].
	 */

	min = 0;
	max = NUMDEFS-1;

	/*
	 * Binary search into the table.  Relies on the items being
	 * sorted by name.
	 */

	while (min <= max) {
		curr = (min+max)/2;

		if (! (n = strcmp(def_table[curr].name, name)))
			return &def_table[curr];

		if (n < 0)
			min = curr+1;
		else
			max = curr-1;
	}

	/*
	 * Item was never found.
	 */

	fprintf(stderr, "getdef(): configuration error - unknown item '%s' (notify administrator)\r\n", name);
	syslog(LOG_CRIT, "getdef(): unknown configuration item `%s'", name);
	return (struct itemdef *) NULL;
}



/*
 * def_load - load configuration table
 *
 * Loads the user-configured options from the default configuration file
 */
void def_load(void)
{
	int		i;
	FILE		*fp;
	struct itemdef	*d;
	char		buf[BUFSIZ], def_fname[PATH_MAX], *name, *value, *s;
	struct passwd	*pw;

	/*
	 * Get MBSE BBS root directory
	 */
	if ((pw = getpwnam("mbse")) == NULL) {
		syslog(LOG_CRIT, "cannot find user `mbse' in password file");
		return;
	}
	snprintf(def_fname, PATH_MAX, "%s/etc/login.defs", pw->pw_dir);

	/*
	 * Open the configuration definitions file.
	 */
	if ((fp = fopen(def_fname, "r")) == NULL) {
		syslog(LOG_CRIT, "cannot open login definitions %s [%m]", def_fname);
		return;
	}

	/*
	 * Go through all of the lines in the file.
	 */
	while (fgets(buf, sizeof(buf), fp) != NULL) {

		/*
		 * Trim trailing whitespace.
		 */

		for (i = strlen(buf)-1 ; i >= 0 ; --i) {
			if (!isspace(buf[i]))
				break;
		}
		buf[++i] = '\0';

		/*
		 * Break the line into two fields.
		 */

		name = buf + strspn(buf, " \t");	/* first nonwhite */
		if (*name == '\0' || *name == '#')
			continue;			/* comment or empty */

		s = name + strcspn(name, " \t");	/* end of field */
		if (*s == '\0')
			continue;			/* only 1 field?? */

		*s++ = '\0';
		value = s + strspn(s, " \"\t");		/* next nonwhite */
		*(value + strcspn(value, "\"")) = '\0';

		/*
		 * Locate the slot to save the value.  If this parameter
		 * is unknown then "def_find" will print an err message.
		 */

		if ((d = def_find(name)) == NULL)
			continue;

		/*
		 * Save off the value.
		 */

		if ((d->value = strdup(value)) == NULL) {
			fprintf(stderr, "getdef: Could not allocate space for config info.\n");
			syslog(LOG_ERR, "getdef: could not allocate space for config info");
			break;
		}
	}
	(void) fclose(fp);

	/*
	 * Set the initialized flag.
	 */
	++def_loaded;
}



#ifdef CKDEFS
int main(int argc, char **argv)
{
	int i;
	char *cp;
	struct itemdef *d;

	def_load ();

	for (i = 0 ; i < NUMDEFS ; ++i) {
		if ((d = def_find(def_table[i].name)) == NULL)
			printf("error - lookup '%s' failed\n", def_table[i].name);
		else
			printf("%4d %-24s %s\n", i+1, d->name, d->value);
	}
	for (i = 1;i < argc;i++) {
		if (cp = getdef_str (argv[1]))
			printf ("%s `%s'\n", argv[1], cp);
		else
			printf ("%s not found\n", argv[1]);
	}
	exit(0);
}
#endif

