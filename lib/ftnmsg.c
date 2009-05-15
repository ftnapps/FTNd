/*****************************************************************************
 *
 * $Id: ftnmsg.c,v 1.10 2005/10/11 20:49:42 mbse Exp $
 * File ..................: ftnmsg.c
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
#include "mbselib.h"



static char *months[] = {
(char *)"Jan",(char *)"Feb",(char *)"Mar",	
(char *)"Apr",(char *)"May",(char *)"Jun",
(char *)"Jul",(char *)"Aug",(char *)"Sep",
(char *)"Oct",(char *)"Nov",(char *)"Dec"
};


char *ftndate(time_t t)
{
    static char	buf[32];
    struct tm	*ptm;

    ptm = localtime(&t);
    if (ptm->tm_sec > 59)
	ptm->tm_sec = 59;

    snprintf(buf, 32, "%02d %s %02d  %02d:%02d:%02d",ptm->tm_mday,
		months[ptm->tm_mon], ptm->tm_year%100,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    return buf;
}



FILE *ftnmsghdr(ftnmsg *fmsg, FILE *pkt, faddr *routeaddr, char flavor, char *Pid)
{
	unsigned char	buffer[0x0e];
	time_t		Now;
	faddr		*from;

	if ((strlen(fmsg->to->name) > 36) || 
	    (strlen(fmsg->from->name) > 36) || 
	    (strlen(fmsg->subj) > 72))
		return NULL;

	if (routeaddr == NULL) 
		routeaddr = fmsg->to;
	pkt = openpkt(pkt, routeaddr, flavor, FALSE);
	if (pkt == NULL) 
		return NULL;

	if (fmsg->area)
		from = bestaka_s(fmsg->to);
	else
		from = fmsg->from;

	memset(&buffer, 0, sizeof(buffer));
	buffer[0x00] = 2;
	buffer[0x02] = (from->node & 0x00ff);
	buffer[0x03] = (from->node & 0xff00) >> 8;
	buffer[0x04] = (fmsg->to->node & 0x00ff);
	buffer[0x05] = (fmsg->to->node & 0xff00) >> 8;
	buffer[0x06] = (from->net & 0x00ff);
	buffer[0x07] = (from->net & 0xff00) >> 8;
	buffer[0x08] = (fmsg->to->net & 0x00ff);
	buffer[0x09] = (fmsg->to->net & 0xff00) >> 8;
	buffer[0x0a] = (fmsg->flags & 0x00ff);
	buffer[0x0b] = (fmsg->flags & 0xff00) >> 8;
	fwrite(buffer, 1, sizeof(buffer), pkt);

	fprintf(pkt, "%s%c", ftndate(fmsg->date), '\0');
	fprintf(pkt, "%s%c", fmsg->to->name?fmsg->to->name:(char *)"Sysop", '\0');
	fprintf(pkt, "%s%c", fmsg->from->name?fmsg->from->name:(char *)"Sysop", '\0');
	fprintf(pkt, "%s%c", fmsg->subj?fmsg->subj:(char *)"<none>", '\0');

	if (fmsg->area) {
		fprintf(pkt, "AREA:%s\r", fmsg->area);
	} else {
		if (fmsg->to->point)
			fprintf(pkt, "\1TOPT %u\r", fmsg->to->point);
		if (fmsg->from->point)
			fprintf(pkt, "\1FMPT %u\r", fmsg->from->point);
		fprintf(pkt, "\1INTL %u:%u/%u %u:%u/%u\r",
			fmsg->to->zone,
			fmsg->to->net,
			fmsg->to->node,
			fmsg->from->zone,
			fmsg->from->net,
			fmsg->from->node
			);
	}

	if (fmsg->msgid_s) 
		fprintf(pkt, "\1MSGID: %s\r", fmsg->msgid_s);
	else if (fmsg->msgid_a) 
		fprintf(pkt, "\1MSGID: %s %08x\r",
			fmsg->msgid_a,
			fmsg->msgid_n);

	if (fmsg->reply_s) 
		fprintf(pkt, "\1REPLY: %s\r", fmsg->reply_s);
	else if (fmsg->reply_a)
		fprintf(pkt, "\1REPLY: %s %08x\r",
			fmsg->reply_a,
			fmsg->reply_n);

	Now = time(NULL) - (gmt_offset((time_t)0) * 60);
	fprintf(pkt, "\001PID: %s %s (%s-%s)\r", Pid, VERSION, OsName(), OsCPU());
	fprintf(pkt, "\001TZUTC: %s\r", gmtoffset(Now));
	if (ferror(pkt)) 
		return NULL;
	else 
		return pkt;
}



void tidy_ftnmsg(ftnmsg *tmsg)
{
	if (tmsg == NULL) 
		return;

	tmsg->flags = 0;
	if (tmsg->to) 
		tidy_faddr(tmsg->to); 
	tmsg->to=NULL;
	if (tmsg->from) 
		tidy_faddr(tmsg->from); 
	tmsg->from=NULL;
	if (tmsg->subj) 
		free(tmsg->subj); 
	tmsg->subj=NULL;
	if (tmsg->msgid_s) 
		free(tmsg->msgid_s); 
	tmsg->msgid_s=NULL;
	if (tmsg->msgid_a) 
		free(tmsg->msgid_a); 
	tmsg->msgid_a=NULL;
	if (tmsg->reply_s) 
		free(tmsg->reply_s); 
	tmsg->reply_s=NULL;
	if (tmsg->reply_a) 
		free(tmsg->reply_a); 
	tmsg->reply_a=NULL;
	if (tmsg->origin) 
		free(tmsg->origin); 
	tmsg->origin=NULL;
	if (tmsg->area) 
		free(tmsg->area); 
	tmsg->area=NULL;
	free(tmsg);
}


