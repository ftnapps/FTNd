/*****************************************************************************
 *
 * $Id: limits.c,v 1.2 2003/08/15 20:05:36 mbroek Exp $
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

/*
 * Separated from setup.c.  --marekm
 * Resource limits thanks to Cristian Gafton.
 */

#include "../config.h"
#include "mblogin.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include "limits.h"



void setup_limits(const struct passwd *info)
{
	const struct group *grp;
	mode_t oldmask;

/*
 *	if not root, and uid == gid, and username is the same as primary
 *	group name, set umask group bits to be the same as owner bits
 *	(examples: 022 -> 002, 077 -> 007).
 */
	if (info->pw_uid != 0 && info->pw_uid == info->pw_gid) {
		grp = getgrgid(info->pw_gid);
		if (grp && (strcmp(info->pw_name, grp->gr_name) == 0)) {
			oldmask = umask(0777);
			umask((oldmask & ~070) | ((oldmask >> 3) & 070));
		}
	}
}

