/*****************************************************************************
 *
 * $Id: pktname.c,v 1.16 2007/02/03 12:18:42 mbse Exp $
 * Purpose ...............: BinkleyTerm outbound naming
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "users.h"
#include "mbsedb.h"


#ifndef PATH_MAX
#define PATH_MAX 512
#endif

#define ptyp "ut"
#define ftyp "lo"
#define ttyp "pk"
#define rtyp "req"
#define styp "spl"
#define btyp "bsy"
#define qtyp "sts"
#define ltyp "pol"



char *prepbuf(faddr *);
char *prepbuf(faddr *addr)
{
    static char	buf[PATH_MAX];
    char	*p, *domain=NULL, zpref[8];
    int		i;

    snprintf(buf, PATH_MAX -1, "%s", CFG.outbound);

    if (CFG.addr4d) {
	Syslog('o', "Use 4d addressing, zone is %d", addr->zone);

	if ((addr->zone == 0) || (addr->zone == CFG.aka[0].zone))
	    zpref[0] = '\0';
	else
	    snprintf(zpref, 8, ".%03x", addr->zone);
    } else {
	/*
	 * If we got a 5d address we use the given domain, if
	 * we got a 4d address, we look for a matching domain name.
	 */
	if (addr && addr->domain && strlen(addr->domain)) {
	    domain = xstrcpy(addr->domain);
	} else {
	    domain = xstrcpy(GetFidoDomain(addr->zone));
	}

	/*
	 * If we got a 2d address, add the default zone.
	 */
	if (addr->zone == 0 ) {
	    addr->zone = CFG.aka[0].zone;
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
	snprintf(p, PATH_MAX -1, "%s/%04x%04x.pnt/%08x.", zpref,addr->net,addr->node,addr->point);
    else
	snprintf(p, PATH_MAX -1, "%s/%04x%04x.",zpref,addr->net,addr->node);

    if (domain)
	free(domain);
    return buf;
}



char *pktname(faddr *addr, char flavor)
{
    static char	*p, *q;

    p = prepbuf(addr);
    if (flavor == 'f') 
	flavor = 'o';
    if (flavor == 'i')
	flavor = 'd';

    q = p + strlen(p);
    snprintf(q, PATH_MAX -1, "%c%s", flavor, ptyp);
    return p;
}



char *floname(faddr *addr, char flavor)
{
    static char	*p, *q;

    p = prepbuf(addr);
    if (flavor == 'o') 
	flavor = 'f';
    if (flavor == 'i')
	flavor = 'd';

    q = p + strlen(p);
    snprintf(q, PATH_MAX -1, "%c%s", flavor, ftyp);
    return p;
}



char *reqname(faddr *addr)
{
    static char *p, *q;

    p = prepbuf(addr);
    q = p + strlen(p);    
    snprintf(q, PATH_MAX -1, "%s", rtyp);
    return p;
}



char *splname(faddr *addr)
{
    static char *p, *q;

    p = prepbuf(addr);
    q = p + strlen(p);
    snprintf(q, PATH_MAX -1, "%s", styp);
    return p;
}



char *bsyname(faddr *addr)
{
    static char	*p, *q;

    p = prepbuf(addr);
    q = p + strlen(p);
    snprintf(q, PATH_MAX -1, "%s", btyp);
    return p;
}



char *stsname(faddr *addr)
{
    static char *p, *q;

    p = prepbuf(addr);
    q = p + strlen(p);
    snprintf(q, PATH_MAX -1, "%s", qtyp);
    return p;
}



char *polname(faddr *addr)
{
    static char	*p, *q;

    p = prepbuf(addr);
    q = p + strlen(p);
    snprintf(q, PATH_MAX -1, "%s", ltyp);
    return p;
}



static char *dow[] = {(char *)"su", (char *)"mo", (char *)"tu", (char *)"we", 
		      (char *)"th", (char *)"fr", (char *)"sa"};

char *dayname(void)
{
    time_t	tt;
    struct	tm *ptm;
    static char	buf[3];
    
    tt = time(NULL);
    ptm = localtime(&tt);
    snprintf(buf, 3, "%s", dow[ptm->tm_wday]);

    return buf;	
}



char *arcname(faddr *addr, unsigned short Zone, int ARCmailCompat)
{
    static char	*buf;
	char	*p;
	char	*ext;
	time_t	tt;
	struct	tm *ptm;
	faddr	*bestaka;

	tt = time(NULL);
	ptm = localtime(&tt);
	ext = dow[ptm->tm_wday];

	bestaka = bestaka_s(addr);

	buf = prepbuf(addr);
	p = strrchr(buf, '/');

	if (!ARCmailCompat && (Zone != addr->zone)) {
		/*
		 * Generate ARCfile name from the CRC of the ASCII string
		 * of the node address.
		 */
		snprintf(p, PATH_MAX -1, "/%08x.%s0", StringCRC32(ascfnode(addr, 0x1f)), ext);
	} else {
		if (addr->point) {
			snprintf(p, PATH_MAX -1, "/%04x%04x.%s0",
				((bestaka->net) - (addr->net)) & 0xffff,
				((bestaka->node) - (addr->node) + (addr->point)) & 0xffff,
				ext);
		} else if (bestaka->point) {
			/*
			 * Inserted the next code for if we are a point,
			 * I hope this is ARCmail 0.60 compliant. 21-May-1999
			 */
			snprintf(p, PATH_MAX -1, "/%04x%04x.%s0", ((bestaka->net) - (addr->net)) & 0xffff,
				((bestaka->node) - (addr->node) - (bestaka->point)) & 0xffff, ext);
		} else {
			snprintf(p, PATH_MAX -1, "/%04x%04x.%s0", ((bestaka->net) - (addr->net)) & 0xffff,
				((bestaka->node) - (addr->node)) &0xffff, ext);
		}
	}

	tidy_faddr(bestaka);
	return buf;
}


