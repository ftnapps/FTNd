/*****************************************************************************
 *
 * File ..................: mbcico/dial.c
 * Purpose ...............: Fidonet mailer
 * Last modification date : 08-Jun-2001
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "portsel.h"
#include "config.h"
#include "chat.h"
#include "ttyio.h"
#include "session.h"
#include "dial.h"


extern time_t		c_start;
extern time_t		c_end;
extern int		online;
extern int		master;
int			carrier;
extern long		sentbytes;
extern long		rcvdbytes;
extern int		Loaded;


int initmodem(void)
{
	int	i;

	for (i = 0; i < 3; i++)
		if (strlen(modem.init[i]))
			if (chat(modem.init[i], CFG.timeoutreset, NULL)) {
				WriteError("dial: could not reset the modem");
				return 1;
			}
	return 0;
}



int dialphone(char *Phone)
{
	int	rc;

	Syslog('+', "dial: %s (%s)",MBSE_SS(Phone), MBSE_SS(tranphone(Phone)));
	carrier = FALSE;

	if (initmodem())
		return 2;

	rc = 0;
	if (strlen(nodes.phone[0])) {
		if (strlen(nodes.dial)) 
			rc = chat(nodes.dial, CFG.timeoutconnect, nodes.phone[0]);
		else
			rc = chat(modem.dial, CFG.timeoutconnect, nodes.phone[0]);
		if ((rc == 0) && strlen(nodes.phone[1])) {
			if (strlen(nodes.dial))
				rc = chat(nodes.dial, CFG.timeoutconnect, nodes.phone[1]);
			else
				rc = chat(modem.dial, CFG.timeoutconnect, nodes.phone[1]);
		}
	} else {
		if (strlen(nodes.dial))
			rc = chat(nodes.dial, CFG.timeoutconnect, Phone);
		else
			rc = chat(modem.dial, CFG.timeoutconnect, Phone);
	}

	if (rc) {
		Syslog('+', "Could not connect to the remote");
		return 1;
	} else {
		c_start = time(NULL);
		carrier = TRUE;
		return 0;
	}
}



int hangup()
{
	char	*tmp;
	FILE	*fp;

	FLUSHIN();
	FLUSHOUT();
	if (strlen(modem.hangup))
		chat(modem.hangup, CFG.timeoutreset, NULL);

	if (carrier) {
		time(&c_end);
		online += (c_end - c_start);
		Syslog('+', "Connection time %s", t_elapsed(c_start, c_end));
		carrier = FALSE;
		history.offline = c_end;
		history.online  = c_start;
		history.sent_bytes = sentbytes;
		history.rcvd_bytes = rcvdbytes;
		history.inbound = ~master;
		tmp = calloc(128, sizeof(char));
		sprintf(tmp, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
		if ((fp = fopen(tmp, "a")) == NULL)
			WriteError("$Can't open %s", tmp);
		else {
			fwrite(&history, sizeof(history), 1, fp);
			fclose(fp);
		}
		free(tmp);
		memset(&history, 0, sizeof(history));
		if (Loaded) {
			Syslog('s', "Updateing noderecord %s", aka2str(nodes.Aka[0]));
			nodes.LastDate = time(NULL);
			UpdateNode();
		}
	}
	FLUSHIN();
	FLUSHOUT();
	return 0;
}



int aftercall()
{
	if (strlen(modem.info)) {
		Syslog('d', "Reading link stat (aftercall)");
		FLUSHIN();
		FLUSHOUT();
		chat(modem.info, CFG.timeoutreset, NULL);
	}
	return 0;
}


