/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Import a echomail message
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek                FIDO:           2:280/2802
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
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/dbmsgs.h"
#include "../lib/dbuser.h"
#include "rollover.h"
#include "storeecho.h"



extern	int	echo_in;		/* Total echomails		*/
extern	int	echo_imp;		/* Imported echomails		*/
extern	int	echo_dupe;		/* Dupe echomails		*/
extern	int	echo_bad;		/* Bad echomails		*/
extern	int	do_quiet;		/* Quiet flag			*/



/*
 * Store echomail into the JAM base.
 *
 *  0 - All seems well.
 *  1 - Can't access messagebase.
 *
 */
int storeecho(faddr *f, faddr *t, time_t mdate, int flags, char *subj, char *msgid, char *reply, int bad, int dupe, FILE *fp)
{
        int             result;
        unsigned long   crc2;
        char            *buf;

	/*
	 *  Update import counters
	 */
	if (!bad && !dupe) {
		StatAdd(&msgs.Received, 1L);
		msgs.LastRcvd = time(NULL);
		StatAdd(&mgroup.MsgsRcvd, 1L);
		mgroup.LastDate = time(NULL);
		UpdateMsgs();
	}

        if (bad) {
                if (strlen(CFG.badboard) == 0) {
                        Syslog('+', "Killing bad message");
                        return 0;
                } else {
                        if ((result = Msg_Open(CFG.badboard)))
                                Syslog('+', "Tossing in bad board");
                }
        } else if (dupe) {
                if (strlen(CFG.dupboard) == 0) {
                        Syslog('+', "Killing dupe message");
                        return 0;
                } else {
                        if ((result = Msg_Open(CFG.dupboard)))
                                Syslog('+', "Tossing in dupe board");
                }
        } else {
                result = Msg_Open(msgs.Base);
        }
        if (!result) {
                WriteError("Can't open JAMmb %s", msgs.Base);
                return 1;
        }

        if (Msg_Lock(30L)) {
		if (dupe)
			echo_dupe++;
		else if (bad)
			echo_bad++;
		else
                        echo_imp++;

                if (!do_quiet) {
                        colour(3, 0);
                        printf("\r%6u => %-40s\r", echo_in, msgs.Name);
                        fflush(stdout);
                }

                Msg_New();

                /*
                 * Fill subfields
                 */
                strcpy(Msg.From, f->name);
                strcpy(Msg.To, t->name);
                strcpy(Msg.FromAddress, ascfnode(f,0x1f));
                strcpy(Msg.Subject, subj);
                Msg.Written = mdate;
                Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
                Msg.Echomail = TRUE;

                /*
                 * These are the only usefull flags in echomail
                 */
                if ((flags & M_PVT) && ((msgs.MsgKinds == BOTH) || (msgs.MsgKinds == PRIVATE)))
                        Msg.Private = TRUE;
                if (flags & M_FILE)
                        Msg.FileAttach = TRUE;

                /*
                 * Set MSGID and REPLY crc.
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
                 * Start write the message
                 * If not a bad or dupe message, eat the first
                 * line (AREA:tag).
                 */
		buf = calloc(2049, sizeof(char));
                rewind(fp);
                if (!dupe && !bad)
                        fgets(buf , 2048, fp);
                Msg_Write(fp);
                Msg_AddMsg();
                Msg_UnLock();
                Msg_Close();
		free(buf);
		return 0;
        } else {
                Syslog('+', "Can't lock msgbase %s", msgs.Base);
                Msg_UnLock();
                Msg_Close();
                return 1;
        }
}


