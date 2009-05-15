/*****************************************************************************
 *
 * $Id: calllist.c,v 1.14 2006/05/22 12:09:15 mbse Exp $
 * Purpose ...............: mbtask - calllist
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "taskstat.h"
#include "taskutil.h"
#include "callstat.h"
#include "outstat.h"
#include "mbtask.h"
#include "calllist.h"



/*
 *  Global variables
 */
tocall			calllist[MAXTASKS];	/* Array with calllist	*/
extern int		internet;		/* Internet is down	*/
extern int		s_scanout;		/* Scan outbound sema	*/
extern _alist_l		*alist;			/* Nodes to call list	*/
extern int		pots_calls;
extern int		isdn_calls;
extern int		inet_calls;
extern int		pots_lines;		/* POTS lines available	*/
extern int		isdn_lines;		/* ISDN lines available	*/
extern struct taskrec	TCFG;



/*
 * Check the actual list of nodes to call. 
 * Returns number of systems to call.
 */
int check_calllist(void)
{
    int		    i, found, call_work = 0;
    struct _alist   *tmp;
    char	    *buf;

    buf = calloc(81, sizeof(char));

    /*
     * Check callist, remove obsolete entries.
     */
    for (i = 0; i < MAXTASKS; i++) {
	if (calllist[i].addr.zone) {
	    found = FALSE;
	    for (tmp = alist; tmp; tmp = tmp->next) {
		if ((calllist[i].addr.zone  == tmp->addr.zone) && (calllist[i].addr.net   == tmp->addr.net) &&
		    (calllist[i].addr.node  == tmp->addr.node) && (calllist[i].addr.point == tmp->addr.point) &&
		    ((tmp->flavors) & F_CALL)) {
		    found = TRUE;
		}
	    }
	    if (!found) {
		fido2str_r(calllist[i].addr, 0x01f, buf);
		Syslog('c', "Removing slot %d node %s from calllist", i, buf);
		memset(&calllist[i], 0, sizeof(tocall));
	    }
	}
    }

    if (pots_calls || isdn_calls || inet_calls) {
	for (tmp = alist; tmp; tmp = tmp->next) {
	    if ((((tmp->callmode == CM_INET) && TCFG.max_tcp && internet) ||
		 ((tmp->callmode == CM_ISDN) && isdn_lines) || ((tmp->callmode == CM_POTS) && pots_lines)) &&
		((tmp->flavors) & F_CALL)) {
		call_work++;

		/*
		 * Check if node is already in the list of systems to call.
		 */
		found = FALSE;
		for (i = 0; i < MAXTASKS; i++) {
		    if ((calllist[i].addr.zone  == tmp->addr.zone) && (calllist[i].addr.net   == tmp->addr.net) &&
			(calllist[i].addr.node  == tmp->addr.node) && (calllist[i].addr.point == tmp->addr.point)) {
			found = TRUE;
			/*
			 * Refresh last call status
			 */
			calllist[i].cst = tmp->cst;
		    }
		}

		/*
		 * Node not in the calllist, add node.
		 */
		if (!found) {
		    for (i = 0; i < MAXTASKS; i++) {
			if (!calllist[i].addr.zone) {
			    fido2str_r(tmp->addr, 0x1f, buf);
			    Syslog('c', "Adding %s to calllist slot %d", buf, i);
			    calllist[i].addr = tmp->addr;
			    calllist[i].cst = tmp->cst;
			    calllist[i].callmode = tmp->callmode;
			    calllist[i].moflags  = tmp->moflags;
			    calllist[i].diflags  = tmp->diflags;
			    calllist[i].ipflags  = tmp->ipflags;
			    break;
			}
		    }
		}
	    }
	}
    } else {
	if (s_scanout)
	    sem_set((char *)"scanout", FALSE);
    }

    call_work = 0;
    for (i = 0; i < MAXTASKS; i++) {
	if (calllist[i].addr.zone) {
	    if (!call_work) {
		Syslog('c', "Slot Call  Pid   Try Status  Mode    Modem    ISDN     TCP/IP   Address");
		Syslog('c', "---- ----- ----- --- ------- ------- -------- -------- -------- ----------------");
	    }
	    call_work++;
	    fido2str_r(calllist[i].addr, 0x1f, buf);
	    Syslog('c', "%4d %s %5d %3d %s %s %08x %08x %08x %s", i, calllist[i].calling?"true ":"false", calllist[i].taskpid,
		calllist[i].cst.tryno, callstatus(calllist[i].cst.trystat), callmode(calllist[i].callmode),
		calllist[i].moflags, calllist[i].diflags, calllist[i].ipflags, buf);
	}
    }

    free(buf);
    return call_work;
}


