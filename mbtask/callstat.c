/*****************************************************************************
 *
 * $Id: callstat.c,v 1.8 2006/01/30 22:27:03 mbse Exp $
 * Purpose ...............: Read mailer last call status
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "taskutil.h"
#include "callstat.h"


extern struct sysconfig        CFG;


void stsname_r(faddr *, char *);
void stsname_r(faddr *addr, char *buf)
{
    char	*p, *domain=NULL, zpref[8];
    int		i;

    snprintf(buf, PATH_MAX, "%s", CFG.outbound);

    if (CFG.addr4d) {
	if ((addr->zone == 0) || (addr->zone == CFG.aka[0].zone))
	    zpref[0] = '\0';
	else
	    snprintf(zpref, 8, ".%03x", addr->zone);
    } else {
	/*
	 * If we got a 5d address we use the given domain, if
	 * we got a 4d address, we look for a matching domain name.
	 */
	if (addr->domain)
	    domain = xstrcpy(addr->domain);
	else
	    for (i = 0; i < 40; i++)
		if (CFG.aka[i].zone == addr->zone) {
		    domain = xstrcpy(CFG.aka[i].domain);
		    break;
		}

	if ((domain != NULL) && (strlen(CFG.aka[0].domain) != 0) && (strcasecmp(domain,CFG.aka[0].domain) != 0)) {
	    if ((p = strrchr(buf,'/'))) 
		p++;
	    else 
		p = buf;
	    strcpy(p, domain);
	    for (; *p; p++) 
		*p = tolower(*p);
	    for (i = 0; i < 40; i++)
		if ((strlen(CFG.aka[i].domain)) && (strcasecmp(CFG.aka[i].domain, domain) == 0))
		    break;

	    /*
	     * The default zone must be the first one in the
	     * setup, other zones get the hexadecimal zone
	     * number appended.
	     */
	    if (CFG.aka[i].zone == addr->zone)
		zpref[0] = '\0';
	    else
		snprintf(zpref, 8, ".%03x", addr->zone);
	} else {
	    /*
	     * this is our primary domain
	     */
	    if ((addr->zone == 0) || (addr->zone == CFG.aka[0].zone))
		zpref[0]='\0';
	    else 
		snprintf(zpref, 8, ".%03x",addr->zone);
	}
    }

    p = buf + strlen(buf);

    if (addr->point)
	snprintf(p, PATH_MAX - strlen(buf), "%s/%04x%04x.pnt/%08x.sts", zpref,addr->net,addr->node,addr->point);
    else
	snprintf(p, PATH_MAX - strlen(buf), "%s/%04x%04x.sts",zpref,addr->net,addr->node);

    if (domain)
	free(domain);
    return;
}



void getstatus_r(faddr *addr, callstat *cst)
{
    FILE    *fp;
    char    *temp;

    cst->trytime = 0;
    cst->tryno   = 0;
    cst->trystat = 0;
    temp = calloc(PATH_MAX, sizeof(char));
    stsname_r(addr, temp);

    if ((fp = fopen(temp, "r"))) {
	fread(cst, sizeof(callstat), 1, fp);
	fclose(fp);
    }
    free(temp);

    return;
}


