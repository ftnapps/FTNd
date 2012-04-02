/*****************************************************************************
 *
 * $Id: dial.c,v 1.16 2005/10/16 11:37:54 mbse Exp $
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
#include "config.h"
#include "chat.h"
#include "ttyio.h"
#include "session.h"
#include "dial.h"


extern time_t	c_start;
extern time_t	c_end;
extern int	online;
extern int	master;
int		carrier;
extern int	sentbytes;
extern int	rcvdbytes;
extern int	Loaded;


int initmodem(void)
{
    int	i;

    for (i = 0; i < 3; i++)
	if (strlen(modem.init[i]))
	    if (chat(modem.init[i], CFG.timeoutreset, FALSE, NULL)) {
		WriteError("dial: could not reset the modem");
		return MBERR_MODEM_ERROR;
	    }
    return MBERR_OK;
}



int dialphone(char *Phone)
{
    int	rc;

    Syslog('+', "dial: %s (%s)",MBSE_SS(Phone), MBSE_SS(tranphone(Phone)));
    carrier = FALSE;

    if (initmodem())
	return MBERR_MODEM_ERROR;

    rc = 0;
    if (strlen(nodes.phone[0])) {
	if (strlen(nodes.dial)) 
	    rc = chat(nodes.dial, CFG.timeoutconnect, FALSE, nodes.phone[0]);
	else
	    rc = chat(modem.dial, CFG.timeoutconnect, FALSE, nodes.phone[0]);
	if ((rc == 0) && strlen(nodes.phone[1])) {
	    if (strlen(nodes.dial))
		rc = chat(nodes.dial, CFG.timeoutconnect, FALSE, nodes.phone[1]);
	    else
		rc = chat(modem.dial, CFG.timeoutconnect, FALSE, nodes.phone[1]);
	}
    } else {
	if (strlen(nodes.dial))
	    rc = chat(nodes.dial, CFG.timeoutconnect, FALSE, Phone);
	else
	    rc = chat(modem.dial, CFG.timeoutconnect, FALSE, Phone);
    }

    if (rc) {
	Syslog('+', "Could not connect to the remote");
	return MBERR_NO_CONNECTION;
    } else {
	c_start = time(NULL);
	carrier = TRUE;
	return MBERR_OK;
    }
}



int hangup()
{
    char    *tmp;
    FILE    *fp;

    FLUSHIN();
    FLUSHOUT();
    if (strlen(modem.hangup))
	chat(modem.hangup, CFG.timeoutreset, FALSE, NULL);

    if (carrier) {
	c_end = time(NULL);
	online += (c_end - c_start);
	Syslog('+', "Connection time %s", t_elapsed(c_start, c_end));
	carrier = FALSE;
	history.offline = (int)c_end;
	history.online  = (int)c_start;
	history.sent_bytes = sentbytes;
	history.rcvd_bytes = rcvdbytes;
	history.inbound = ~master;
	tmp = calloc(PATH_MAX, sizeof(char));
	snprintf(tmp, PATH_MAX -1, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
	if ((fp = fopen(tmp, "a")) == NULL)
	    WriteError("$Can't open %s", tmp);
	else {
	    fwrite(&history, sizeof(history), 1, fp);
	    fclose(fp);
	}
	free(tmp);
	memset(&history, 0, sizeof(history));
	if (Loaded) {
	    nodes.LastDate = time(NULL);
	    UpdateNode();
	}
    }
    FLUSHIN();
    FLUSHOUT();
    return MBERR_OK;
}



void aftercall()
{
    FLUSHIN();
    FLUSHOUT();
    chat(modem.info, CFG.timeoutreset, TRUE, NULL);
}


