/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
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


extern int		tcp_mode;
extern int		forcedcalls;
extern int		immediatecall;
extern char		*forcedphone;
extern char		*forcedline;
extern char		*inetaddr;



int portopen(faddr *addr)
{
	char	*p;
	int	rc;

	if (inetaddr) {
		Syslog('d', "portopen inetaddr %s", inetaddr);
		if ((rc = opentcp(inetaddr))) {
			Syslog('+', "Cannot connect %s", inetaddr);
			nodeulock(addr);
			putstatus(addr,1,ST_NOCONN);
			return ST_NOCONN;
		}
		return 0;
	}

	if (forcedline) {
		Syslog('d', "portopen forcedline %s", forcedline);
		p = forcedline;
		strncpy(history.tty, p, 6);

		if (load_port(p)) {
			if ((rc = openport(p, ttyinfo.portspeed))) {
				Syslog('+', "Cannot open port %s",p);
				nodeulock(addr);
				putstatus(addr, 10, ST_PORTERR);
				return ST_PORTERR;
			}
			return ST_PORTOK;
		} else {
			nodeulock(addr);
			putstatus(addr, 0, ST_PORTERR);
			return ST_PORTERR;
		}
	}

	WriteError("No call method available");
	return ST_PORTERR;
}



int call(faddr *addr)
{
	int		i, rc = 1;
	struct hostent	*he;

	/*
	 *  Don't call points, call their boss instead.
	 */
	addr->point = 0;

	/*
	 *  First check if node is locked, if not lock it immediatly
	 *  or stop further waste of time and logfile space.
	 */
	if (nodelock(addr)) {
		Syslog('+', "System %s is locked", ascfnode(addr, 0x1f));
		putstatus(addr, 0, ST_LOCKED);
		return ST_LOCKED;
	}

	if ((nlent = getnlent(addr)) == NULL) {
		WriteError("Cannot call %s: fatal in nodelist lookup", ascfnode(addr, 0x1f));
		putstatus(addr,0,ST_LOOKUP);
		nodeulock(addr);
		return ST_LOOKUP;
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
		sprintf(history.aka.domain, "%s", addr->domain);

	/*
	 * First see if this node can be reached over the internet and
	 * that internet calls are allowed.
	 */
	if (nlent->iflags && ((localoptions & NOIBN & NOITN & NOIFC) == 0)) {
		if (!inetaddr) {
			Syslog('d', "Trying to find IP address...");
			/*
			 * There is no fdn or IP address at the commandline.
			 * First check nodesetup for an override in the phone field.
			 */
			if (strlen(nodes.phone[0])) {
				inetaddr = xstrcpy(nodes.phone[0]);
			} else if (strlen(nodes.phone[1])) {
				inetaddr = xstrcpy(nodes.phone[1]);
			} else {
				/*
				 * Try to find the fdn in several places in the nodelist fields.
				 */
				if ((nlent->phone != NULL) && (strncmp(nlent->phone, (char *)"000-", 4) == 0)) {
					inetaddr = xstrcpy(nlent->phone+4);
					for (i = 0; i < strlen(inetaddr); i++)
						if (inetaddr[i] == '-')
							inetaddr[i] = '.';
					Syslog('d', "Got IP address from phone field");
				} else if ((he = gethostbyname(nlent->name))) {
					inetaddr = xstrcpy(nlent->name);
					Syslog('d', "Got hostname from nodelist system name");
				} else if ((he = gethostbyname(nlent->location))) {
					/*
					 * A fdn at the nodelist location field is not in the specs
					 * but the real world differs from the specs.
					 */
					inetaddr = xstrcpy(nlent->location);
					Syslog('d', "Got hostname from nodelist location");
				}
			}
		}

		/*
		 * If we have an internet address, set protocol
		 */
		if (inetaddr) {
			RegTCP();
			Syslog('d', "TCP/IP node \"%s\"", MBSE_SS(inetaddr));

			if (tcp_mode == TCPMODE_NONE) {
				/*
				 * If protocol not forced at the commandline, get it
				 * from the nodelist. If it fails, fallback to dial.
				 * Priority IBN, IFC, ITN.
				 */
				if ((nlent->iflags & IP_IBN) && ((localoptions & NOIBN) == 0)) {
					tcp_mode = TCPMODE_IBN;
					Syslog('d', "TCP/IP mode set to IBN");
				} else if ((nlent->iflags & IP_IFC) && ((localoptions & NOIFC) == 0)) {
					tcp_mode = TCPMODE_IFC;
					Syslog('d', "TCP/IP mode set to IFC");
				} else if ((nlent->iflags & IP_ITN) && ((localoptions & NOITN) == 0)) {
					tcp_mode = TCPMODE_ITN;
					Syslog('d', "TCP/IP mode seto to ITN");
				} else {
					Syslog('+', "No common TCP/IP protocols for node %s", nlent->name);
					free(inetaddr);
					inetaddr = NULL;
				}
			}
		} else {
			WriteError("No IP address, abort call");
			rc = ST_NOCALL8;
			putstatus(addr, 10, rc);
			nodeulock(addr);
			return rc;
		}
	}

	if (((nlent->oflags & OL_CM) == 0) && (!IsZMH())) {
		Syslog('?', "Warning: calling MO system outside ZMH");
	}

	if (inbound)
		free(inbound);
	inbound = xstrcpy(CFG.pinbound); /* master sessions are secure */

	/*
	 * Call when:
	 *  the nodelist has a phone, or phone on commandline, or TCP address given
	 * and
	 *  nodenumber on commandline, or node is CM and not down, hold, pvt
	 * and
	 *  nocall is false
	 */
	if ((nlent->phone || forcedphone || inetaddr ) && 
	     (forcedcalls || (((nlent->pflag & (NL_DUMMY|NL_DOWN|NL_HOLD|NL_PVT)) == 0) && ((localoptions & NOCALL) == 0)))) {
		Syslog('+', "Calling %s (%s, phone %s)",ascfnode(addr,0x1f), nlent->name,nlent->phone?nlent->phone:forcedphone);
		IsDoing("Call %s", ascfnode(addr, 0x0f));
		rc = portopen(addr);

		if ((rc == 0) && (!inetaddr)) {
			if ((rc = dialphone(forcedphone?forcedphone:nlent->phone))) {
				Syslog('+', "Dial failed");
				nodeulock(addr);
				rc+=1; /* rc=2 - dial fail, rc=3 - could not reset */
			} 
		}

		if (rc == 0) {
			if (!inetaddr)
				nolocalport();

			if (tcp_mode == TCPMODE_IBN)
				rc = session(addr,nlent,SESSION_MASTER,SESSION_BINKP,NULL);
			else
				rc = session(addr,nlent,SESSION_MASTER,SESSION_UNKNOWN,NULL);

			if (rc) 
				rc=abs(rc)+10;
		}

		IsDoing("Disconnect");
		if (inetaddr) {
			closetcp();
		} else {
			hangup();
			if (rc == 0)
				aftercall();
			localport();
			closeport();
		}
	} else {
		IsDoing("NoCall");
		Syslog('+', "Cannot call %s (%s, phone %s)", ascfnode(addr,0x1f),MBSE_SS(nlent->name), MBSE_SS(nlent->phone));
		if ((nlent->phone || forcedphone || inetaddr ))
			rc=ST_NOCALL8;
		else 
			rc=ST_NOCALL7;
		putstatus(addr, 10, rc);
		nodeulock(addr);
		return rc;
	}

	if ((rc > 10) && (rc < 20))  /* Session error */
		putstatus(addr, 5, rc);
	else if ((rc == 2) || (rc == 30))
		putstatus(addr,1,rc);
	else
		putstatus(addr,0,rc);
	return rc;
}


