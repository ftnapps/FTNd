/*****************************************************************************
 *
 * File ..................: faddr.c
 * Purpose ...............: Fidonet Address conversions. 
 * Last modification date : 18-Dec-1999
 *
 *****************************************************************************
 * Copyright (C) 1993-1999
 *   
 * Michiel Broek		FIDO:	2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MB BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "common.h"

/************************************************************************
 *
 *  String functions
 */


/*
 * Fidonet aka to string, returns Z:N/N@domain or Z:N/N.P@domain
 */
char *aka2str(fidoaddr aka)
{
	static char	result[43];

	result[0] = '\0';
	if (aka.point == 0) 
		sprintf(result, "%d:%d/%d@%s", aka.zone, aka.net, aka.node, aka.domain);
	else
		sprintf(result, "%d:%d/%d.%d@%s", aka.zone, aka.net, aka.node, aka.point, aka.domain);

	return result;
}



fidoaddr str2aka(char *addr)
{
	char		a[256];
	static char	b[43];
	char		*temp;
	fidoaddr	n;

	sprintf(b, "%s~", addr);
	n.zone = 0;
	n.net = 0;
	n.node = 0;
	n.point = 0;
	n.domain[0] = '\0';

	if ((strchr(b, ':') == NULL) || (strchr(b, '@') == NULL))
		return n;

	/* First split the f:n/n.p and domain part
	 */
	temp = strtok(b, "@");
	strcpy(n.domain, strtok(NULL, "~"));

	/* Handle f:n/n.p part
	 */
	strcpy(a, strcat(temp, "~"));
	if (strchr(a, '.') == NULL) {
		n.zone = atoi(strtok(a, ":"));
		n.net = atoi(strtok(NULL, "/"));
		n.node = atoi(strtok(NULL, "~"));
	} else {
		n.zone = atoi(strtok(a, ":"));
		n.net = atoi(strtok(NULL, "/"));
		n.node = atoi(strtok(NULL, "."));
		n.point = atoi(strtok(NULL, "~"));
	}
	return n;
}




