/*****************************************************************************
 *
 * $Id: ping.c,v 1.14 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: Ping Service
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "sendmail.h"
#include "postnetmail.h"
#include "ping.h"



/*
 * External declarations
 */
extern	int	do_quiet;



/*
 * Global variables
 */
extern	int	net_in;			/* Netmails received		    */
extern	int	net_out;		/* Netmails forwarded		    */
extern	int	net_bad;		/* Bad netmails (tracking errors    */
extern	char	*msgid;			/* Original message id		    */



int Ping(faddr *f, faddr *t, FILE *fp, int intransit)
{
    int	    rc = 0;
    char    *Buf;
    FILE    *np;
    time_t  Now;
    faddr   *from;

    Now = time(NULL);
    if (SearchFidonet(f->zone))
	f->domain = xstrcpy(fidonet.domain);

    Syslog('+', "%s ping msg from %s", intransit ? "Intransit":"Final", ascfnode(f, 0xff));
    Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
    rewind(fp);

    np = tmpfile();
    from = bestaka_s(f);
    if (intransit) {
	from->name = xstrcpy((char *)"Ping TRACE service");
    } else {
	from->zone  = t->zone;
	from->net   = t->net;
	from->node  = t->node;
	from->point = t->point;
        from->name  = xstrcpy((char *)"Ping service");
    }

    if (f->point)
	fprintf(np, "\001TOPT %d\r", f->point);
    if (from->point)
	fprintf(np, "\001FMPT %d\r", from->point);
    fprintf(np, "\001INTL %d:%d/%d %d:%d/%d\r", f->zone, f->net, f->node, from->zone, from->net, from->node);

    /*
     * Add MSGID, REPLY and PID
     */
    fprintf(np, "\001MSGID: %s %08x\r", ascfnode(from, 0x1f), sequencer());
    while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {
	Striplf(Buf);
	if (strncmp(Buf, "\001MSGID:", 7) == 0) {
	    fprintf(np, "\001REPLY:%s\r", Buf+7);
	}
    }
    fprintf(np, "\001PID: MBSE-FIDO %s (%s-%s)\r", VERSION, OsName(), OsCPU());
    fprintf(np, "\001TZUTC: %s\r", gmtoffset(Now));

    fprintf(np, "     Dear %s\r\r", MBSE_SS(f->name));
    if (intransit) {
	fprintf(np, "You did send a PING to %s\r", ascfnode(t, 0x1f));
	fprintf(np, "This is a TRACE response from \"%s\" aka %s\r", CFG.bbs_name, ascfnode(from, 0x1f));
	fprintf(np, "The time of arrival is %s\r", rfcdate(Now));
    } else
	fprintf(np, "Your Ping arrived here at %s\r", rfcdate(Now));
    fprintf(np, "Here are all the detected Via lines of the message from you:\r\r");
    fprintf(np, "======================================================================\r");

    rewind(fp);
    while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {
	Striplf(Buf);
	if (strncmp(Buf, "\1Via", 4) == 0) {
	    fprintf(np, "%s\r", Buf+1);
	}
    }
    fprintf(np, "======================================================================\r");

    fprintf(np, "\rWith regards, %s\r\r", CFG.sysop_name);
    fprintf(np, "%s\r", TearLine());
    Now = time(NULL) - (gmt_offset((time_t)0) * 60);
    rc = postnetmail(np, from, f, NULL, (char *)"Re: Ping", Now, 0x0000, FALSE, from->zone, f->zone);
    tidy_faddr(from);

    fclose(np);

    free(Buf);
    return rc;
}


