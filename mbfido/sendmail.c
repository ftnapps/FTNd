/*****************************************************************************
 *
 * $Id: sendmail.c,v 1.14 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: Output a netmail to one of our links.
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
#include "../lib/mbsedb.h"
#include "addpkt.h"
#include "rollover.h"
#include "sendmail.h"



extern int	net_out;

/*
 *  Start a netmail to one of our nodes in the setup.
 *  Return a file descriptor if success else NULL.
 *  Later the pack routine will add these mails to the outbound.
 */
FILE *SendMgrMail(faddr *t, int Keep, int FileAttach, char *bymgr, char *subj, char *reply)
{
	FILE		*qp;
	time_t		Now;
	fidoaddr	Orig, Dest;
	faddr		From;
	unsigned	flags = M_PVT;
	char		ext[4];

	From = *bestaka_s(t);
	memset(&Orig, 0, sizeof(Orig));
	Orig.zone  = From.zone;
	Orig.net   = From.net;
	Orig.node  = From.node;
	Orig.point = From.point;
	snprintf(Orig.domain, 13, "%s", From.domain);

	memset(&Dest, 0, sizeof(Dest));
	Dest.zone  = t->zone;
	Dest.net   = t->net;
	Dest.node  = t->node;
	Dest.point = t->point;
	snprintf(Dest.domain, 13, "%s", t->domain);

	if (!SearchNode(Dest)) {
		Syslog('!', "SendMgrMail(): Can't find node %s", aka2str(Dest));
		return NULL;
	}

	Syslog('m', "  Netmail from %s to %s", aka2str(Orig), ascfnode(t, 0x1f));

	Now = time(NULL) - (gmt_offset((time_t)0) * 60);
	flags |= (nodes.Crash)            ? M_CRASH    : 0;
	flags |= (FileAttach)             ? M_FILE     : 0;
	flags |= (nodes.Hold)             ? M_HOLD     : 0;

	/*
	 *  Increase counters, update record and reload.
	 */
	StatAdd(&nodes.MailSent, 1L);
	UpdateNode();
	SearchNode(Dest);

	memset(&ext, 0, sizeof(ext));
	if (nodes.PackNetmail)
		snprintf(ext, 4, (char *)"qqq");
	else if (nodes.Crash)
		snprintf(ext, 4, (char *)"ccc");
	else if (nodes.Hold)
		snprintf(ext, 4, (char *)"hhh");
	else
		snprintf(ext, 4, (char *)"nnn");

	if ((qp = OpenPkt(Orig, Dest, (char *)ext)) == NULL)
		return NULL;

	if (AddMsgHdr(qp, &From, t, flags, 0, Now, nodes.Sysop, tlcap(bymgr), subj)) {
		fclose(qp);
		return NULL;
	}

	if (Dest.point)
		fprintf(qp, "\001TOPT %d\r", Dest.point);
	if (Orig.point)
		fprintf(qp, "\001FMPT %d\r", Orig.point);

	fprintf(qp, "\001INTL %d:%d/%d %d:%d/%d\r", Dest.zone, Dest.net, Dest.node, Orig.zone, Orig.net, Orig.node);

	/*
	 * Add MSGID, REPLY and PID
	 */
	fprintf(qp, "\001MSGID: %s %08x\r", aka2str(Orig), sequencer());
	if (reply != NULL)
		fprintf(qp, "\001REPLY: %s\r", reply);
	fprintf(qp, "\001PID: MBSE-FIDO %s (%s-%s)\r", VERSION, OsName(), OsCPU());
	fprintf(qp, "\001TZUTC: %s\r", gmtoffset(Now));
	return qp;
}



void CloseMail(FILE *qp, faddr *t)
{
    time_t	Now;
    struct tm	*tm;
    faddr	*ta;

    putc('\r', qp);
    Now = time(NULL);
    tm = gmtime(&Now);
    ta = bestaka_s(t);
    fprintf(qp, "\001Via %s @%d%02d%02d.%02d%02d%02d.02.UTC %s\r",
		ascfnode(ta, 0x1f), tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, 
		tm->tm_hour, tm->tm_min, tm->tm_sec, VERSION);
    tidy_faddr(ta);
    putc(0, qp);
    fclose(qp);
    net_out++;
}



