/*****************************************************************************
 *
 * $Id: pwio.c,v 1.5 2004/12/28 15:30:53 mbse Exp $
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)

#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "sgetpwent.h"
#include "commonio.h"
#ifndef	HAVE_PUTPWENT
#include "putpwent.h"
#endif
#include "pwio.h"





struct passwd *__pw_dup(const struct passwd *pwent)
{
	struct passwd *pw;

	if (!(pw = (struct passwd *) malloc(sizeof *pw)))
		return NULL;
	*pw = *pwent;
	if (!(pw->pw_name = strdup(pwent->pw_name)))
		return NULL;
	if (!(pw->pw_passwd = strdup(pwent->pw_passwd)))
		return NULL;
#ifdef ATT_AGE
	if (!(pw->pw_age = strdup(pwent->pw_age)))
		return NULL;
#endif
#ifdef ATT_COMMENT
	if (!(pw->pw_comment = strdup(pwent->pw_comment)))
		return NULL;
#endif
	if (!(pw->pw_gecos = strdup(pwent->pw_gecos)))
		return NULL;
	if (!(pw->pw_dir = strdup(pwent->pw_dir)))
		return NULL;
	if (!(pw->pw_shell = strdup(pwent->pw_shell)))
		return NULL;
	return pw;
}



static void *passwd_dup(const void *ent)
{
	const struct passwd *pw = ent;
	return __pw_dup(pw);
}



static void passwd_free(void *ent)
{
	struct passwd *pw = ent;

	free(pw->pw_name);
	free(pw->pw_passwd);
#ifdef ATT_AGE
        free(pw->pw_age);
#endif
#ifdef ATT_COMMENT
        free(pw->pw_comment);
#endif
	free(pw->pw_gecos);
	free(pw->pw_dir);
	free(pw->pw_shell);
	free(pw);
}



static const char *passwd_getname(const void *ent)
{
	const struct passwd *pw = ent;
	return pw->pw_name;
}



static void *passwd_parse(const char *line)
{
	return (void *) sgetpwent(line);
}



static int passwd_put(const void *ent, FILE *file)
{
	const struct passwd *pw = ent;
	return (putpwent(pw, file) == -1) ? -1 : 0;
}



static struct commonio_ops passwd_ops = {
	passwd_dup,
	passwd_free,
	passwd_getname,
	passwd_parse,
	passwd_put,
	fgets,
	fputs
};



static struct commonio_db passwd_db = {
        PASSWD_FILE,    /* filename */
        &passwd_ops,    /* ops */
        NULL,           /* fp */
        NULL,           /* head */
        NULL,           /* tail */
        NULL,           /* cursor */
        0,              /* changed */
        0,              /* isopen */
        0,              /* locked */
        0,              /* readonly */
        1               /* use_lckpwdf */
};



int pw_name(const char *filename)
{
	return commonio_setname(&passwd_db, filename);
}



#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
int pw_lock(void) 
{
	return commonio_lock(&passwd_db);
}
#endif



int pw_open(int mode)
{
	return commonio_open(&passwd_db, mode);
}



const struct passwd *pw_locate(const char *name)
{
	return commonio_locate(&passwd_db, name);
}



int pw_update(const struct passwd *pw)
{
	return commonio_update(&passwd_db, (const void *) pw);
}



int pw_remove(const char *name)
{
	return commonio_remove(&passwd_db, name);
}



int pw_rewind(void)
{
	return commonio_rewind(&passwd_db);
}



const struct passwd *pw_next(void)
{
	return commonio_next(&passwd_db);
}



int pw_close(void)
{
	return commonio_close(&passwd_db);
}



int pw_unlock(void)
{
	return commonio_unlock(&passwd_db);
}



struct commonio_entry *__pw_get_head(void)
{
	return passwd_db.head;
}



void __pw_del_entry(const struct commonio_entry *ent)
{
	commonio_del_entry(&passwd_db, ent);
}

#endif

