/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Outbound Manager - show node info
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "nlinfo.h"



int nlinfo(faddr *addr)
{
	node	*nlent;
	int	i, t;
	char	flagbuf[256];

	if (addr == NULL)
		return 0;

	Syslog('s', "Search nodelist info for %s", ascfnode(addr, 0x1f));
	nlent = getnlent(addr);

	if (nlent->pflag != NL_DUMMY) {
		colour(3, 0);
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
		for (i = 0; fkey[i].flag != 0; i++)
			if ((nlent->mflags & fkey[i].flag) == fkey[i].flag)
				sprintf(flagbuf + strlen(flagbuf), "%s,", fkey[i].key);
		for (i = 0; dkey[i].flag != 0; i++)
			if ((nlent->dflags & dkey[i].flag) == dkey[i].flag)
				sprintf(flagbuf + strlen(flagbuf), "%s,", dkey[i].key);
		for (i = 0; ikey[i].flag != 0; i++)
			if ((nlent->iflags & ikey[i].flag) == ikey[i].flag)
				sprintf(flagbuf + strlen(flagbuf), "%s,", ikey[i].key);
		for (i = 0; okey[i].flag != 0; i++)
			if ((nlent->oflags & okey[i].flag) == okey[i].flag)
				sprintf(flagbuf + strlen(flagbuf), "%s,", okey[i].key);

		switch (nlent->xflags) {
			case RQ_XA: sprintf(flagbuf + strlen(flagbuf), "XA"); break;
			case RQ_XB: sprintf(flagbuf + strlen(flagbuf), "XB"); break;
			case RQ_XC: sprintf(flagbuf + strlen(flagbuf), "XC"); break;
			case RQ_XP: sprintf(flagbuf + strlen(flagbuf), "XP"); break;
			case RQ_XR: sprintf(flagbuf + strlen(flagbuf), "XR"); break;
			case RQ_XW: sprintf(flagbuf + strlen(flagbuf), "XW"); break;
			case RQ_XX: sprintf(flagbuf + strlen(flagbuf), "XX"); break;
		}

		printf("Flags       : %s\n", flagbuf);

		/*
		 * Show User flags
		 */
		flagbuf[0] = 0;
		for (i = 0; nlent->uflags[i]; i++) {
			sprintf(flagbuf + strlen(flagbuf), "%s,", nlent->uflags[i]);
		}
		if (strlen(flagbuf)) {
			flagbuf[strlen(flagbuf) - 1] = 0;
			printf("U-Flags     : %s\n", flagbuf);
		}

		/*
		 * Show P flags
		 */
		printf("P Flag      :");
		if (nlent->pflag & 0x01)
			printf(" Down");
		if (nlent->pflag & 0x02)
			printf(" Hold");
		if (nlent->pflag & 0x04)
			printf(" Pvt");
		if (nlent->pflag & 0x10)
			printf(" ISDN");
		if (nlent->pflag & 0x20)
			printf(" TCP/IP");
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
	}

	if (nlent->addr.domain)
		free(nlent->addr.domain);

	return 0;
}



