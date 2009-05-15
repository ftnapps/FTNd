/*****************************************************************************
 *
 * $Id: dostran.c,v 1.7 2005/08/28 09:42:09 mbse Exp $
 * Purpose ...............: DOS to Unix filename translation
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "mbselib.h"


char *Dos2Unix(char *dosname)
{
	char		buf[PATH_MAX];
	static char	buf2[PATH_MAX];
	char		*p, *q;

	memset(&buf, 0, sizeof(buf));
	memset(&buf2, 0, sizeof(buf2));
	snprintf(buf, PATH_MAX -1, "%s", dosname);
	p = buf;

	if (strlen(CFG.dospath)) {
		if (strncasecmp(p, CFG.dospath, strlen(CFG.dospath)) == 0) {
			strcpy((char *)buf2, CFG.uxpath);
			for (p+=strlen(CFG.dospath), q = buf2 + strlen(buf2); *p; p++, q++)
				*q = ((*p) == '\\')?'/':tolower(*p);
			*q = '\0';
			p = buf2;
		} else {
			if (strncasecmp(p, CFG.uxpath, strlen(CFG.uxpath)) == 0) {
				for (p+=strlen(CFG.uxpath), q = buf2 + strlen(buf2); *p; p++, q++)
					*q = ((*p) == '\\')?'/':tolower(*p);
				*q = '\0';
				p = buf2;
			}
		}
	}
	return buf2;
}



char *Unix2Dos(char *uxname)
{
	char		*q;
	static char	buf[PATH_MAX];

	memset(&buf, 0, sizeof(buf));

	if (strlen(CFG.dospath)) {
		snprintf(buf, PATH_MAX -1, "%s", CFG.dospath);

		if (*(CFG.dospath+strlen(CFG.dospath)-1) != '\\')
			buf[strlen(buf)] = '\\';

		if (*(q=uxname+strlen(CFG.uxpath)) == '/')
			q++;

		for (; *q; q++)
			buf[strlen(buf)] = (*q == '/')?'\\':*q;

	} else {
		snprintf(buf, PATH_MAX -1, "%s", uxname);
	}

	return buf;
}


