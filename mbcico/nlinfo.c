/*****************************************************************************
 *
 * $Id: nlinfo.c,v 1.15 2005/08/28 13:45:26 mbse Exp $
 * Purpose ...............: MBSE BBS Outbound Manager - show node info
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
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "nlinfo.h"



int nlinfo(faddr *addr)
{
    node	    *nlent;
    int		    i, t;
    char	    flagbuf[256];
    nodelist_modem  **tmpm;
    nodelist_flag   **tmpf;
    
    if (addr == NULL)
	return 0;

    Syslog('s', "Search nodelist info for %s", ascfnode(addr, 0x1f));
    nlent = getnlent(addr);

    if (nlent->pflag != NL_DUMMY) {
	mbse_colour(CYAN, BLACK);
	printf("System      : %s\n", nlent->name);
	printf("Sysop       : %s@%s\n", nlent->sysop, ascinode(addr, 0x3f));
	printf("Location    : %s\n", nlent->location);
	if (nlent->phone)
	    printf("Phone       : %s\n", nlent->phone);
	else
	    printf("Phone       : -Unpublished-\n");
	printf("Speed       : %d\n", nlent->speed);

	flagbuf[0] = 0;

	/*
	 * Get all normal nodelist flags
	 */
	for (tmpf = &nl_online; *tmpf; tmpf = &((*tmpf)->next))
	    if ((nlent->oflags & (*tmpf)->value) == (*tmpf)->value)
		snprintf(flagbuf + strlen(flagbuf), 256 - strlen(flagbuf), "%s,", (*tmpf)->name);
	for (tmpf = &nl_request; *tmpf; tmpf = &((*tmpf)->next))
	    if (nlent->xflags == (*tmpf)->value)
		snprintf(flagbuf + strlen(flagbuf), 256 - strlen(flagbuf), "%s,", (*tmpf)->name);
	for (tmpm = &nl_pots; *tmpm; tmpm=&((*tmpm)->next))
	    if ((nlent->mflags & (*tmpm)->mask) == (*tmpm)->mask)
		snprintf(flagbuf + strlen(flagbuf), 256 - strlen(flagbuf), "%s,", (*tmpm)->name);
	for (tmpm = &nl_isdn; *tmpm; tmpm=&((*tmpm)->next))
	    if ((nlent->dflags & (*tmpm)->mask) == (*tmpm)->mask)
		snprintf(flagbuf + strlen(flagbuf), 256 - strlen(flagbuf), "%s,", (*tmpm)->name);
	for (tmpm = &nl_tcpip; *tmpm; tmpm=&((*tmpm)->next))
	    if ((nlent->iflags & (*tmpm)->mask) == (*tmpm)->mask)
		snprintf(flagbuf + strlen(flagbuf), 256 - strlen(flagbuf), "%s,", (*tmpm)->name);
	flagbuf[strlen(flagbuf)-1] = '\0';
	printf("Flags       : %s\n", flagbuf);

	/*
	 * Show User flags
	 */
	flagbuf[0] = 0;
	for (i = 0; nlent->uflags[i]; i++) {
	    snprintf(flagbuf + strlen(flagbuf), 256 - strlen(flagbuf), "%s,", nlent->uflags[i]);
	}
	if (strlen(flagbuf)) {
	    flagbuf[strlen(flagbuf) - 1] = 0;
	    printf("U-Flags     : %s\n", flagbuf);
	}

	/*
	 * Show P flags
	 */
	printf("P Flag      :");
	if (nlent->pflag & NL_DOWN)
	    printf(" Down");
	if (nlent->pflag & NL_HOLD)
	    printf(" Hold");
	if (nlent->pflag & NL_PVT)
	    printf(" Pvt");
	printf("\n");
	if (nlent->t1) {
	    printf("System open : ");
	    t = toupper(nlent->t1);
	    printf("%02d:", t - 65);
	    if (isupper(nlent->t1))
		printf("00 - ");
	    else
		printf("30 - ");
	    t = toupper(nlent->t2);
	    printf("%02d:", t - 65);
	    if (isupper(nlent->t2))
		printf("00\n");
	    else
		printf("30\n");
	}
	printf("Uplink      : %u/%u\n", nlent->upnet, nlent->upnode);
	printf("Region      : %u\n", nlent->region);
	printf("URL         : %s\n", printable(nlent->url, 0));
    }

    if (nlent->addr.domain)
	free(nlent->addr.domain);
    if (nlent->url)
	free(nlent->url);

    return 0;
}



