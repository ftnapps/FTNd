/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer - awnser a call
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
#include "lutil.h"
#include "session.h"
#include "config.h"
#include "answer.h"
#include "openport.h"
#include "portsel.h"
#include "dial.h"
#include "rdoptions.h"
#include "mbcico.h"


extern int		carrier, online;
extern time_t		c_start, c_end;
extern unsigned long    sentbytes;
extern unsigned long    rcvdbytes;
extern int		Loaded;

int answer(char *stype)
{
	int	st, rc;
	char	*p, *q;
	FILE	*fp;

	/*
	 *  mgetty set's the environment variable CONNECT and CALLER_ID,
	 *  so if they are present, we might as well make log entries.
	 */
	if ((q = getenv("CONNECT")) != NULL)
		Syslog('+', "CONNECT %s", q);
	if ((q = getenv("CALLER_ID")) != NULL)
		if (strncmp(q, "none", 4))
			Syslog('+', "CALLER  %s", q);

	/*
	 *  Incoming calls from modem/ISDN lines do have a tty.
	 *  Network calls don't have a tty attached.
	 */
	carrier = TRUE;
	p = ttyname(0);
	if (p) {
		q = strrchr(ttyname(0), '/');
		if (q)
			p = q + 1;
		strncpy(history.tty, p, 6);
		if (load_port(p))
			Syslog('d', "Port %s, modem %s", ttyinfo.tty, modem.modem);
		else
			Syslog('d', "Port and modem not loaded!");
	}

	if ((nlent = getnlent(NULL)) == NULL) {
		WriteError("could not get dummy nodelist entry");
		return 1;
	}

	c_start = time(NULL);
	rdoptions(FALSE);

	if (inbound)
		free(inbound);
	inbound = xstrcpy(CFG.inbound); /* slave session is unsecure by default */

	if (stype == NULL) {
		st=SESSION_UNKNOWN;
	} else if (strcmp(stype,"tsync") == 0) {
		st=SESSION_FTSC;
		IsDoing("Answer ftsc");
	} else if (strcmp(stype,"yoohoo") == 0) {
		st=SESSION_YOOHOO;
		IsDoing("Answer yoohoo");
	} else if (strncmp(stype,"**EMSI_",7) == 0) {
		st=SESSION_EMSI;
		IsDoing("Answer EMSI");
	} else if (strncmp(stype,"ibn",3) == 0) {
		st=SESSION_BINKP;
		IsDoing("Answer Binkp");
	} else {
		st=SESSION_UNKNOWN;
		IsDoing("Answer unknown");
	}

	if ((rc = rawport()) != 0)
		WriteError("Unable to set raw mode");
	else {
		nolocalport();
		rc=session(NULL,NULL,SESSION_SLAVE,st,stype);
	}

	cookedport();
	if (p) {
		/*
		 *  Hangup will write the history record.
		 */
		hangup();
	} else {
		/*
		 *  Network call, write history record.
		 */
		c_end = time(NULL);
		online += (c_end - c_start);

		history.online  = c_start;
		history.offline = c_end;
		history.sent_bytes = sentbytes;
		history.rcvd_bytes = rcvdbytes;
		history.inbound = TRUE;

		p = calloc(PATH_MAX, sizeof(char));
		sprintf(p, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
		if ((fp = fopen(p, "a")) == NULL)
			WriteError("$Can't open %s", p);
		else {
			fwrite(&history, sizeof(history), 1, fp);
			fclose(fp);
		}
		free(p);
		if (Loaded) {
			nodes.LastDate = time(NULL);
			UpdateNode();
		}
	}
	return rc;
}


