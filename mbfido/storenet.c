/*****************************************************************************
 *
 * $Id: storenet.c,v 1.15 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: Import a netmail message in the message base.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/mbsedb.h"
#include "msgflags.h"
#include "rollover.h"
#include "storenet.h"



/*
 * Global variables
 */
extern	int	net_imp;		/* Netmails imported                */
extern	int	net_bad;		/* Bad netmails (tracking errors    */



/*
 * Store netmail into the JAM base.
 *
 *  0 - All seems well.
 *  1 - Can't access messagebase.
 *  2 - Can't find a netmail board.
 *
 */
int storenet(faddr *f, faddr *t, time_t mdate, int flags, char *Subj, char *msgid, char *reply, FILE *fp, char *flagstr)
{
    int		    result = FALSE, i, empty = TRUE, bad;
    unsigned int    crc2;
    char	    *Buf;

    mbse_CleanSubject(Subj);

    if (! SearchNetBoard(t->zone, t->net)) {
	bad = TRUE;
	WriteError("Can't find netmail board for %d:%d/%d", t->zone, t->net, t->node);
	if (strlen(CFG.badboard) == 0) {
	    Syslog('+', "Killing bad message, no badmail area");
	    net_bad++;
	    return 2;
	} else {
	    if ((result = Msg_Open(CFG.badboard)))
		Syslog('+', "Tossing in bad board");
	}
    } else {
	bad = FALSE;
	StatAdd(&msgs.Received, 1L);
	msgs.LastRcvd = time(NULL);
	UpdateMsgs();
	result = Msg_Open(msgs.Base);
    }

    if (!result) {
	WriteError("Can't open msgbase %s", msgs.Base);
	net_bad++;
	return 1;
    }

    if (Msg_Lock(30L)) {
	Msg_New();
	Syslog('m', "Flagfield 0x%04x", flags);
	strcpy(Msg.From, f->name);
	strcpy(Msg.To, usr.sUserName);
	strcpy(Msg.FromAddress, ascfnode(f,0x1f));
	strcpy(Msg.ToAddress, ascfnode(t,0x1f));
	strcpy(Msg.Subject, Subj);
	Msg.Written = mdate;
	Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
	Msg.Netmail = TRUE;

	/*
	 *  Set flags for the message base.
	 */
	if (bad) {
	    Msg.Private = (((flags & M_PVT) ? TRUE:FALSE) || flag_on((char *)"PVT", flagstr));
	} else if ((msgs.MsgKinds == BOTH) || (msgs.MsgKinds == PRIVATE)) {
	    Msg.Private = (((flags & M_PVT) ? TRUE:FALSE) || flag_on((char *)"PVT", flagstr));
	} else {
	    Msg.Private = TRUE;	/* Allways */
	}
	Msg.Crash          = ((flags & M_CRASH)    || flag_on((char *)"CRA", flagstr));
	Msg.FileAttach     = ((flags & M_FILE)     || flag_on((char *)"FIL", flagstr));
	Msg.Intransit      = ((flags & M_TRANSIT));
	Msg.FileRequest    = ((flags & M_REQ)      || flag_on((char *)"FRQ", flagstr));
	Msg.ReceiptRequest = ((flags & M_RRQ)      || flag_on((char *)"RRQ", flagstr));
	Msg.Immediate      =                          flag_on((char *)"IMM", flagstr);
	Msg.Direct         =                          flag_on((char *)"DIR", flagstr);
	Msg.Gate           =                          flag_on((char *)"ZON", flagstr);
	Msg.ConfirmRequest = ((flags & M_AUDIT)    || flag_on((char *)"CFM", flagstr));
	Msg.Orphan         = ((flags & M_ORPHAN));

	if (Msg.ReceiptRequest) {
	    Syslog('+', "Netmail has ReceiptRequest flag, no message created");
	}

	/*
	 * Set MSGID and REPLY crc.
	 */
	if (msgid != NULL) {
	    crc2 = 0xffffffffL;
	    Msg.MsgIdCRC = upd_crc32(msgid, crc2, strlen(msgid));
	}
	if (reply != NULL) {
	    crc2 = 0xffffffffL;
	    Msg.ReplyCRC = upd_crc32(reply, crc2, strlen(reply));
	}

	/*
	 *  Check if this is an empty netmail
	 */
	rewind(fp);
	Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
	while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {

	    for (i = 0; i < strlen(Buf); i++) {
		if (*(Buf + i) == '\0')
		    break;
		if (*(Buf + i) == '\n')
		    *(Buf + i) = '\0';
		if (*(Buf + i) == '\r')
		    *(Buf + i) = '\0';
	    }
	    if (*(Buf) != '\0') {
		if ((*(Buf) != '\001') && (strcmp(Buf, (char *)"--- ")))
		    empty = FALSE;
	    }
	}
	free(Buf);

	if (!empty) {
	    if (bad)
		Syslog('+', "Import netmail to %s in badmail board", usr.sUserName);
	    else
		Syslog('+', "Import netmail to %s", usr.sUserName);
	    rewind(fp);
	    Msg_Write(fp);
	    Msg_AddMsg();
	    if (bad)
		net_bad++;
	    else
		net_imp++;
	} else {
	    Syslog('+', "Empty netmail for %s dropped", usr.sUserName);
	}
	Msg_UnLock();
	Msg_Close();

	return 0;
    } else {
	WriteError("Can't lock msgbase %s, netmail is lost", msgs.Base);
	Msg_Close();
	return 1;
    }
}


