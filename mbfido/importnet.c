/*****************************************************************************
 *
 * File ..................: tosser/importnet.c
 * Purpose ...............: Import a netmail message
 * Last modification date : 20-Jan-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/dbmsgs.h"
#include "../lib/dbuser.h"
#include "rollover.h"
#include "importnet.h"



/*
 * Global variables
 */
extern	int	net_imp;		/* Netmails imported                */
extern	int	net_bad;		/* Bad netmails (tracking errors    */
extern	char	*subj;			/* Message subject		    */



/*
 * Import netmail into the BBS.
 *
 *  0 - All seems well.
 *  1 - Something went wrong.
 *
 */
int importnet(faddr *f, faddr *t, time_t mdate, int flags, FILE *fp)
{
	char		*msgid = NULL, *reply = NULL;
	int		result, i, empty = TRUE;
	unsigned long	crc2;
	char		*Buf;

	if (SearchNetBoard(t->zone, t->net)) {
		StatAdd(&msgs.Received, 1L);
		time(&msgs.LastRcvd);
		UpdateMsgs();

		result = Msg_Open(msgs.Base);
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
			strcpy(Msg.Subject, subj);
			Msg.Written = mdate;
			Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
			Msg.Netmail = TRUE;

			/*
			 * These are the only usefull flags in netmail
			 */
			if ((msgs.MsgKinds == BOTH) || (msgs.MsgKinds == PRIVATE)) {
				if (flags & M_PVT)
					Msg.Private = TRUE;
				else
					Msg.Private = FALSE;
			} else
				Msg.Private = TRUE;	/* Allways */
			if (flags & M_CRASH) 
				Msg.Crash = TRUE;
			if (flags & M_FILE)
				Msg.FileAttach = TRUE;
			if (flags & M_REQ)
				Msg.FileRequest = TRUE;
			if (flags & M_RRQ)
				Msg.ReceiptRequest = TRUE;

			/*
			 * Set MSGID and REPLYID crc.
			 */
			if (msgid != NULL) {
				crc2 = -1;
				Msg.MsgIdCRC = upd_crc32(msgid, crc2, strlen(msgid));
			}
			if (reply != NULL) {
				crc2 = -1;
				Msg.ReplyCRC = upd_crc32(reply, crc2, strlen(reply));
			}

			/*
			 *  Check if this is an empty netmail
			 */
			rewind(fp);
			Buf = calloc(2049, sizeof(char));
			while ((fgets(Buf, 2048, fp)) != NULL) {

				for (i = 0; i < strlen(Buf); i++) {
					if (*(Buf + i) == '\0')
						break;
					if (*(Buf + i) == '\n')
						*(Buf + i) = '\0';
					if (*(Buf + i) == '\r')
						*(Buf + i) = '\0';
				}
				if (*(Buf) != '\0') {
					if ((*(Buf) != '\001') &&
					    (strcmp(Buf, (char *)"--- ")))
						empty = FALSE;
				}
			}
			free(Buf);

			if (!empty) {
				Syslog('+', "Import netmail to %s", usr.sUserName);
				rewind(fp);
				Msg_Write(fp);
				Msg_AddMsg();
				net_imp++;
			} else {
				Syslog('+', "Empty netmail for %s dropped", usr.sUserName);
			}
			Msg_UnLock();
			Msg_Close();

			return 0;
		} else {
			WriteError("Can't lock msgbase %s", msgs.Base);
			Msg_Close();
			return 1;
		}
	} else {
		WriteError("Can't find a netmail board");
		net_bad++;
		return 1;
	} /* if SearchNetBoard() */
}


