/*****************************************************************************
 *
 * $Id: call.c,v 1.39 2005/09/12 13:47:09 mbse Exp $
 * Purpose ...............: Fidonet mailer 
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
#include "../lib/users.h"
#include "../lib/nodelist.h"
#include "../lib/mbsedb.h"
#include "session.h"
#include "callstat.h"
#include "call.h"
#include "config.h"
#include "dial.h"
#include "lutil.h"
#include "portsel.h"
#include "openport.h"
#include "opentcp.h"
#include "rdoptions.h"
#include "inbound.h"



extern int		tcp_mode;
extern int		telnet;
extern int		immediatecall;
extern char		*forcedphone;
extern char		*forcedline;
extern char		*inetaddr;
extern char		*protocol;
extern pid_t		mypid;


int portopen(faddr *addr)
{
    char    *p;
    int	    rc;

    if (inetaddr) {
	Syslog('d', "portopen inetaddr %s", inetaddr);
	if ((rc = opentcp(inetaddr))) {
	    Syslog('+', "Cannot connect %s", inetaddr);
	    nodeulock(addr, mypid);
	    return MBERR_NO_CONNECTION;
	}
	return MBERR_OK;
    }

    if (forcedline) {
	Syslog('d', "portopen forcedline %s", forcedline);
	p = forcedline;
	strncpy(history.tty, p, 6);

	if (load_port(p)) {
	    if ((rc = openport(p, ttyinfo.portspeed))) {
		Syslog('+', "Cannot open port %s",p);
		nodeulock(addr, mypid);
		putstatus(addr, 10, MBERR_PORTERROR);
		return MBERR_PORTERROR;
	    }
	    return MBERR_OK;
	} else {
	    nodeulock(addr, mypid);
	    putstatus(addr, 0, MBERR_PORTERROR);
	    return MBERR_PORTERROR;
	}
    }

    WriteError("No call method available, maybe missing parameters");
    return MBERR_NO_CONNECTION;
}



int call(faddr *addr)
{
    int		    rc = 1;
    char	    *p, temp[81];

    /*
     *  First check if node is locked, if not lock it immediatly
     *  or stop further waste of time and logfile space.
     */
    if (nodelock(addr, mypid)) {
	Syslog('+', "System %s is locked", ascfnode(addr, 0x1f));
	putstatus(addr, 0, MBERR_NODE_LOCKED);
	return MBERR_NODE_LOCKED;
    }

    if ((nlent = getnlent(addr)) == NULL) {
	WriteError("Cannot call %s: fatal in nodelist lookup", ascfnode(addr, 0x1f));
	putstatus(addr,10,MBERR_NODE_NOT_IN_LIST);
	nodeulock(addr, mypid);
	return MBERR_NODE_NOT_IN_LIST;
    }

    /*
     * Check if we got a URL to make the call.
     */
    if (nlent->url == NULL) {
	WriteError("Cannot call %s: no dialmethod available in nodelist", ascfnode(addr, 0x1f));
	putstatus(addr,10,MBERR_NODE_NOT_IN_LIST);
	nodeulock(addr, mypid);
	return MBERR_NODE_NOT_IN_LIST;
    }

    /*
     * Load the noderecord if the node is in the setup.
     */
    noderecord(addr);
    rdoptions(TRUE);

    /*
     * Fill default history info in case we get a FTS0001 session
     */
    strncpy(history.system_name, nlent->name, 35);
    strncpy(history.location, nlent->location, 35);
    strncpy(history.sysop, nlent->sysop, 35);
    history.aka.zone  = addr->zone;
    history.aka.net   = addr->net;
    history.aka.node  = addr->node;
    history.aka.point = addr->point;
    if (addr->domain && strlen(addr->domain))
	snprintf(history.aka.domain, 13, "%s", printable(addr->domain, 0));

    /*
     * Extract the protocol from the URL.
     */
    strncpy(temp, nlent->url, 80);
    p = strchr(temp, ':');
    *p = '\0';
    protocol = xstrcpy(temp);

    if (strcasecmp(protocol, "pots") && strcasecmp(protocol, "isdn")) {
	if (!inetaddr) {
	    /*
	     * Get the internet address from the URL.
	     */
	    p = strchr(nlent->url, '/');
	    p++;
	    p++;
	    inetaddr = xstrcpy(p);
	}
	RegTCP();

	if (tcp_mode == TCPMODE_NONE) {
	    if (strcmp(protocol, "binkp") == 0) {
		tcp_mode = TCPMODE_IBN;
	    } else if (strcmp(protocol, "fido") == 0) {
		tcp_mode = TCPMODE_IFC;
	    } else if (strcmp(protocol, "telnet") == 0) {
		tcp_mode = TCPMODE_ITN;
		telnet = TRUE;
	    } else {
		Syslog('+', "No common TCP/IP protocols for node %s", nlent->name);
		free(inetaddr);
		inetaddr = NULL;
	    }
	}

	if (inetaddr == NULL) {
	    WriteError("No IP address, abort call");
	    rc = MBERR_NO_IP_ADDRESS;
	    putstatus(addr, 10, rc);
	    nodeulock(addr, mypid);
	    return rc;
	}
    } else {
	if (!forcedphone) {
            /*
	     * Get the phone number from the URL.
	     */
	    p = strchr(nlent->url, '/');
	    p++;
	    p++;
	    forcedphone = xstrcpy(p);
	}
    }

    if (((nlent->can_pots && nlent->is_cm) == FALSE) && ((nlent->can_ip && nlent->is_icm) == FALSE) && (!IsZMH())) {
	Syslog('?', "Warning: calling non-CM system outside ZMH");
    }

    inbound_open(addr, TRUE, (tcp_mode == TCPMODE_IBN));	/* master sessions are secure */

    /*
     * Call when:
     *  there is a phone number and node is not down, hold or pvt
     *    or
     *  there is a fqdn/ip-address and node is not down or hold
     *      and
     *  nocall is false
     */
    if (((forcedphone && ((nlent->pflag & (NL_DUMMY | NL_DOWN | NL_HOLD | NL_PVT)) == 0)) ||
	 (inetaddr && ((nlent->pflag & (NL_DUMMY | NL_DOWN | NL_HOLD)) == 0))) && ((localoptions & NOCALL) == 0)) {
	Syslog('+', "Calling %s (%s, phone %s)", ascfnode(addr,0x1f), nlent->name, forcedphone);
	IsDoing("Call %s", ascfnode(addr, 0x0f));
	rc = portopen(addr);

	if ((rc == MBERR_OK) && (forcedphone)) {
	    if ((rc = dialphone(forcedphone))) {
		Syslog('+', "Dial failed");
		nodeulock(addr, mypid);
	    } 
	}

	if (rc == MBERR_OK) {
	    if (!inetaddr)
		nolocalport();

	    if (tcp_mode == TCPMODE_IBN)
		rc = session(addr,nlent,SESSION_MASTER,SESSION_BINKP,NULL);
	    else
		rc = session(addr,nlent,SESSION_MASTER,SESSION_UNKNOWN,NULL);
	}

	IsDoing("Disconnect");
	if (inetaddr) {
	    closetcp();
	} else {
	    hangup();
	    localport();
	    closeport();
	    if (strlen(modem.info)) {
		/*
		 * If mode info string defined, open port again to get
		 * the aftercall information. Mostly used with ISDN.
		 */
		portopen(addr);
		aftercall();
		closeport();
	    }
	}
    } else {
	IsDoing("NoCall");
	Syslog('+', "Cannot call %s (%s)", ascfnode(addr,0x1f), MBSE_SS(nlent->name));
	rc = MBERR_NO_CONNECTION;
	putstatus(addr, 10, rc);
	nodeulock(addr, mypid);
	return rc;
    }

    if ((rc == MBERR_NOT_ZMH) || (rc == MBERR_SESSION_ERROR))  /* Session error */
	putstatus(addr, 5, rc);
    else if ((rc == MBERR_NO_CONNECTION) || (rc == MBERR_UNKNOWN_SESSION) || (rc == MBERR_FTRANSFER))
	putstatus(addr,1,rc);
    else
	putstatus(addr,0,rc);
    return rc;
}


