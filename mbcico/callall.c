/*****************************************************************************
 *
 * File ..................: mbcico/callall.c
 * Purpose ...............: Fidonet mailer
 * Last modification date : 27-Nov-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/common.h"
#include "config.h"
#include "../lib/clcomm.h"
#include "scanout.h"
#include "lutil.h"
#include "callstat.h"
#include "callall.h"


static int each(faddr*, char, int, char*);
static fa_list *alist = NULL;



fa_list *callall(void)
{
	fa_list	*tmp;
	int	rc;

	if (alist) {
		for (tmp = alist; tmp; tmp = alist) {
			alist = tmp->next;
			tidy_faddr(tmp->addr);
			free(tmp);
		}
		alist = NULL;
	}

	if ((rc = scanout(each))) {
		WriteError("Error scanning outbound, aborting");
		return NULL;
	}

	return (fa_list *)alist;
}



static int each(faddr *addr, char flavor, int isflo, char *fname)
{
	fa_list		**tmp;
	callstat	*st;

	if ((flavor == 'h') ||
	    ((isflo != OUT_PKT) && (isflo != OUT_FLO) && (isflo != OUT_POL)))
		return 0;

	/*
	 * Outside Zone Mail Hour normal flavor will be hold.
	 */
	if (!IsZMH() && (flavor == 'o'))
		return 0;

	/*
	 * During ZMH only poll and .pkt files will be sent, except
	 * IMMediate mail, that goes always.
	 */
	if (flavor != 'i') {
		if (IsZMH() && (isflo == OUT_FLO))
			return 0;
	}

	/*
	 * Don't add nodes who are undiable
	 */
	st = getstatus(addr);
	if (st->tryno >= 30)
		return 0;

	for (tmp = &alist; *tmp; tmp=&((*tmp)->next))
		if (((*tmp)->addr->zone == addr->zone) &&
		    ((*tmp)->addr->net == addr->net) &&
		    ((*tmp)->addr->node == addr->node) &&
		    ((*tmp)->addr->point == addr->point) &&
		    (((*tmp)->addr->domain == NULL) ||
		     (addr->domain == NULL) ||
		     (strcasecmp((*tmp)->addr->domain,addr->domain) == 0)))
			break;

	if (*tmp == NULL) {
		*tmp=(fa_list *)malloc(sizeof(fa_list));
		(*tmp)->next=NULL;
		(*tmp)->addr=(faddr *)malloc(sizeof(faddr));
		(*tmp)->addr->name=NULL;
		(*tmp)->addr->zone=addr->zone;
		(*tmp)->addr->net=addr->net;
		(*tmp)->addr->node=addr->node;
		(*tmp)->addr->point=addr->point;
		(*tmp)->addr->domain=xstrcpy(addr->domain);
		if (flavor == 'i')
			(*tmp)->force = TRUE;
		else
			(*tmp)->force = FALSE;
		if (isflo == OUT_POL)
			(*tmp)->force = TRUE;

		switch (flavor) {
			case 'i': Syslog('+', "Immediate mail to %s", ascfnode((*tmp)->addr,0x1f));
				  break;
			case 'c': Syslog('+', "Crash mail to %s", ascfnode((*tmp)->addr,0x1f));
				  break;
			case 'o': Syslog('+', "Normal mail to %s",ascfnode((*tmp)->addr,0x1f));
				  break;
			case 'p': Syslog('+', "Poll request to %s",ascfnode((*tmp)->addr,0x1f));
				  break;
			default : Syslog('+', "Some mail (%c) to %s",flavor,ascfnode((*tmp)->addr,0x1f));
		}
	}

	return 0;
}


