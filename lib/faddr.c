/*****************************************************************************
 *
 * $Id: faddr.c,v 1.9 2005/08/28 13:34:43 mbse Exp $
 * Purpose ...............: Fidonet Address conversions. 
 *
 *****************************************************************************
 * Copyright (C) 1993-2005
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "mbselib.h"


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
    if (strlen(aka.domain)) {
	if (aka.point == 0) 
	    snprintf(result, 43, "%d:%d/%d@%s", aka.zone, aka.net, aka.node, aka.domain);
	else
	    snprintf(result, 43, "%d:%d/%d.%d@%s", aka.zone, aka.net, aka.node, aka.point, aka.domain);
    } else {
	if (aka.point == 0)
	    snprintf(result, 43, "%d:%d/%d", aka.zone, aka.net, aka.node);
	else
	    snprintf(result, 43, "%d:%d/%d.%d", aka.zone, aka.net, aka.node, aka.point);
    }
    return result;
}



/*
 * Try to create a aka structure of a string.
 */
fidoaddr str2aka(char *addr)
{
	char		a[256];
	static char	b[43];
	char		*p, *temp = NULL;
	static fidoaddr	n;

	n.zone = 0;
	n.net = 0;
	n.node = 0;
	n.point = 0;
	n.domain[0] = '\0';

	/*
	 * Safety check
	 */
	if (strlen(addr) > 42)
		return n;
	
	snprintf(b, 43, "%s~", addr);
	if ((strchr(b, ':') == NULL) || (strchr(b, '/') == NULL))
		return n;

	/* 
	 * First split the f:n/n.p and domain part
	 */
	if ((strchr(b, '@'))) {
	    temp = strtok(b, "@");
	    p = strtok(NULL, "~");
	    if (p)
		strcpy(n.domain, p);
	}

	/* 
	 * Handle f:n/n.p part
	 */
	if (temp)
	    strcpy(a, strcat(temp, "~"));
	else
	    strcpy(a, b);
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


